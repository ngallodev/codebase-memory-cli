#include "operations/result_wire.h"
#include "operations/json_args.h"
#include "operations/search.h"
#include "operations/compact_out.h"
#include "operations/output_budget.h"
#include "foundation/mem_core.h"
#include "operations/store_host.h"

#include "foundation/constants.h"
#include "foundation/platform.h"
#include "foundation/str_util.h"
#include "sqlite3/sqlite3.h"
#include "store/store.h"
#include "yyjson/yyjson.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    SEARCH_DEFAULT_LIMIT = 50,
    SEARCH_MAX_LIMIT = 1000,
    BM25_QUERY_BUFFER = 1024,
    BM25_INNER_LIMIT = 2000,
    BM25_COL_QN = 3,
    BM25_COL_LABEL = 1,
    BM25_COL_FILE = 4,
    BM25_COL_START = 5,
    BM25_COL_END = 6,
    BM25_COL_RANK = 7,
    SEARCH_MAX_FIELDS = 12,
    SEARCH_MAX_SEMANTIC_KEYWORDS = 32,
    SEARCH_SEMANTIC_MAX_LIMIT = 500,
    /* Vector ranking is resource-bounded: no continuation past this offset. */
    SEARCH_SEMANTIC_MAX_OFFSET = 99998,
};

#define BM25_WEIGHTS "bm25(nodes_fts, 1.0, 1.0, 1.0, 1.0, 0.3)"

static sqlite3_destructor_type transient_destructor(void) {
    static const volatile intptr_t raw = -1;
    sqlite3_destructor_type destructor = NULL;
    memcpy(&destructor, (const void *)&raw, sizeof(destructor));
    return destructor;
}

static char *copy_text(const char *text) {
    if (!text)
        return NULL;
    return cbm_mem_strdup(CBM_MEM_CLASS_OPERATION_ARG, text);
}

static yyjson_doc *read_args(const char *args) {
    return args ? yyjson_read(args, strlen(args), 0) : NULL;
}

