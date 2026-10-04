#include "operations/result_wire.h"
#include "operations/json_args.h"
#include "operations/query.h"
#include "operations/store_host.h"

#include "cypher/cypher.h"
#include "foundation/sha256.h"
#include "store/store.h"
#include "operations/compact_out.h"
#include "yyjson/yyjson.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Visible rows are a presentation budget; 99998 is the largest page the engine's
 * 100k row ceiling can still continue past. */
enum { QUERY_MAX_VISIBLE_ROWS = 99998, QUERY_DEFAULT_VISIBLE_ROWS = 200 };

static char *copy_string(const char *text) {
    if (!text)
        return NULL;
    return cbm_mem_strdup(CBM_MEM_CLASS_OPERATION_ARG, text);
}

static yyjson_doc *read_args(const char *args_json) {
    return yyjson_read(args_json ? args_json : "{}", strlen(args_json ? args_json : "{}"), 0);
}

static char *string_arg(const char *args_json, const char *name) {
    yyjson_doc *doc = read_args(args_json);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    char *result = value && yyjson_is_str(value) ? copy_string(yyjson_get_str(value)) : NULL;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static char *project_arg(const char *args_json) {
    static const char *const names[] = {"project", "project_name", "project_id", "projectName"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        char *value = string_arg(args_json, names[i]);
        if (value)
            return value;
    }
    return NULL;
}

static int int_arg(const char *args_json, const char *name, int fallback) {
    yyjson_doc *doc = read_args(args_json);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    int result = value && yyjson_is_int(value) ? (int)yyjson_get_sint(value) : fallback;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static cbm_operation_result_t error_result(const char *message) {
    return cbm_operation_result_copy(message ? message : "query execution failed", true);
}

/* Snapshot continuation cursor.
 *
 * Offset paging over a live graph can skip or repeat rows when the index
 * changes between calls. A cursor binds a continuation to (1) the project's
 * index generation, (2) the query/project/graph identity and (3) a digest of
 * the complete materialized result. The query is re-evaluated on every page, so
 * a cursor only resumes when the full result is byte-identical.
 *
 * Token: q1.<generation>.<params hash>.<result digest>.<offset>.<seal>
 * The short seal makes an edited offset or token field fail closed instead of
 * silently skipping rows. It is an integrity checksum for an opaque token, not
 * an authentication MAC. */
enum { QUERY_CURSOR_TOKEN_CAP = 224, QUERY_GENERATION_CAP = 96, QUERY_CURSOR_DIGEST_HEX_LEN = 32 };

typedef struct {
    char generation[QUERY_GENERATION_CAP];
    uint64_t params_hash;
    char result_digest[QUERY_CURSOR_DIGEST_HEX_LEN + 1];
    int offset;
} query_cursor_t;

typedef struct {
    const char *generation;
    uint64_t params_hash;
    const char *result_digest;
    bool can_mint;
} query_cursor_context_t;

static void query_sha_u64(cbm_sha256_ctx *sha, uint64_t value) {
    uint8_t bytes[8];
    for (int index = 7; index >= 0; index--) {
        bytes[index] = (uint8_t)(value & 0xffU);
        value >>= 8U;
    }
    cbm_sha256_update(sha, bytes, sizeof(bytes));
}

static void query_sha_field(cbm_sha256_ctx *sha, const char *value) {
    const uint8_t present = value ? 1U : 0U;
    cbm_sha256_update(sha, &present, sizeof(present));
    if (!value)
        return;
    size_t length = strlen(value);
    query_sha_u64(sha, (uint64_t)length);
    cbm_sha256_update(sha, value, length);
}

static void query_digest_hex(const uint8_t digest[CBM_SHA256_DIGEST_LEN],
                             char out[CBM_SHA256_HEX_LEN + 1]) {
    static const char hex[] = "0123456789abcdef";
    for (int index = 0; index < CBM_SHA256_DIGEST_LEN; index++) {
        out[index * 2] = hex[digest[index] >> 4U];
        out[index * 2 + 1] = hex[digest[index] & 0x0fU];
    }
    out[CBM_SHA256_HEX_LEN] = '\0';
}

static uint64_t query_params_hash(const char *project, const char *query, const char *graph) {
    static const char domain[] = "cbm.query_graph.params.v1";
    cbm_sha256_ctx sha;
    cbm_sha256_init(&sha);
    cbm_sha256_update(&sha, domain, sizeof(domain));
    query_sha_field(&sha, project);
    query_sha_field(&sha, query);
    query_sha_field(&sha, graph);
    uint8_t digest[CBM_SHA256_DIGEST_LEN];
    cbm_sha256_final(&sha, digest);
    uint64_t hash = 0;
    for (int index = 0; index < 8; index++)
        hash = (hash << 8U) | digest[index];
    return hash;
}

/* Covers ordered columns and rows, explicit NULL markers, engine truncation and
 * the warning. */
static void query_result_digest(const cbm_cypher_result_t *result,
                                char out[CBM_SHA256_HEX_LEN + 1]) {
    static const char domain[] = "cbm.query_graph.materialization.v1";
    cbm_sha256_ctx sha;
    cbm_sha256_init(&sha);
    cbm_sha256_update(&sha, domain, sizeof(domain));
    query_sha_u64(&sha, (uint64_t)result->col_count);
    query_sha_u64(&sha, (uint64_t)result->row_count);
    const uint8_t truncated = result->truncated ? 1U : 0U;
    cbm_sha256_update(&sha, &truncated, sizeof(truncated));
    query_sha_field(&sha, result->warning);
    for (int column = 0; column < result->col_count; column++)
        query_sha_field(&sha, result->columns ? result->columns[column] : NULL);
    for (int row = 0; row < result->row_count; row++) {
        const char **cells = result->rows ? result->rows[row] : NULL;
        for (int column = 0; column < result->col_count; column++)
            query_sha_field(&sha, cells ? cells[column] : NULL);
    }
    uint8_t digest[CBM_SHA256_DIGEST_LEN];
    cbm_sha256_final(&sha, digest);
    query_digest_hex(digest, out);
}

static void query_cursor_seal(const query_cursor_t *cursor, char out[17]) {
    static const char domain[] = "cbm.query_graph.cursor.v1";
    cbm_sha256_ctx sha;
    cbm_sha256_init(&sha);
    cbm_sha256_update(&sha, domain, sizeof(domain));
    query_sha_field(&sha, cursor->generation);
    query_sha_u64(&sha, cursor->params_hash);
    query_sha_field(&sha, cursor->result_digest);
    query_sha_u64(&sha, (uint64_t)cursor->offset);
    uint8_t digest[CBM_SHA256_DIGEST_LEN];
    cbm_sha256_final(&sha, digest);
    char full_hex[CBM_SHA256_HEX_LEN + 1];
    query_digest_hex(digest, full_hex);
    memcpy(out, full_hex, 16);
    out[16] = '\0';
}

static bool query_cursor_encode(const query_cursor_context_t *context, int offset,
                                char out[QUERY_CURSOR_TOKEN_CAP]) {
    if (!context || !context->can_mint || !context->generation || !context->result_digest ||
        offset <= 0)
        return false;
    query_cursor_t cursor = {.params_hash = context->params_hash, .offset = offset};
    snprintf(cursor.generation, sizeof(cursor.generation), "%s", context->generation);
    snprintf(cursor.result_digest, sizeof(cursor.result_digest), "%.*s",
             QUERY_CURSOR_DIGEST_HEX_LEN, context->result_digest);
    char seal[17];
    query_cursor_seal(&cursor, seal);
    int written =
        snprintf(out, QUERY_CURSOR_TOKEN_CAP, "q1.%s.%016llx.%s.%d.%s", cursor.generation,
                 (unsigned long long)cursor.params_hash, cursor.result_digest, cursor.offset, seal);
    return written > 0 && written < QUERY_CURSOR_TOKEN_CAP;
}

static bool query_hex_span(const char *begin, const char *end, size_t expected) {
    if (!begin || !end || end < begin || (size_t)(end - begin) != expected)
        return false;
    for (const char *digit = begin; digit < end; digit++)
        if (!isxdigit((unsigned char)*digit))
            return false;
    return true;
}

/* Decode and validate every token field that does not require executing the
 * query. The materialization digest itself is checked right after the full
 * query has been re-executed. Returns NULL when valid, else the error text. */
static const char *query_cursor_decode(const char *token, const char *current_generation,
                                       uint64_t expected_params_hash, query_cursor_t *out) {
    static const char invalid[] =
        "invalid_cursor: unrecognized or modified token - re-run the original query without "
        "'cursor'";
    memset(out, 0, sizeof(*out));
    if (!token || strncmp(token, "q1.", 3) != 0)
        return invalid;
    const char *generation_start = token + 3;
    const char *generation_end = strchr(generation_start, '.');
    if (!generation_end || generation_end == generation_start ||
        (size_t)(generation_end - generation_start) >= sizeof(out->generation))
        return invalid;
    memcpy(out->generation, generation_start, (size_t)(generation_end - generation_start));
    out->generation[generation_end - generation_start] = '\0';

    const char *hash_start = generation_end + 1;
    const char *hash_end = strchr(hash_start, '.');
    if (!query_hex_span(hash_start, hash_end, 16))
        return invalid;
    errno = 0;
    char *parsed_end = NULL;
    unsigned long long parsed_hash = strtoull(hash_start, &parsed_end, 16);
    if (errno == ERANGE || parsed_end != hash_end)
        return invalid;
    out->params_hash = (uint64_t)parsed_hash;

    const char *digest_start = hash_end + 1;
    const char *digest_end = strchr(digest_start, '.');
    if (!query_hex_span(digest_start, digest_end, QUERY_CURSOR_DIGEST_HEX_LEN))
        return invalid;
    memcpy(out->result_digest, digest_start, QUERY_CURSOR_DIGEST_HEX_LEN);
    out->result_digest[QUERY_CURSOR_DIGEST_HEX_LEN] = '\0';

    const char *offset_start = digest_end + 1;
    const char *offset_end = strchr(offset_start, '.');
    errno = 0;
    long parsed_offset = offset_end ? strtol(offset_start, &parsed_end, 10) : -1;
    if (!offset_end || offset_end == offset_start || errno == ERANGE || parsed_end != offset_end ||
        parsed_offset <= 0 || parsed_offset > INT_MAX)
        return invalid;
    out->offset = (int)parsed_offset;

    const char *seal_start = offset_end + 1;
    const char *seal_end = token + strlen(token);
    if (!query_hex_span(seal_start, seal_end, 16))
        return invalid;
    char expected_seal[17];
    query_cursor_seal(out, expected_seal);
    if (memcmp(seal_start, expected_seal, 16) != 0)
        return invalid;
    if (out->params_hash != expected_params_hash)
        return "cursor_params_mismatch: this cursor was issued for a different "
               "query/project/graph - pass those arguments unchanged";
    if (strcmp(out->generation, current_generation) != 0)
        return "stale_cursor: the project was reindexed since this cursor was issued - re-run "
               "the original query without 'cursor'";
    return NULL;
}

static bool query_semantic_prefix_column(const char *column) {
    if (!column)
        return false;
    const char *leaf = strrchr(column, '.');
    leaf = leaf ? leaf + 1 : column;
    return strcmp(leaf, "qualified_name") == 0 || strcmp(leaf, "qn") == 0 ||
           strcmp(leaf, "file") == 0 || strcmp(leaf, "file_path") == 0 ||
           strcmp(leaf, "path") == 0 || strcmp(leaf, "root_path") == 0 ||
           strcmp(leaf, "source_file") == 0;
}

static const char *query_truncation_reason(const cbm_cypher_result_t *result, bool page_limit_hit) {
    if (page_limit_hit)
        return "page_limit";
    if (result->truncated)
        return "engine_limit";
    return NULL;
}

static char *query_tree_response(const cbm_cypher_result_t *result, int row_offset, int row_count,
                                 bool exact_total, const query_cursor_context_t *cursor_context) {
    bool materialized_more = row_offset + row_count < result->row_count;
    bool truncated = materialized_more || result->truncated;
    cbm_sb_t sb;
    cbm_sb_init(&sb);
    bool cell_count_safe = row_count > 0 && result->col_count > 0 &&
                           (size_t)row_count <= SIZE_MAX / (size_t)result->col_count;
    size_t cell_count = cell_count_safe ? (size_t)row_count * (size_t)result->col_count : 0;
    cell_count_safe = cell_count_safe && cell_count <= SIZE_MAX / sizeof(const char *);
    const char **cells = cell_count_safe ? malloc(cell_count * sizeof(*cells)) : NULL;
    bool *prefix_cols = cells ? calloc((size_t)result->col_count, sizeof(*prefix_cols)) : NULL;
    if (cells && prefix_cols) {
        for (int row = 0; row < row_count; row++)
            for (int col = 0; col < result->col_count; col++)
                cells[(size_t)row * (size_t)result->col_count + (size_t)col] =
                    result->rows[row_offset + row][col];
        for (int col = 0; col < result->col_count; col++)
            prefix_cols[col] = query_semantic_prefix_column(result->columns[col]);
        cbm_tree_table_rows_profiled(&sb, "rows", row_count, (const char *const *)result->columns,
                                     result->col_count, cells, NULL, prefix_cols);
    } else {
        cbm_tree_table_header(&sb, "rows", row_count, (const char *const *)result->columns,
                              result->col_count);
        for (int row = 0; row < row_count; row++) {
            cbm_tree_row_begin(&sb);
            for (int col = 0; col < result->col_count; col++)
                cbm_tree_cell_str(&sb, result->rows[row_offset + row][col], col == 0);
            cbm_tree_row_end(&sb);
        }
    }
    free(prefix_cols);
    free(cells);
    cbm_tree_scalar_int(&sb, "returned", row_count);
    cbm_tree_scalar_int(&sb, "total", result->row_count);
    cbm_tree_scalar_str(&sb, "total_relation", exact_total ? "eq" : "gte");
    if (row_offset > 0)
        cbm_tree_scalar_int(&sb, "offset", row_offset);
    cbm_tree_scalar_bool(&sb, "has_more", truncated);
    cbm_tree_scalar_bool(&sb, "truncated", truncated);
    const char *reason = query_truncation_reason(result, materialized_more);
    if (reason)
        cbm_tree_scalar_str(&sb, "truncation_reason", reason);
    if (row_count > 0 && materialized_more) {
        char next_cursor[QUERY_CURSOR_TOKEN_CAP];
        if (query_cursor_encode(cursor_context, row_offset + row_count, next_cursor))
            cbm_tree_scalar_str(&sb, "next_cursor", next_cursor);
        cbm_tree_scalar_int(&sb, "next_offset", row_offset + row_count);
    }
    if (result->warning)
        cbm_tree_scalar_str(&sb, "warning", result->warning);
    if (result->row_count == 0 && !result->truncated)
        cbm_tree_scalar_str(&sb, "hint",
                            "Query returned no results. Use get_graph_schema() to see "
                            "available labels and edge types.");
    return cbm_sb_finish(&sb);
}

static char *query_json_response(const cbm_cypher_result_t *result, int row_offset, int row_count,
                                 bool exact_total, const query_cursor_context_t *cursor_context) {
    bool materialized_more = row_offset + row_count < result->row_count;
    bool truncated = materialized_more || result->truncated;
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!root) {
        if (doc)
            yyjson_mut_doc_free(doc);
        return NULL;
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_val *columns = yyjson_mut_arr(doc);
    yyjson_mut_val *rows = yyjson_mut_arr(doc);
    for (int c = 0; c < result->col_count; ++c)
        yyjson_mut_arr_add_str(doc, columns, result->columns[c]);
    for (int r = 0; r < row_count; ++r) {
        yyjson_mut_val *row = yyjson_mut_arr(doc);
        for (int c = 0; c < result->col_count; ++c)
            yyjson_mut_arr_add_str(doc, row, result->rows[row_offset + r][c]);
        yyjson_mut_arr_add_val(rows, row);
    }
    yyjson_mut_obj_add_val(doc, root, "columns", columns);
    yyjson_mut_obj_add_val(doc, root, "rows", rows);
    yyjson_mut_obj_add_int(doc, root, "returned", row_count);
    yyjson_mut_obj_add_int(doc, root, "total", result->row_count);
    yyjson_mut_obj_add_str(doc, root, "total_relation", exact_total ? "eq" : "gte");
    if (row_offset > 0)
        yyjson_mut_obj_add_int(doc, root, "offset", row_offset);
    yyjson_mut_obj_add_bool(doc, root, "has_more", truncated);
    yyjson_mut_obj_add_bool(doc, root, "truncated", truncated);
    const char *reason = query_truncation_reason(result, materialized_more);
    if (reason)
        yyjson_mut_obj_add_str(doc, root, "truncation_reason", reason);
    if (row_count > 0 && materialized_more) {
        char next_cursor[QUERY_CURSOR_TOKEN_CAP];
        if (query_cursor_encode(cursor_context, row_offset + row_count, next_cursor))
            yyjson_mut_obj_add_strcpy(doc, root, "next_cursor", next_cursor);
        yyjson_mut_obj_add_int(doc, root, "next_offset", row_offset + row_count);
    }
    if (result->warning)
        yyjson_mut_obj_add_str(doc, root, "warning", result->warning);
    if (result->row_count == 0 && !result->truncated)
        yyjson_mut_obj_add_str(doc, root, "hint",
                               "Query returned no results. Use get_graph_schema() to see "
                               "available labels and edge types.");
    char *json = cbm_operation_json_write(doc);
    yyjson_mut_doc_free(doc);
    return json;
}

cbm_operation_result_t cbm_query_operation_execute(const char *args_json) {
    char *query = string_arg(args_json, "query");
    char *project = project_arg(args_json);
    /* `max_rows` is a presentation budget, never an evaluation budget: the
     * engine always evaluates the whole query (ORDER BY, DISTINCT, aggregation
     * and SKIP semantics must not see a truncated input). Zero keeps the legacy
     * maximum. */
    int visible_limit = int_arg(args_json, "max_rows", QUERY_DEFAULT_VISIBLE_ROWS);
    if (visible_limit == 0 || visible_limit > QUERY_MAX_VISIBLE_ROWS)
        visible_limit = QUERY_MAX_VISIBLE_ROWS;
    else if (visible_limit < 0)
        visible_limit = 1;
    int requested_offset = int_arg(args_json, "offset", 0);
    int row_offset = requested_offset < 0 ? 0 : requested_offset;
    char *cursor_arg = string_arg(args_json, "cursor");
    char *graph = string_arg(args_json, "graph");
    bool missed_graph = graph && strcmp(graph, "missed") == 0;
    cbm_operation_arg_free(graph);

    if (!query) {
        cbm_operation_arg_free(cursor_arg);
        cbm_operation_arg_free(project);
        return error_result("query is required");
    }
    if (missed_graph && !project) {
        cbm_operation_arg_free(cursor_arg);
        cbm_operation_arg_free(query);
        return error_result("project is required when graph=\"missed\"");
    }
    if (!project || !project[0]) {
        cbm_operation_arg_free(cursor_arg);
        cbm_operation_arg_free(query);
        cbm_operation_arg_free(project);
        return error_result("project is required");
    }

    cbm_store_open_status_t open_status = CBM_STORE_OPEN_OK;
    cbm_store_t *store = cbm_store_host_open_query(project, &open_status);
    if (!store) {
        cbm_operation_arg_free(cursor_arg);
        cbm_operation_arg_free(query);
        cbm_operation_arg_free(project);
        return error_result(open_status == CBM_STORE_OPEN_CORRUPT
                                ? CBM_STORE_CORRUPT_ERROR
                                : "project not found or not indexed");
    }
    if (cbm_store_count_nodes(store, project) <= 0) {
        cbm_store_close(store);
        cbm_operation_arg_free(cursor_arg);
        cbm_operation_arg_free(query);
        cbm_operation_arg_free(project);
        return error_result("project not indexed or index is empty");
    }

    char generation[QUERY_GENERATION_CAP] = "";
    if (cbm_store_generation(store, generation, sizeof(generation)) != CBM_STORE_OK) {
        cbm_store_close(store);
        cbm_operation_arg_free(cursor_arg);
        cbm_operation_arg_free(query);
        cbm_operation_arg_free(project);
        return error_result(
            "index_metadata_error: generation metadata is unreadable; reindex before querying");
    }
    bool generation_available = generation[0] != '\0';
    uint64_t params_hash = query_params_hash(project, query, missed_graph ? "missed" : "code");
    query_cursor_t cursor = {0};
    if (cursor_arg) {
        const char *cursor_error = NULL;
        if (requested_offset != 0)
            cursor_error = "invalid_params: cursor cannot be combined with a nonzero offset";
        else if (!generation_available || strcmp(generation, "legacy") == 0)
            cursor_error = "cursor_unsupported: this index predates generation tracking; reindex "
                           "before using snapshot pagination";
        else
            cursor_error = query_cursor_decode(cursor_arg, generation, params_hash, &cursor);
        if (cursor_error) {
            cbm_operation_result_t error = error_result(cursor_error);
            cbm_store_close(store);
            cbm_operation_arg_free(cursor_arg);
            cbm_operation_arg_free(query);
            cbm_operation_arg_free(project);
            return error;
        }
        row_offset = cursor.offset;
    }

    char coverage_project[512];
    const char *cypher_project = project;
    if (missed_graph) {
        cbm_store_coverage_shadow_project(coverage_project, sizeof(coverage_project), project);
        cypher_project = coverage_project;
    }

    cbm_cypher_result_t result = {0};
    int rc = cbm_cypher_execute(store, query, cypher_project, 0, &result);
    if (rc < 0) {
        cbm_operation_result_t error =
            error_result(result.error ? result.error : "query execution failed");
        cbm_cypher_result_free(&result);
        cbm_store_close(store);
        cbm_operation_arg_free(cursor_arg);
        cbm_operation_arg_free(query);
        cbm_operation_arg_free(project);
        return error;
    }

    char result_digest[CBM_SHA256_HEX_LEN + 1];
    query_result_digest(&result, result_digest);
    if (cursor_arg &&
        strncmp(cursor.result_digest, result_digest, QUERY_CURSOR_DIGEST_HEX_LEN) != 0) {
        cbm_operation_result_t error = error_result(
            "stale_cursor: the complete query materialization changed since this cursor was "
            "issued; re-run the original query without 'cursor'");
        cbm_cypher_result_free(&result);
        cbm_store_close(store);
        cbm_operation_arg_free(cursor_arg);
        cbm_operation_arg_free(query);
        cbm_operation_arg_free(project);
        return error;
    }
    query_cursor_context_t cursor_context = {
        .generation = generation,
        .params_hash = params_hash,
        .result_digest = result_digest,
        .can_mint = generation_available && strcmp(generation, "legacy") != 0,
    };
    int materialized_rows = row_offset < result.row_count ? result.row_count - row_offset : 0;
    int row_count = materialized_rows < visible_limit ? materialized_rows : visible_limit;
    bool exact_total = !result.truncated;

    char *format = string_arg(args_json, "format");
    bool legacy_json = format && strcmp(format, "json") == 0;
    cbm_operation_arg_free(format);

    char *payload =
        legacy_json
            ? query_json_response(&result, row_offset, row_count, exact_total, &cursor_context)
            : query_tree_response(&result, row_offset, row_count, exact_total, &cursor_context);

    cbm_cypher_result_free(&result);
    cbm_store_close(store);
    cbm_operation_arg_free(cursor_arg);
    cbm_operation_arg_free(query);
    cbm_operation_arg_free(project);
    return payload ? cbm_operation_result_take(payload, false) : error_result("out of memory");
}