static char *string_arg(const char *args, const char *name) {
    yyjson_doc *doc = read_args(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    char *result = value && yyjson_is_str(value) ? copy_text(yyjson_get_str(value)) : NULL;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static int int_arg(const char *args, const char *name, int fallback) {
    yyjson_doc *doc = read_args(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    int result = value && yyjson_is_int(value) ? (int)yyjson_get_sint(value) : fallback;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static bool bool_arg(const char *args, const char *name) {
    yyjson_doc *doc = read_args(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    bool result = value && yyjson_is_bool(value) && yyjson_get_bool(value);
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static bool search_arg_present(const char *args, const char *name) {
    yyjson_doc *doc = read_args(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    bool present = yyjson_is_obj(root) && yyjson_obj_get(root, name) != NULL;
    if (doc)
        yyjson_doc_free(doc);
    return present;
}

static cbm_operation_result_t json_result(yyjson_mut_doc *doc, bool error) {
    if (!doc)
        return cbm_operation_result_copy("{\"error\":\"result allocation failed\"}", true);
    char *json = cbm_operation_json_write(doc);
    yyjson_mut_doc_free(doc);
    return json ? cbm_operation_result_take(json, error)
                : cbm_operation_result_copy("{\"error\":\"result encoding failed\"}", true);
}

static cbm_operation_result_t error_result(const char *message, const char *hint) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root) {
        if (doc)
            yyjson_mut_doc_free(doc);
        return cbm_operation_result_copy(message ? message : "search failed", true);
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "error", message ? message : "search failed");
    if (hint)
        yyjson_mut_obj_add_strcpy(doc, root, "hint", hint);
    return json_result(doc, true);
}

static bool valid_relationship(const char *value) {
    if (!value || !value[0])
        return false;
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p)
        if (!((*p >= 'A' && *p <= 'Z') || *p == '_'))
            return false;
    return true;
}

static int build_match(const char *query, char *output, size_t output_size) {
    if (!query || !output || output_size < 2U)
        return 0;
    size_t position = 0U;
    int tokens = 0;
    const char *p = query;
    while (*p) {
        while (*p && !((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                       (*p >= '0' && *p <= '9') || *p == '_'))
            ++p;
        if (!*p)
            break;
        const char *start = p;
        while (*p && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                      (*p >= '0' && *p <= '9') || *p == '_'))
            ++p;
        size_t length = (size_t)(p - start);
        const char *separator = tokens ? " OR " : "";
        size_t separator_length = strlen(separator);
        if (position + separator_length + length + 1U >= output_size)
            break;
        memcpy(output + position, separator, separator_length);
        position += separator_length;
        memcpy(output + position, start, length);
        position += length;
        ++tokens;
    }
    output[position] = '\0';
    return tokens;
}

static char *file_pattern_like(const char *pattern) {
    if (!pattern)
        return NULL;
    char *glob = cbm_glob_to_like(pattern);
    char *like = glob ? cbm_mem_strdup(CBM_MEM_CLASS_OTHER, glob) : NULL;
    free(glob);
    if (!like || strchr(pattern, '*') || strchr(pattern, '?'))
        return like;
    size_t length = strlen(like);
    char *contains = cbm_alloc(CBM_MEM_CLASS_OTHER, length + 3U);
    if (!contains)
        return like;
    contains[0] = '%';
    memcpy(contains + 1U, like, length);
    contains[length + 1U] = '%';
    contains[length + 2U] = '\0';
    cbm_free(CBM_MEM_CLASS_OTHER, like);
    return contains;
}

static cbm_store_t *open_indexed_project(const char *project,
                                         cbm_store_open_status_t *open_status) {
    if (!project || !cbm_validate_project_name(project))
        return NULL;
    const char *cache_dir = cbm_resolve_cache_dir();
    if (!cache_dir)
        return NULL;

    char db_path[CBM_SZ_2K];
    int n = snprintf(db_path, sizeof(db_path), "%s/%s.db", cache_dir, project);
    if (n <= 0 || (size_t)n >= sizeof(db_path))
        return NULL;

    cbm_store_t *store = cbm_store_host_open_query_path(db_path, open_status);
    if (!store)
        return NULL;
    cbm_project_t indexed = {0};
    if (cbm_store_get_project(store, project, &indexed) != CBM_STORE_OK) {
        cbm_store_close(store);
        return NULL;
    }
    cbm_project_free_fields(&indexed);
    return store;
}

static char *ranked_tree(const char *json) {
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *rows = yyjson_obj_get(root, "rows");
    yyjson_val *columns = yyjson_obj_get(root, "cols");
    int ncols = (int)yyjson_arr_size(columns);
    int nrows = (int)yyjson_arr_size(rows);
    const char *cols[5];
    const bool strings[] = {true, true, true, true, false};
    const bool prefixes[] = {true, false, true, false, false};
    if (ncols != 5) {
        if (doc)
            yyjson_doc_free(doc);
        return NULL;
    }
    size_t count = (size_t)nrows * 5U;
    const char **cells = cbm_calloc(CBM_MEM_CLASS_OTHER, count * sizeof(*cells));
    char (*numbers)[32] = cbm_calloc(CBM_MEM_CLASS_OTHER, (size_t)nrows * sizeof(*numbers));
    if (!cells || !numbers) {
        cbm_free(CBM_MEM_CLASS_OTHER, cells);
        cbm_free(CBM_MEM_CLASS_OTHER, numbers);
        yyjson_doc_free(doc);
        return NULL;
    }
    for (int c = 0; c < 5; ++c)
        cols[c] = yyjson_get_str(yyjson_arr_get(columns, (size_t)c));
    for (int r = 0; r < nrows; ++r) {
        yyjson_val *row = yyjson_arr_get(rows, (size_t)r);
        for (int c = 0; c < 4; ++c)
            cells[(size_t)r * 5U + (size_t)c] = yyjson_get_str(yyjson_arr_get(row, (size_t)c));
        snprintf(numbers[r], sizeof(numbers[r]), "%.17g", yyjson_get_num(yyjson_arr_get(row, 4)));
        cells[(size_t)r * 5U + 4U] = numbers[r];
    }
    cbm_sb_t sb;
    cbm_sb_init(&sb);
    cbm_tree_table_rows_profiled(&sb, "results", nrows, cols, 5, cells, strings, prefixes);
    cbm_free(CBM_MEM_CLASS_OTHER, cells);
    cbm_free(CBM_MEM_CLASS_OTHER, numbers);
    yyjson_mut_doc *metadata_doc = yyjson_doc_mut_copy(doc, NULL);
    yyjson_doc_free(doc);
    if (!metadata_doc) {
        cbm_sb_free(&sb);
        return NULL;
    }
    yyjson_mut_val *metadata_root = yyjson_mut_doc_get_root(metadata_doc);
    yyjson_mut_obj_remove_key(metadata_root, "rows");
    yyjson_mut_obj_remove_key(metadata_root, "cols");
    cbm_operation_result_t metadata_result = json_result(metadata_doc, false);
    char *metadata = metadata_result.payload ? cbm_json_to_tree(metadata_result.payload) : NULL;
    cbm_operation_result_dispose(&metadata_result);
    if (!metadata) {
        cbm_sb_free(&sb);
        return NULL;
    }
    cbm_sb_append(&sb, metadata);
    free(metadata);
    return cbm_sb_finish(&sb);
}

static cbm_operation_result_t bm25_search(cbm_store_t *store, const char *project,
                                          const char *query, const char *file_pattern,
                                          const char *label_filter, int limit, int offset,
                                          const char *args) {
    sqlite3 *db = cbm_store_get_db(store);
    char match[BM25_QUERY_BUFFER];
    if (!db || build_match(query, match, sizeof(match)) == 0)
        return cbm_operation_result_copy("", true);
    char *file_like = file_pattern_like(file_pattern);
    /* Exact-name tier: a definition whose NAME is the query outranks every
     * partial hit. BM25 term frequency otherwise rewards a long test-method
     * name that repeats the token over the class itself, and the label tiers
     * then push that class's own methods above it. The definition asked for by
     * name comes first, a case-insensitive exact spelling next, and everything
     * else keeps its BM25 order. */
    const char *sql =
        "SELECT n.id,n.label,n.name,n.qualified_name,n.file_path,n.start_line,n.end_line,"
        "(fts.base_rank-CASE WHEN n.name=?8 THEN 30.0 "
        "WHEN lower(n.name)=lower(?8) THEN 20.0 ELSE 0.0 END"
        "-CASE WHEN n.label IN ('Function','Method') THEN 10.0 "
        "WHEN n.label='Route' THEN 8.0 WHEN n.label IN (" CBM_SQL_TYPE_LIKE_LABELS ") THEN 5.0 "
        "WHEN n.label IN (" CBM_SQL_RELATION_LABELS ") THEN 5.0 ELSE 0.0 END) AS rank "
        "FROM (SELECT rowid," BM25_WEIGHTS " AS base_rank FROM nodes_fts "
        "WHERE nodes_fts MATCH ?1 ORDER BY base_rank, rowid LIMIT ?5) fts "
        "JOIN nodes n ON n.id=fts.rowid WHERE n.project=?2 "
        "AND n.label NOT IN ('File','Folder','Variable','Project') "
        "AND (?6 IS NULL OR n.file_path LIKE ?6) "
        /* The label filter applies in query mode exactly as in structural mode.
         * MIRRORED in the count query below. */
        "AND (?7 IS NULL OR n.label=?7) ORDER BY rank,n.id LIMIT ?3 OFFSET ?4";
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
        cbm_free(CBM_MEM_CLASS_OTHER, file_like);
        return error_result("full-text search unavailable",
                            "Re-index the project or use structural search flags.");
    }
    sqlite3_destructor_type destructor = transient_destructor();
    sqlite3_bind_text(statement, 1, match, -1, destructor);
    sqlite3_bind_text(statement, 2, project, -1, destructor);
    sqlite3_bind_int(statement, 3, limit);
    sqlite3_bind_int(statement, 4, offset);
    sqlite3_bind_int(statement, 5, BM25_INNER_LIMIT);
    if (file_like)
        sqlite3_bind_text(statement, 6, file_like, -1, destructor);
    else
        sqlite3_bind_null(statement, 6);
    if (label_filter && label_filter[0])
        sqlite3_bind_text(statement, 7, label_filter, -1, destructor);
    else
        sqlite3_bind_null(statement, 7);
    sqlite3_bind_text(statement, 8, query, -1, destructor);

    int total = 0;
    const char *count_sql =
        "SELECT COUNT(*) FROM (SELECT fts.rowid FROM (SELECT rowid FROM nodes_fts "
        "WHERE nodes_fts MATCH ?1 ORDER BY " BM25_WEIGHTS " LIMIT ?3) fts "
        "JOIN nodes n ON n.id=fts.rowid WHERE n.project=?2 "
        "AND n.label NOT IN ('File','Folder','Variable','Project') "
        "AND (?6 IS NULL OR n.file_path LIKE ?6) "
        "AND (?7 IS NULL OR n.label=?7))";
    sqlite3_stmt *counter = NULL;
    if (sqlite3_prepare_v2(db, count_sql, -1, &counter, NULL) == SQLITE_OK) {
        sqlite3_bind_text(counter, 1, match, -1, destructor);
        sqlite3_bind_text(counter, 2, project, -1, destructor);
        sqlite3_bind_int(counter, 3, BM25_INNER_LIMIT);
        if (file_like)
            sqlite3_bind_text(counter, 6, file_like, -1, destructor);
        else
            sqlite3_bind_null(counter, 6);
        if (label_filter && label_filter[0])
            sqlite3_bind_text(counter, 7, label_filter, -1, destructor);
        else
            sqlite3_bind_null(counter, 7);
        if (sqlite3_step(counter) == SQLITE_ROW)
            total = sqlite3_column_int(counter, 0);
        sqlite3_finalize(counter);
    }

    /* The top-candidate window is a performance ceiling, not an exact-total
     * boundary. Probe one candidate beyond it so a broad query never presents a
     * window-local count as the complete match count. The probe is global to
     * the FTS table, so saturation is reported conservatively even when later
     * project/path filters might discard the hidden candidates. */
    bool candidate_window_saturated = true;
    sqlite3_stmt *probe = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT rowid FROM nodes_fts WHERE nodes_fts MATCH ?1 "
                           "ORDER BY bm25(nodes_fts), rowid LIMIT 1 OFFSET ?2",
                           -1, &probe, NULL) == SQLITE_OK) {
        sqlite3_bind_text(probe, 1, match, -1, destructor);
        sqlite3_bind_int(probe, 2, BM25_INNER_LIMIT);
        candidate_window_saturated = sqlite3_step(probe) != SQLITE_DONE;
        sqlite3_finalize(probe);
    }

    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    yyjson_mut_val *columns = doc ? yyjson_mut_arr(doc) : NULL;
    yyjson_mut_val *rows = doc ? yyjson_mut_arr(doc) : NULL;
    if (!doc || !root || !columns || !rows) {
        if (doc)
            yyjson_mut_doc_free(doc);
        sqlite3_finalize(statement);
        cbm_free(CBM_MEM_CLASS_OTHER, file_like);
        return error_result("result allocation failed", NULL);
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_int(doc, root, "total", total);
    yyjson_mut_obj_add_str(doc, root, "total_relation", candidate_window_saturated ? "gte" : "eq");
    if (candidate_window_saturated)
        yyjson_mut_obj_add_bool(doc, root, "candidate_window_saturated", true);
    yyjson_mut_obj_add_str(doc, root, "search_mode", "bm25");
    static const char *const names[] = {"qn", "label", "file", "lines", "rank"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        yyjson_mut_arr_add_str(doc, columns, names[i]);
    yyjson_mut_obj_add_val(doc, root, "cols", columns);
    int emitted = 0;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        char lines[32] = "";
        int start = sqlite3_column_int(statement, BM25_COL_START);
        int end = sqlite3_column_int(statement, BM25_COL_END);
        if (start > 0)
            (void)snprintf(lines, sizeof(lines), "%d-%d", start, end > start ? end : start);
        yyjson_mut_val *row = yyjson_mut_arr(doc);
        const unsigned char *qn = sqlite3_column_text(statement, BM25_COL_QN);
        const unsigned char *label = sqlite3_column_text(statement, BM25_COL_LABEL);
        const unsigned char *file = sqlite3_column_text(statement, BM25_COL_FILE);
        yyjson_mut_arr_add_strcpy(doc, row, qn ? (const char *)qn : "");
        yyjson_mut_arr_add_strcpy(doc, row, label ? (const char *)label : "");
        yyjson_mut_arr_add_strcpy(doc, row, file ? (const char *)file : "");
        yyjson_mut_arr_add_strcpy(doc, row, lines);
        yyjson_mut_arr_add_real(doc, row, sqlite3_column_double(statement, BM25_COL_RANK));
        yyjson_mut_arr_add_val(rows, row);
        ++emitted;
    }
    sqlite3_finalize(statement);
    cbm_free(CBM_MEM_CLASS_OTHER, file_like);
    yyjson_mut_obj_add_val(doc, root, "rows", rows);
    bool has_more = total > offset + emitted;
    yyjson_mut_obj_add_int(doc, root, "returned", emitted);
    yyjson_mut_obj_add_bool(doc, root, "has_more", has_more);
    if (has_more && emitted > 0)
        yyjson_mut_obj_add_int(doc, root, "next_offset", offset + emitted);
    bool truncated = has_more || candidate_window_saturated;
    yyjson_mut_obj_add_bool(doc, root, "truncated", truncated);
    if (truncated)
        yyjson_mut_obj_add_str(doc, root, "truncation_reason",
                               has_more ? "page_limit" : "candidate_window");
    char *format = string_arg(args, "format");
    bool json_format = format && strcmp(format, "json") == 0;
    cbm_operation_arg_free(format);
    size_t ceiling = cbm_output_budget_bytes(cbm_output_budget_tokens(args, 0));
    char *payload = NULL;
    for (;;) {
        yyjson_mut_doc *encoding = yyjson_mut_doc_new(NULL);
        if (encoding)
            yyjson_mut_doc_set_root(encoding, yyjson_mut_val_mut_copy(encoding, root));
        char *json = json_format ? encoding ? cbm_operation_json_write(encoding) : NULL
                                 : yyjson_mut_write(doc, 0, NULL);
        if (encoding)
            yyjson_mut_doc_free(encoding);
        payload = json_format ? json : json ? ranked_tree(json) : NULL;
        if (!json_format)
            free(json);
        if (!payload || !ceiling || strlen(payload) <= ceiling)
            break;
        cbm_operation_result_t oversized = cbm_operation_result_take(payload, false);
        cbm_operation_result_dispose(&oversized);
        payload = NULL;
        if (emitted == 0)
            break;
        yyjson_mut_arr_remove_last(rows);
        --emitted;
        yyjson_mut_obj_remove_key(root, "returned");
        yyjson_mut_obj_remove_key(root, "has_more");
        yyjson_mut_obj_remove_key(root, "next_offset");
        yyjson_mut_obj_remove_key(root, "truncated");
        yyjson_mut_obj_remove_key(root, "truncation_reason");
        yyjson_mut_obj_add_int(doc, root, "returned", emitted);
        yyjson_mut_obj_add_bool(doc, root, "has_more", true);
        yyjson_mut_obj_add_int(doc, root, "next_offset", offset + emitted);
        yyjson_mut_obj_add_bool(doc, root, "truncated", true);
        yyjson_mut_obj_add_str(doc, root, "truncation_reason", "output_budget");
    }
    yyjson_mut_doc_free(doc);
    return payload ? cbm_operation_result_take(payload, false)
                   : error_result("max_output_tokens is too small for search metadata", NULL);
}

static bool blocked_field(const char *field) {
    return !field || !field[0] || strcmp(field, "fp") == 0 || strcmp(field, "sp") == 0 ||
           strcmp(field, "bt") == 0;
}

static bool core_field(const char *field) {
    return strcmp(field, "qn") == 0 || strcmp(field, "qualified_name") == 0 ||
           strcmp(field, "name") == 0 || strcmp(field, "label") == 0 ||
           strcmp(field, "file") == 0 || strcmp(field, "file_path") == 0 ||
           strcmp(field, "path") == 0 || strcmp(field, "lines") == 0 || strcmp(field, "in") == 0 ||
           strcmp(field, "out") == 0;
}

static int parse_fields(const char *args, const char **fields, yyjson_doc **owner,
                        bool *core_requested) {
    *owner = NULL;
    *core_requested = false;
    yyjson_doc *doc = read_args(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *array = yyjson_is_obj(root) ? yyjson_obj_get(root, "fields") : NULL;
    if (!yyjson_is_arr(array)) {
        if (doc)
            yyjson_doc_free(doc);
        return 0;
    }
    int count = 0;
    size_t index, max;
    yyjson_val *value;
    yyjson_arr_foreach(array, index, max, value) {
        const char *field = yyjson_is_str(value) ? yyjson_get_str(value) : NULL;
        if (blocked_field(field))
            continue;
        if (core_field(field)) {
            *core_requested = true;
            continue;
        }
        if (count < SEARCH_MAX_FIELDS)
            fields[count++] = field;
    }
    if (count == 0) {
        yyjson_doc_free(doc);
        return 0;
    }
    *owner = doc;
    return count;
}

static int result_qn_cmp(const void *left, const void *right) {
    const cbm_search_result_t *a = left;
    const cbm_search_result_t *b = right;
    const char *aq = a->node.qualified_name ? a->node.qualified_name : "";
    const char *bq = b->node.qualified_name ? b->node.qualified_name : "";
    return strcmp(aq, bq);
}

static size_t prefix_length(const char *qn) {
    const char *last = qn ? strrchr(qn, '.') : NULL;
    return last ? (size_t)(last - qn) : 0U;
}

static void sg_toon_property_cell(cbm_sb_t *sb, yyjson_val *v) {
    if (v && yyjson_is_str(v)) {
        cbm_tree_cell_str(sb, yyjson_get_str(v), false);
    } else if (v && yyjson_is_bool(v)) {
        cbm_tree_cell_bool(sb, yyjson_get_bool(v), false);
    } else if (v && yyjson_is_int(v)) {
        cbm_tree_cell_int(sb, yyjson_get_int(v), false);
    } else if (v && yyjson_is_real(v)) {
        cbm_tree_cell_real(sb, yyjson_get_real(v), false);
    } else if (v && !yyjson_is_null(v)) {
        char *json = yyjson_val_write(v, 0, NULL);
        cbm_tree_cell_str(sb, json ? json : "", false);
        free(json);
    } else {
        cbm_tree_cell_str(sb, "", false);
    }
}

/* "start-end" line range, or empty when the node carries no line info. */
static void sg_lines_str(char *out, size_t sz, int start, int end) {
    if (start > 0) {
        snprintf(out, sz, "%d-%d", start, end > start ? end : start);
    } else {
        out[0] = '\0';
    }
}

static void emit_search_results_grouped_tree(cbm_sb_t *sb, const cbm_search_output_t *out,
                                             const char *const *fields, int nfields,

                                             bool include_connected, int returned) {
    cbm_sb_append(sb, "results: ");
    char count_buf[CBM_SZ_32];
    snprintf(count_buf, sizeof(count_buf), "%d", returned);
    cbm_sb_append(sb, count_buf);
    cbm_sb_append(sb, "  (rows: name label lines in out");
    for (int f = 0; f < nfields; f++) {
        cbm_sb_append(sb, " ");
        cbm_sb_append(sb, fields[f]);
    }
    if (include_connected) {
        cbm_sb_append(sb, " connected");
    }
    cbm_sb_append(sb, "; group prefix \"-\" means empty; "
                      "qn = prefix empty ? name : prefix + \".\" + name)\n");

    char *previous_prefix = NULL;
    char *previous_file = NULL;
    for (int i = 0; i < returned; i++) {
        const cbm_search_result_t *sr = &out->results[i];
        const char *qn = sr->node.qualified_name ? sr->node.qualified_name : "";
        const char *file = sr->node.file_path ? sr->node.file_path : "";
        size_t plen = prefix_length(qn);
        bool same_group = previous_prefix && strlen(previous_prefix) == plen &&
                          memcmp(previous_prefix, qn, plen) == 0 && previous_file &&
                          strcmp(previous_file, file) == 0;
        if (!same_group) {
            cbm_free(CBM_MEM_CLASS_OTHER, previous_prefix);
            cbm_free(CBM_MEM_CLASS_OTHER, previous_file);
            previous_prefix = cbm_alloc(CBM_MEM_CLASS_OTHER, plen + 1U);
            if (previous_prefix) {
                memcpy(previous_prefix, qn, plen);
                previous_prefix[plen] = '\0';
            }
            previous_file = cbm_mem_strdup(CBM_MEM_CLASS_OTHER, file);
            if (!previous_prefix || !previous_file) {
                sb->oom = true;
                break;
            }
            cbm_tree_cell_str(sb, previous_prefix, true);
            cbm_sb_append(sb, " (");
            cbm_tree_cell_str(sb, previous_file, true);
            cbm_sb_append(sb, "):\n");
        }
        const char *shortname = plen ? qn + plen + 1 : qn;
        char lines[CBM_SZ_32];
        sg_lines_str(lines, sizeof(lines), sr->node.start_line, sr->node.end_line);
        cbm_sb_append(sb, "  ");
        cbm_tree_cell_str(sb, shortname, true);
        cbm_tree_cell_str(sb, sr->node.label ? sr->node.label : "", false);
        cbm_tree_cell_str(sb, lines, false);
        cbm_tree_cell_int(sb, sr->in_degree, false);
        cbm_tree_cell_int(sb, sr->out_degree, false);
        /* Extra property columns (fields param). Routed through the shared
         * cell emitters so values with spaces (signatures, docstrings) are
         * QUOTED — a raw append would shift every following column. Missing
         * values emit as "-" (the emitter's empty-cell placeholder). */
        if (nfields > 0) {
            yyjson_doc *pd =
                (sr->node.properties_json && sr->node.properties_json[0])
                    ? yyjson_read(sr->node.properties_json, strlen(sr->node.properties_json), 0)
                    : NULL;
            yyjson_val *pr = pd ? yyjson_doc_get_root(pd) : NULL;
            for (int f = 0; f < nfields; f++) {
                yyjson_val *v = (pr && yyjson_is_obj(pr)) ? yyjson_obj_get(pr, fields[f]) : NULL;
                sg_toon_property_cell(sb, v);
            }
            if (pd) {
                yyjson_doc_free(pd);
            }
        }
        if (include_connected && sr->node.id > 0) {
            cbm_sb_t connected;
            cbm_sb_init(&connected);
            for (int c = 0; c < sr->connected_count; ++c) {
                if (c)
                    cbm_sb_append(&connected, ",");
                cbm_sb_append(&connected, sr->connected_names[c]);
            }
            if (connected.oom)
                sb->oom = true;
            cbm_tree_cell_str(sb, connected.buf ? connected.buf : "", false);
            cbm_sb_free(&connected);
        }
        cbm_sb_append(sb, "\n");
    }
    cbm_free(CBM_MEM_CLASS_OTHER, previous_prefix);
    cbm_free(CBM_MEM_CLASS_OTHER, previous_file);
}

/* The grouped search shape is excellent when rows share a module/file, but its
 * reconstruction rule and group headers are pure overhead for a singleton or
 * a scatter of unrelated rows. The common, no-extra-fields path can be
 * represented as a regular table, so render both byte-for-byte and keep the
 * smaller lossless form. Extra property/connected columns retain the grouped
 * emitter because their cells are dynamically typed. */
static void render_search_results_flat_tree(cbm_sb_t *flat, const cbm_search_output_t *out,
                                            int returned) {
    static const char *const cols[] = {"qn", "label", "file", "lines", "in", "out"};
    static const bool string_cols[] = {true, true, true, true, false, false};
    static const bool prefix_cols[] = {true, false, true, false, false, false};
    enum { COLS = 6, CELL_TEXTS = 3 };
    if (returned == 0) {
        cbm_tree_table_header(flat, "results", 0, cols, COLS);
        return;
    }
    if (returned < 0 || (size_t)returned > (size_t)-1 / (COLS * sizeof(char *)) ||
        (size_t)returned > (size_t)-1 / (CELL_TEXTS * CBM_SZ_32)) {
        flat->oom = true;
        return;
    }
    const char **cells = cbm_calloc(CBM_MEM_CLASS_OTHER, (size_t)returned * COLS * sizeof(*cells));
    char *text = cbm_calloc(CBM_MEM_CLASS_OTHER, (size_t)returned * CELL_TEXTS * CBM_SZ_32);
    if (!cells || !text) {
        cbm_free(CBM_MEM_CLASS_OTHER, (void *)cells);
        cbm_free(CBM_MEM_CLASS_OTHER, text);
        flat->oom = true;
        return;
    }
    for (int i = 0; i < returned; i++) {
        const cbm_search_result_t *sr = &out->results[i];
        char *lines = text + (size_t)(i * CELL_TEXTS) * CBM_SZ_32;
        char *in_degree = lines + CBM_SZ_32;
        char *out_degree = in_degree + CBM_SZ_32;
        sg_lines_str(lines, CBM_SZ_32, sr->node.start_line, sr->node.end_line);
        snprintf(in_degree, CBM_SZ_32, "%d", sr->in_degree);
        snprintf(out_degree, CBM_SZ_32, "%d", sr->out_degree);
        cells[(size_t)i * COLS] = sr->node.qualified_name ? sr->node.qualified_name : "";
        cells[(size_t)i * COLS + 1U] = sr->node.label ? sr->node.label : "";
        cells[(size_t)i * COLS + 2U] = sr->node.file_path ? sr->node.file_path : "";
        cells[(size_t)i * COLS + 3U] = lines;
        cells[(size_t)i * COLS + 4U] = in_degree;
        cells[(size_t)i * COLS + 5U] = out_degree;
    }
    cbm_tree_table_rows_profiled(flat, "results", returned, cols, COLS, cells, string_cols,
                                 prefix_cols);
    cbm_free(CBM_MEM_CLASS_OTHER, (void *)cells);
    cbm_free(CBM_MEM_CLASS_OTHER, text);
}

static void emit_search_results_tree(cbm_sb_t *sb, const cbm_search_output_t *out,
                                     const char *const *fields, int nfields, bool include_connected,
                                     int returned) {
    cbm_sb_t grouped;
    cbm_sb_init(&grouped);
    emit_search_results_grouped_tree(&grouped, out, fields, nfields, include_connected, returned);
    cbm_sb_t flat;
    cbm_sb_init(&flat);
    if (nfields == 0 && !include_connected)
        render_search_results_flat_tree(&flat, out, returned);
    const char *grouped_text = grouped.oom ? NULL : grouped.buf;
    const char *flat_text = flat.oom ? NULL : flat.buf;
    if (!flat_text && !grouped_text)
        sb->oom = true;
    if (flat_text && (!grouped_text || strlen(flat_text) < strlen(grouped_text))) {
        cbm_sb_append(sb, flat_text);
    } else if (grouped_text) {
        cbm_sb_append(sb, grouped_text);
    }
    cbm_sb_free(&flat);
    cbm_sb_free(&grouped);
}

static void add_property_value(yyjson_mut_doc *doc, yyjson_mut_val *row, yyjson_val *value) {
    yyjson_mut_val *copy = value && !yyjson_is_null(value) ? yyjson_val_mut_copy(doc, value) : NULL;
    if (copy)
        yyjson_mut_arr_add_val(row, copy);
    else
        yyjson_mut_arr_add_null(doc, row);
}

static void emit_structural(yyjson_mut_doc *doc, yyjson_mut_val *root, cbm_search_output_t *output,
                            int offset, const char **fields, int field_count,
                            bool include_connected) {
    yyjson_mut_obj_add_int(doc, root, "total", output->total);
    yyjson_mut_obj_add_int(doc, root, "count", output->count);
    yyjson_mut_val *columns = yyjson_mut_arr(doc);
    static const char *const base[] = {"name", "label", "lines", "in", "out"};
    for (size_t i = 0; i < sizeof(base) / sizeof(base[0]); ++i)
        yyjson_mut_arr_add_str(doc, columns, base[i]);
    for (int i = 0; i < field_count; ++i)
        yyjson_mut_arr_add_strcpy(doc, columns, fields[i]);
    if (include_connected)
        yyjson_mut_arr_add_str(doc, columns, "connected");
    yyjson_mut_obj_add_val(doc, root, "cols", columns);
    if (output->count > 1)
        qsort(output->results, (size_t)output->count, sizeof(*output->results), result_qn_cmp);
    yyjson_mut_val *groups = yyjson_mut_arr(doc);
    yyjson_mut_val *rows = NULL;
    char current[2048] = "";
    for (int i = 0; i < output->count; ++i) {
        cbm_search_result_t *search = &output->results[i];
        const char *qn = search->node.qualified_name ? search->node.qualified_name : "";
        const char *file = search->node.file_path ? search->node.file_path : "";
        size_t prefix = prefix_length(qn);
        char key[2048];
        (void)snprintf(key, sizeof(key), "%.*s|%s", (int)prefix, qn, file);
        if (!rows || strcmp(key, current) != 0) {
            (void)snprintf(current, sizeof(current), "%s", key);
            yyjson_mut_val *group = yyjson_mut_obj(doc);
            char prefix_text[1024];
            (void)snprintf(prefix_text, sizeof(prefix_text), "%.*s", (int)prefix, qn);
            yyjson_mut_obj_add_strcpy(doc, group, "qn_prefix", prefix_text);
            yyjson_mut_obj_add_strcpy(doc, group, "file", file);
            rows = yyjson_mut_arr(doc);
            yyjson_mut_obj_add_val(doc, group, "rows", rows);
            yyjson_mut_arr_add_val(groups, group);
        }
        char lines[32] = "";
        if (search->node.start_line > 0)
            (void)snprintf(lines, sizeof(lines), "%d-%d", search->node.start_line,
                           search->node.end_line > search->node.start_line
                               ? search->node.end_line
                               : search->node.start_line);
        yyjson_mut_val *row = yyjson_mut_arr(doc);
        yyjson_mut_arr_add_strcpy(doc, row, prefix ? qn + prefix + 1U : qn);
        yyjson_mut_arr_add_strcpy(doc, row, search->node.label ? search->node.label : "");
        yyjson_mut_arr_add_strcpy(doc, row, lines);
        yyjson_mut_arr_add_int(doc, row, search->in_degree);
        yyjson_mut_arr_add_int(doc, row, search->out_degree);
        if (field_count > 0) {
            yyjson_doc *properties = search->node.properties_json
                                         ? yyjson_read(search->node.properties_json,
                                                       strlen(search->node.properties_json), 0)
                                         : NULL;
            yyjson_val *property_root = properties ? yyjson_doc_get_root(properties) : NULL;
            for (int field = 0; field < field_count; ++field)
                add_property_value(doc, row,
                                   yyjson_is_obj(property_root)
                                       ? yyjson_obj_get(property_root, fields[field])
                                       : NULL);
            if (properties)
                yyjson_doc_free(properties);
        }
        if (include_connected) {
            yyjson_mut_val *connected = yyjson_mut_arr(doc);
            for (int c = 0; c < search->connected_count; ++c)
                yyjson_mut_arr_add_strcpy(
                    doc, connected, search->connected_names[c] ? search->connected_names[c] : "");
            yyjson_mut_arr_add_val(row, connected);
        }
        yyjson_mut_arr_add_val(rows, row);
    }
    yyjson_mut_obj_add_val(doc, root, "groups", groups);
    yyjson_mut_obj_add_bool(doc, root, "has_more", output->total > offset + output->count);
}

typedef enum {
    SEMANTIC_OK = 0,
    SEMANTIC_TYPE_ERROR,  /* semantic_query is not an array of strings */
    SEMANTIC_STORE_ERROR, /* the vector scan itself failed */
} semantic_status_t;

typedef struct {
    cbm_vector_result_t *results;
    int count; /* ranked rows materialized (offset + limit + 1 lookahead at most) */
    int offset;
    int limit;
    bool total_exact;
    bool present;
} semantic_page_t;

/* A mixed-type array is a caller error and is never silently narrowed to its
 * string members. A store without a vector table (lean index) yields an empty
 * page; a failed scan is SEMANTIC_STORE_ERROR and must never be rendered as
 * zero matches. */
static semantic_status_t semantic_query(const char *args, cbm_store_t *store, const char *project,
                                        int materialize_limit, cbm_vector_result_t **results,
                                        int *count, bool *present) {
    *results = NULL;
    *count = 0;
    *present = false;
    yyjson_doc *doc = read_args(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *array = yyjson_is_obj(root) ? yyjson_obj_get(root, "semantic_query") : NULL;
    if (!array) {
        if (doc)
            yyjson_doc_free(doc);
        return SEMANTIC_OK;
    }
    *present = true;
    semantic_status_t status = SEMANTIC_OK;
    if (!yyjson_is_arr(array)) {
        status = SEMANTIC_TYPE_ERROR;
    } else if (yyjson_arr_size(array) > 0) {
        const char *keywords[SEARCH_MAX_SEMANTIC_KEYWORDS];
        int keyword_count = 0;
        size_t index, max;
        yyjson_val *value;
        yyjson_arr_foreach(array, index, max, value) {
            if (!yyjson_is_str(value)) {
                status = SEMANTIC_TYPE_ERROR;
                break;
            }
            if (keyword_count < SEARCH_MAX_SEMANTIC_KEYWORDS)
                keywords[keyword_count++] = yyjson_get_str(value);
        }
        if (status == SEMANTIC_OK) {
            cbm_vector_result_t *found = NULL;
            int found_count = 0;
            int rc = cbm_store_vector_search(store, project, keywords, keyword_count,
                                             materialize_limit, &found, &found_count);
            if (rc == CBM_STORE_ERR) {
                status = SEMANTIC_STORE_ERROR;
            } else if (rc == CBM_STORE_OK && found_count > 0) {
                *results = found;
                *count = found_count;
            }
        }
    }
    yyjson_doc_free(doc);
    return status;
}

static bool semantic_engine_saturated(const semantic_page_t *page, int returned) {
    bool remaining = page->offset + returned < page->count;
    return remaining && returned > 0 &&
           (long long)page->offset + returned > SEARCH_SEMANTIC_MAX_OFFSET;
}

static void emit_semantic(yyjson_mut_doc *doc, yyjson_mut_val *root, const semantic_page_t *page) {
    int available = page->count > page->offset ? page->count - page->offset : 0;
    int returned = available < page->limit ? available : page->limit;
    const cbm_vector_result_t *results = returned > 0 ? page->results + page->offset : NULL;
    bool remaining = page->offset + returned < page->count;
    bool saturated = semantic_engine_saturated(page, returned);
    bool has_more = remaining && !saturated;
    yyjson_mut_obj_add_int(doc, root, "semantic_total", page->count);
    yyjson_mut_obj_add_str(doc, root, "semantic_total_relation",
                           page->total_exact && !saturated ? "eq" : "gte");
    yyjson_mut_obj_add_int(doc, root, "semantic_returned", returned);
    yyjson_mut_obj_add_bool(doc, root, "semantic_has_more", has_more);
    if (has_more && returned > 0)
        yyjson_mut_obj_add_int(doc, root, "semantic_next_offset", page->offset + returned);
    else if (has_more)
        yyjson_mut_obj_add_bool(doc, root,
                                page->limit == 0 ? "semantic_continuation_requires_positive_limit"
                                                 : "semantic_continuation_requires_higher_budget",
                                true);
    if (saturated) {
        yyjson_mut_obj_add_bool(doc, root, "semantic_engine_saturated", true);
        yyjson_mut_obj_add_bool(doc, root, "semantic_continuation_unavailable", true);
    }
    yyjson_mut_val *semantic = yyjson_mut_obj(doc);
    yyjson_mut_val *columns = yyjson_mut_arr(doc);
    static const char *const names[] = {"qn", "label", "file", "score"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        yyjson_mut_arr_add_str(doc, columns, names[i]);
    yyjson_mut_obj_add_val(doc, semantic, "cols", columns);
    yyjson_mut_val *rows = yyjson_mut_arr(doc);
    for (int i = 0; i < returned; ++i) {
        yyjson_mut_val *row = yyjson_mut_arr(doc);
        yyjson_mut_arr_add_strcpy(doc, row,
                                  results[i].qualified_name ? results[i].qualified_name : "");
        yyjson_mut_arr_add_strcpy(doc, row, results[i].label ? results[i].label : "");
        yyjson_mut_arr_add_strcpy(doc, row, results[i].file_path ? results[i].file_path : "");
        yyjson_mut_arr_add_real(doc, row, results[i].score);
        yyjson_mut_arr_add_val(rows, row);
    }
    yyjson_mut_obj_add_val(doc, semantic, "rows", rows);
    yyjson_mut_obj_add_val(doc, root, "semantic", semantic);
}

cbm_operation_result_t cbm_search_operation_execute(const char *args) {
    char *project = string_arg(args, "project");
    char *query = string_arg(args, "query");
    char *label = string_arg(args, "label");
    char *name_pattern = string_arg(args, "name_pattern");
    char *qn_pattern = string_arg(args, "qn_pattern");
    char *file_pattern = string_arg(args, "file_pattern");
    char *relationship = string_arg(args, "relationship");
    int limit = int_arg(args, "limit", SEARCH_DEFAULT_LIMIT);
    int offset = int_arg(args, "offset", 0);
    int min_degree = int_arg(args, "min_degree", -1);
    int max_degree = int_arg(args, "max_degree", -1);
    int semantic_limit = int_arg(args, "semantic_limit", SEARCH_DEFAULT_LIMIT);
    int semantic_offset = int_arg(args, "semantic_offset", 0);
    bool exclude_entry_points = bool_arg(args, "exclude_entry_points");
    bool include_connected = bool_arg(args, "include_connected");
    if (limit < 1)
        limit = 1;
    else if (limit > SEARCH_MAX_LIMIT)
        limit = SEARCH_MAX_LIMIT;
    if (offset < 0)
        offset = 0;
    if (semantic_limit < 0)
        semantic_limit = 0;
    else if (semantic_limit > SEARCH_SEMANTIC_MAX_LIMIT)
        semantic_limit = SEARCH_SEMANTIC_MAX_LIMIT;
    if (semantic_offset < 0)
        semantic_offset = 0;
    /* One ranked hit beyond the requested page makes has_more deterministic. */
    int semantic_materialize_limit = semantic_offset + semantic_limit + 1;

    cbm_operation_result_t result = {0};
    int original_count = 0;
    cbm_store_t *store = NULL;
    cbm_store_open_status_t open_status = CBM_STORE_OPEN_NOT_FOUND;
    cbm_search_output_t output = {0};
    cbm_vector_result_t *vectors = NULL;
    int vector_count = 0;
    yyjson_doc *fields_owner = NULL;

    if (!project || !project[0]) {
        result = error_result("project is required", "Run the command from an indexed repository.");
        goto done;
    }
    if (semantic_offset > SEARCH_SEMANTIC_MAX_OFFSET) {
        result = error_result("semantic_offset maximum is 99998",
                              "Semantic ranking is resource-bounded; continue only with a "
                              "semantic_next_offset emitted by search.");
        goto done;
    }
    if (relationship && !valid_relationship(relationship)) {
        result = error_result("relationship must be uppercase letters and underscores", NULL);
        goto done;
    }
    if (query && search_arg_present(args, "semantic_query")) {
        result = error_result("query and semantic_query are mutually exclusive",
                              "Use query for BM25 full-text ranking or semantic_query for vector "
                              "ranking, then issue a separate request for the other mode.");
        goto done;
    }
    store = open_indexed_project(project, &open_status);
    if (!store) {
        result =
            open_status == CBM_STORE_OPEN_CORRUPT
                ? error_result(CBM_STORE_CORRUPT_MESSAGE, CBM_STORE_CORRUPT_HINT)
                : error_result("project not indexed", "Run 'codebase-memory-cli index .' first.");
        goto done;
    }

    if (query && query[0]) {
        result = bm25_search(store, project, query, file_pattern, label, limit, offset, args);
        if (result.payload && result.payload[0])
            goto done;
        cbm_operation_result_dispose(&result);
    }

    bool semantic_present = false;
    semantic_status_t semantic_status =
        semantic_query(args, store, project, semantic_materialize_limit, &vectors, &vector_count,
                       &semantic_present);
    if (semantic_status == SEMANTIC_STORE_ERROR) {
        result = error_result("semantic search failed: the vector index could not be scanned",
                              "See the daemon log for the SQLite error; re-index the project if "
                              "it persists.");
        goto done;
    }
    if (semantic_status != SEMANTIC_OK) {
        result = error_result("semantic_query must be an array of keyword strings, and every "
                              "element must be a string",
                              "Example: --semantic-query '[\"send\",\"publish\"]'.");
        goto done;
    }
    semantic_page_t semantic_page = {.results = vectors,
                                     .count = vector_count,
                                     .offset = semantic_offset,
                                     .limit = semantic_limit,
                                     .total_exact = vector_count < semantic_materialize_limit,
                                     .present = semantic_present};
    bool structural = label || name_pattern || qn_pattern || file_pattern || relationship ||
                      exclude_entry_points || min_degree != -1 || max_degree != -1;
    bool semantic_only = semantic_present && !structural;
    cbm_search_params_t params = {.project = project,
                                  .label = label,
                                  .name_pattern = name_pattern,
                                  .qn_pattern = qn_pattern,
                                  .file_pattern = file_pattern,
                                  .relationship = relationship,
                                  .exclude_entry_points = exclude_entry_points,
                                  .include_connected = include_connected,
                                  .limit = limit,
                                  .offset = offset,
                                  .min_degree = min_degree,
                                  .max_degree = max_degree};
    if (!semantic_only && cbm_store_search(store, &params, &output) != CBM_STORE_OK) {
        result = error_result("graph search failed", NULL);
        goto done;
    }

    const char *fields[SEARCH_MAX_FIELDS];
    bool core_requested = false;
    int field_count = parse_fields(args, fields, &fields_owner, &core_requested);
    original_count = output.count;
    int original_fields = field_count;
    size_t ceiling = cbm_output_budget_bytes(cbm_output_budget_tokens(args, 0));
    bool budget_hit = false;
    for (;;) {
        yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
        yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
        if (!doc || !root) {
            if (doc)
                yyjson_mut_doc_free(doc);
            result = error_result("result allocation failed", NULL);
            goto done;
        }
        yyjson_mut_doc_set_root(doc, root);
        if (!semantic_only)
            emit_structural(doc, root, &output, offset, fields, field_count, include_connected);
        if (core_requested)
            yyjson_mut_obj_add_str(doc, root, "fields_hint",
                                   "Core fields are already included and were not duplicated as "
                                   "extra property columns.");
        if (output.total == 0 && !semantic_only) {
            if (name_pattern && label)
                yyjson_mut_obj_add_str(
                    doc, root, "hint",
                    "No results. Remove the label filter or broaden name_pattern.");
            else if (name_pattern)
                yyjson_mut_obj_add_str(
                    doc, root, "hint",
                    "No nodes match this pattern. Check spelling or broaden the regex.");
            else if (label)
                yyjson_mut_obj_add_str(
                    doc, root, "hint",
                    "No nodes have this label. Use architecture/schema discovery to "
                    "inspect available labels.");
        }
        if (semantic_present) {
            emit_semantic(doc, root, &semantic_page);
            if (semantic_only && vector_count == 0)
                yyjson_mut_obj_add_str(
                    doc, root, "hint",
                    "No semantic matches. Re-index at moderate/full semantic depth "
                    "or broaden the keywords.");
        }
        if (budget_hit) {
            yyjson_mut_obj_add_bool(doc, root, "truncated", true);
            yyjson_mut_obj_add_str(doc, root, "truncation_reason", "output_budget");
            if (field_count < original_fields)
                yyjson_mut_obj_add_int(doc, root, "fields_omitted", original_fields - field_count);
            if (!semantic_only) {
                yyjson_mut_obj_add_int(doc, root, "returned", output.count);
                if (output.total > offset + output.count)
                    yyjson_mut_obj_add_int(doc, root, "next_offset", offset + output.count);
            }
        }
        char *format = string_arg(args, "format");
        bool json_format = format && strcmp(format, "json") == 0;
        cbm_operation_arg_free(format);
        if (json_format) {
            result = json_result(doc, false);
        } else {
            if (!semantic_only) {
                yyjson_mut_obj_remove_key(root, "cols");
                yyjson_mut_obj_remove_key(root, "groups");
                yyjson_mut_obj_remove_key(root, "count");
                if (!budget_hit) {
                    yyjson_mut_obj_add_int(doc, root, "returned", output.count);
                    if (output.total > offset + output.count)
                        yyjson_mut_obj_add_int(doc, root, "next_offset", offset + output.count);
                }
            }
            cbm_operation_result_t metadata_result = json_result(doc, false);
            char *metadata =
                metadata_result.payload ? cbm_json_to_tree(metadata_result.payload) : NULL;
            cbm_operation_result_dispose(&metadata_result);
            cbm_sb_t tree;
            cbm_sb_init(&tree);
            if (!semantic_only)
                emit_search_results_tree(&tree, &output, fields, field_count, include_connected,
                                         output.count);
            if (!metadata)
                tree.oom = true;
            cbm_sb_append(&tree, metadata);
            free(metadata);
            result = cbm_operation_result_take(cbm_sb_finish(&tree), false);
        }

        if (!result.payload) {
            result.is_error = true;
            break;
        }
        if (!ceiling || strlen(result.payload) <= ceiling)
            break;
        budget_hit = true;
        cbm_operation_result_dispose(&result);
        if (field_count > 0)
            field_count = 0;
        else if (semantic_page.limit > 0 && semantic_present)
            --semantic_page.limit;
        else if (output.count > 0)
            --output.count;
        else {
            result = error_result("max_output_tokens is too small for search metadata", NULL);
            break;
        }
    }

done:
    if (fields_owner)
        yyjson_doc_free(fields_owner);
    if (vectors)
        cbm_store_free_vector_results(vectors, vector_count);
    if (original_count > 0)
        output.count = original_count;
    cbm_store_search_free(&output);
    if (store)
        cbm_store_close(store);
    cbm_operation_arg_free(project);
    cbm_operation_arg_free(query);
    cbm_operation_arg_free(label);
    cbm_operation_arg_free(name_pattern);
    cbm_operation_arg_free(qn_pattern);
    cbm_operation_arg_free(file_pattern);
    cbm_operation_arg_free(relationship);
    return result;
}
