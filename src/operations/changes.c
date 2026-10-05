#include "operations/output_budget.h"
#include "operations/result_wire.h"
#include "operations/changes.h"
#include "operations/json_args.h"

#include "foundation/compat_fs.h"
#include "foundation/constants.h"
#include "foundation/limits.h"
#include "foundation/log.h"
#include "foundation/platform.h"
#include "foundation/sha256.h"
#include "foundation/str_util.h"
#include "operations/command_runner.h"
#include "operations/compact_out.h"
#include "operations/store_host.h"
#include "pipeline/pipeline.h"
#include "store/store.h"
#include "yyjson/yyjson.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

enum {
    CHANGES_DEFAULT_BFS_DEPTH = 2,
    CHANGES_BFS_LIMIT_MAX = 5000,
    CHANGES_DEFAULT_IMPACT_LIMIT = 200,
    CHANGES_DEFAULT_PAGE_LIMIT = 20,
};

#define CHANGES_SKIP_ONE 1
#define CHANGES_PAIR_LEN 2

static char *changes_strdup(const char *text) {
    if (!text)
        return NULL;
    size_t len = strlen(text);
    char *copy = malloc(len + 1U);
    if (copy)
        memcpy(copy, text, len + 1U);
    return copy;
}

static void *changes_realloc(void *ptr, size_t size) {
    void *grown = realloc(ptr, size);
    if (!grown && size != 0) {
        free(ptr);
        abort();
    }
    return grown;
}

static yyjson_doc *changes_args_doc(const char *args) {
    const char *json = args ? args : "{}";
    return yyjson_read(json, strlen(json), 0);
}

static char *changes_string_arg(const char *args, const char *name) {
    yyjson_doc *doc = changes_args_doc(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    char *result = value && yyjson_is_str(value)
                       ? cbm_mem_strdup(CBM_MEM_CLASS_OPERATION_ARG, yyjson_get_str(value))
                       : NULL;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static char *changes_project_arg(const char *args) {
    static const char *const names[] = {"project", "project_name", "project_id", "projectName"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        char *value = changes_string_arg(args, names[i]);
        if (value)
            return value;
    }
    return NULL;
}

static int changes_int_arg(const char *args, const char *name, int fallback) {
    yyjson_doc *doc = changes_args_doc(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    int result = value && yyjson_is_int(value) ? (int)yyjson_get_sint(value) : fallback;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static int changes_clamp_depth(int depth, const char *tool) {
    int cap = cbm_operation_max_depth();
    if (depth > cap) {
        char req_buf[16];
        char cap_buf[16];
        snprintf(req_buf, sizeof(req_buf), "%d", depth);
        snprintf(cap_buf, sizeof(cap_buf), "%d", cap);
        cbm_log_warn("operation.depth_capped", "operation", tool, "requested", req_buf, "cap",
                     cap_buf);
        return cap;
    }
    return depth;
}

static bool changes_validate_search_path_arg(const char *s) {
    if (!s)
        return false;
    for (const char *p = s; *p; ++p) {
        switch (*p) {
        case '\'':
        case '"':
        case ';':
        case '|':
        case '$':
        case '`':
        case '<':
        case '>':
        case '\n':
        case '\r':
#ifndef _WIN32
        case '\\':
#endif
            return false;
        default:
            break;
        }
    }
    return true;
}

static bool changes_validate_windows_cmd_interpolation_arg(const char *s) {
#ifdef _WIN32
    return s && strpbrk(s, "%!^") == NULL;
#else
    return s != NULL;
#endif
}

static char *changes_doc_to_str(yyjson_mut_doc *doc) {
    return cbm_operation_json_write(doc);
}

static cbm_operation_result_t changes_error(const char *message) {
    return cbm_operation_result_copy(message ? message : "detect_changes failed", true);
}

static cbm_operation_result_t changes_project_error(const char *project,
                                                    cbm_store_open_status_t open_status) {
    if (open_status == CBM_STORE_OPEN_CORRUPT) {
        return changes_error(CBM_STORE_CORRUPT_ERROR);
    }
    if (!project) {
        return changes_error(
            "{\"error\":\"missing required argument: project\",\"hint\":\"Pass the project as the "
            "\\\"project\\\" argument. Run projects to see indexed projects.\"}");
    }
    return changes_error("{\"error\":\"project not found or not indexed\",\"hint\":\"Run projects "
                         "to see indexed projects.\"}");
}

static cbm_store_t *changes_open_store_and_root(const char *project, char **root_path_out,
                                                cbm_store_open_status_t *open_status) {
    *root_path_out = NULL;
    *open_status = CBM_STORE_OPEN_NOT_FOUND;
    if (!project || !project[0])
        return NULL;
    cbm_store_t *store = cbm_store_host_open_query(project, open_status);
    if (!store)
        return NULL;
    cbm_project_t info = {0};
    if (cbm_store_get_project(store, project, &info) != CBM_STORE_OK || !info.root_path ||
        !info.root_path[0]) {
        cbm_project_free_fields(&info);
        cbm_store_close(store);
        return NULL;
    }
    *root_path_out = changes_strdup(info.root_path);
    cbm_project_free_fields(&info);
    if (!*root_path_out) {
        cbm_store_close(store);
        return NULL;
    }
    return store;
}

static int changes_hop_cmp_qn(const void *pa, const void *pb) {
    const cbm_node_hop_t *a = pa;
    const cbm_node_hop_t *b = pb;
    const char *qa = a->node.qualified_name ? a->node.qualified_name : "";
    const char *qb = b->node.qualified_name ? b->node.qualified_name : "";
    int c = strcmp(qa, qb);
    return c != 0 ? c : a->hop - b->hop;
}

bool cbm_detect_node_in_hunks(const cbm_node_t *node, const cbm_changed_hunk_t *hunks,
                              int hunk_count, const char *file) {
    for (int h = 0; h < hunk_count; ++h) {
        if (strcmp(hunks[h].path, file) == 0 && node->start_line <= hunks[h].end_line &&
            node->end_line >= hunks[h].start_line) {
            return true;
        }
    }
    return false;
}
static bool detect_is_seedable_label(const char *lb) {
    return lb && strcmp(lb, "File") != 0 && strcmp(lb, "Folder") != 0 &&
           strcmp(lb, "Project") != 0 && strcmp(lb, "Module") != 0 && strcmp(lb, "Package") != 0 &&
           strcmp(lb, "Section") != 0;
}

static void detect_collect_seeds(cbm_store_t *store, const char *project, const char *file,
                                 const cbm_changed_hunk_t *hunks, int hunk_count, int64_t **seeds,
                                 int *n, int *cap) {
    cbm_node_t *nodes = NULL;
    int ncount = 0;
    cbm_store_find_nodes_by_file(store, project, file, &nodes, &ncount);
    bool scope_to_hunks = false;
    for (int h = 0; h < hunk_count; h++) {
        if (strcmp(hunks[h].path, file) == 0) {
            scope_to_hunks = true;
            break;
        }
    }
    /* A file can have hunks yet no SEEDABLE definition overlapping any of them:
     * an import-only edit, a module-level constant, or a change above the first
     * definition all land outside every definition's line range. Scoping would
     * then drop the file from the seed set entirely — strictly worse recall
     * than the whole-file behavior this replaces. Probe for an overlap first
     * and keep whole-file seeding for that file when there is none.
     *
     * The probe must apply the same label filter as the seeding loop below:
     * container nodes span the whole file (a Module node is lines 1..EOF), so
     * counting them would report an overlap for every hunk and defeat the
     * fallback entirely. */
    if (scope_to_hunks) {
        bool any_overlap = false;
        for (int i = 0; i < ncount && !any_overlap; i++) {
            any_overlap = detect_is_seedable_label(nodes[i].label) &&
                          cbm_detect_node_in_hunks(&nodes[i], hunks, hunk_count, file);
        }
        scope_to_hunks = any_overlap;
    }
    for (int i = 0; i < ncount; i++) {
        if (detect_is_seedable_label(nodes[i].label)) {
            if (scope_to_hunks && !cbm_detect_node_in_hunks(&nodes[i], hunks, hunk_count, file)) {
                continue;
            }
            if (*n >= *cap) {
                *cap = *cap ? *cap * 2 : 16;
                *seeds = changes_realloc(*seeds, (size_t)*cap * sizeof(int64_t));
            }
            (*seeds)[(*n)++] = nodes[i].id;
        }
    }
    cbm_store_free_nodes(nodes, ncount);
}

/* Module key for the impacted rollup = the first TWO path segments
 * ("src/operations/search.c" -> "src/operations"), a quotient of the blast radius coarse enough
 * to fit yet specific enough to localize (one segment collapses a whole tree
 * to "src"). Falls back to one segment, then the whole path. */
static char *detect_module_of(const char *file) {
    if (!file || !file[0]) {
        return changes_strdup("(root)");
    }
    const char *s1 = strchr(file, '/');
    if (!s1) {
        return changes_strdup(file);
    }
    const char *s2 = strchr(s1 + 1, '/');
    size_t len = s2 ? (size_t)(s2 - file) : strlen(file);
    char *module = malloc(len + 1U);
    if (!module) {
        return NULL;
    }
    memcpy(module, file, len);
    module[len] = '\0';
    return module;
}

/* Aggregate the impact set into the 2-segment module rollup. Fills up to
 * DETECT_MODCAP (module, count) pairs; symbols beyond the cap land in
 * *overflow (surfaced as "(other)", never silently dropped). Shared by the
 * tree and json emitters so both encodings carry the same model. */
enum { DETECT_MODCAP = 256 };

typedef struct {
    char *name;
    int count;
} detect_module_row_t;

static int detect_module_rollup(const cbm_traverse_result_t *impact, detect_module_row_t *modules,
                                int *overflow) {
    int nmods = 0;
    *overflow = 0;
    for (int i = 0; i < impact->visited_count; i++) {
        char *module = detect_module_of(impact->visited[i].node.file_path);
        if (!module) {
            (*overflow)++;
            continue;
        }
        int j = 0;
        for (; j < nmods; j++) {
            if (strcmp(modules[j].name, module) == 0) {
                modules[j].count++;
                break;
            }
        }
        if (j == nmods) {
            if (nmods < DETECT_MODCAP) {
                modules[nmods].name = module;
                modules[nmods].count = 1;
                nmods++;
                module = NULL;
            } else {
                (*overflow)++;
            }
        }
        free(module);
    }
    return nmods;
}

static void detect_module_rollup_free(detect_module_row_t *modules, int count) {
    for (int i = 0; i < count; i++) {
        free(modules[i].name);
    }
    free(modules);
}

/* Emit one losslessly pageable window of the impacted set. The visited array is
 * hop-ordered, so page zero preserves the closest, highest-signal rows. Engine
 * saturation is reported as a lower-bound total, never as an exact count. The
 * rows are grouped by qualified name AFTER the window is selected, on a copy,
 * so the pageable order of tr->visited is never disturbed. */
static void detect_emit_impacted_tree(cbm_sb_t *sb, const cbm_traverse_result_t *tr, int start,
                                      int count, bool engine_saturated) {
    cbm_tree_scalar_int(sb, "impacted_total", tr->visited_count);
    cbm_tree_scalar_str(sb, "impacted_total_relation", engine_saturated ? "gte" : "eq");
    int shown = count;
    cbm_tree_scalar_int(sb, "impacted_shown", shown);
    cbm_node_hop_t *selected = NULL;
    const cbm_node_hop_t *rows = tr->visited;
    if (shown > 0) {
        selected = malloc((size_t)shown * sizeof(*selected));
        if (selected) {
            memcpy(selected, tr->visited + start, (size_t)shown * sizeof(*selected));
            rows = selected;
        } else {
            rows = tr->visited + start;
        }
    }
    if (shown > 1 && selected) {
        qsort(selected, (size_t)shown, sizeof(*selected), changes_hop_cmp_qn);
    }
    static const char *const columns[] = {"qn", "label", "file", "hop"};
    const char **cells = shown > 0 ? calloc((size_t)shown * 4U, sizeof(*cells)) : NULL;
    char (*hop_text)[32] = shown > 0 ? calloc((size_t)shown, sizeof(*hop_text)) : NULL;
    if (shown == 0 || (cells && hop_text)) {
        for (int i = 0; i < shown; i++) {
            size_t base = (size_t)i * 4U;
            cells[base] = rows[i].node.qualified_name ? rows[i].node.qualified_name : "";
            cells[base + 1U] = rows[i].node.label ? rows[i].node.label : "";
            cells[base + 2U] = rows[i].node.file_path ? rows[i].node.file_path : "";
            snprintf(hop_text[i], sizeof(hop_text[i]), "%d", rows[i].hop);
            cells[base + 3U] = hop_text[i];
        }
        static const bool string_cols[] = {true, true, true, false};
        static const bool prefix_cols[] = {true, false, true, false};
        cbm_tree_table_rows_profiled(sb, "impacted", shown, columns, 4, cells, string_cols,
                                     prefix_cols);
    } else {
        cbm_tree_table_header(sb, "impacted", 0, columns, 4);
        cbm_tree_scalar_str(sb, "impacted_render_error", "out_of_memory");
    }
    free(hop_text);
    free(cells);
    free(selected);
    bool has_more = start + shown < tr->visited_count;
    cbm_tree_scalar_bool(sb, "impacted_has_more", has_more);
    if (has_more && shown > 0) {
        cbm_tree_scalar_int(sb, "impacted_next_offset", start + shown);
    }
}

/* Portable, allocation-backed record reader. Git's -z formats preserve UTF-8
 * and allow every path byte except NUL, including newlines. A fixed or
 * line-based buffer would either quote or split valid paths. */
static char *detect_read_record(FILE *stream, int delimiter, bool *oom, bool *terminated) {
    cbm_sb_t record;
    cbm_sb_init(&record);
    bool saw_input = false;
    if (terminated) {
        *terminated = false;
    }
    for (;;) {
        int ch = fgetc(stream);
        if (ch == EOF) {
            break;
        }
        saw_input = true;
        if (ch == delimiter) {
            if (terminated) {
                *terminated = true;
            }
            break;
        }
        char byte = (char)ch;
        cbm_sb_append_n(&record, &byte, 1);
        if (record.oom) {
            break;
        }
    }
    if (record.oom) {
        if (oom) {
            *oom = true;
        }
        cbm_sb_free(&record);
        return NULL;
    }
    if (!saw_input) {
        cbm_sb_free(&record);
        return NULL;
    }
    return cbm_sb_finish(&record);
}

static bool detect_add_changed_path(char ***files, int *file_count, int *file_cap,
                                    const char *path) {
    if (!path || !path[0]) {
        return true;
    }
    for (int i = 0; i < *file_count; i++) {
        if (strcmp((*files)[i], path) == 0) {
            return true;
        }
    }
    if (*file_count >= *file_cap) {
        int next_cap = *file_cap ? *file_cap * 2 : 16;
        char **grown = realloc(*files, (size_t)next_cap * sizeof(*grown));
        if (!grown) {
            return false;
        }
        *files = grown;
        *file_cap = next_cap;
    }
    char *copy = changes_strdup(path);
    if (!copy) {
        return false;
    }
    (*files)[(*file_count)++] = copy;
    return true;
}

static int detect_changed_path_compare(const void *left, const void *right) {
    const char *const *left_path = left;
    const char *const *right_path = right;
    return strcmp(*left_path, *right_path);
}

static bool detect_valid_object_id(const char *value) {
    size_t length = value ? strlen(value) : 0;
    if (length != 40 && length != 64) {
        return false;
    }
    for (size_t i = 0; i < length; i++) {
        if (!isxdigit((unsigned char)value[i])) {
            return false;
        }
    }
    return true;
}

/* Snapshot cursors. Offset paging over a live repository can skip or repeat
 * rows when commits, the worktree or the graph change between calls. Each
 * pageable stream (changed files, impacted symbols, module rollup) gets a
 * cursor bound to a fingerprint of the complete answer: the resolved HEAD and
 * base commits, the merge-base, the index generation, the bytes of every
 * changed file, the impact rows and the module rollup. */
typedef struct {
    char stream;       /* c=changed files, i=impacted symbols, m=module rollup */
    char snapshot[33]; /* first 128 bits of the SHA-256 live-state fingerprint */
    uint64_t qhash;    /* semantic query identity; page sizing is deliberately excluded */
    int offset;        /* next row in this independently pageable stream */
} detect_cursor_t;

static uint64_t detect_fnv1a(const char *text, uint64_t hash) {
    while (text && *text) {
        hash ^= (uint64_t)(unsigned char)*text++;
        hash *= UINT64_C(0x100000001b3);
    }
    return hash;
}

static uint64_t detect_params_hash(const char *project, const char *base_branch, const char *scope,
                                   const char *direction, int depth) {
    uint64_t hash = UINT64_C(0xcbf29ce484222325);
    hash = detect_fnv1a(project ? project : "", hash);
    hash = detect_fnv1a("|", hash);
    hash = detect_fnv1a(base_branch ? base_branch : "", hash);
    hash = detect_fnv1a("|", hash);
    hash = detect_fnv1a(scope ? scope : "impact", hash);
    hash = detect_fnv1a("|", hash);
    hash = detect_fnv1a(direction ? direction : "inbound", hash);
    char depth_text[32];
    snprintf(depth_text, sizeof(depth_text), "|%d", depth);
    return detect_fnv1a(depth_text, hash);
}

static void detect_cursor_encode(char stream, const char snapshot[33], uint64_t qhash, int offset,
                                 char out[80]) {
    snprintf(out, 80, "d1.%c.%s.%016llx.%d", stream, snapshot, (unsigned long long)qhash, offset);
}

static const char *detect_cursor_decode(const char *token, char expected_stream,
                                        const char current_snapshot[33], uint64_t expected_qhash,
                                        detect_cursor_t *out) {
    static const char invalid[] =
        "invalid_cursor: unrecognized detect_changes cursor - rerun without the cursor";
    memset(out, 0, sizeof(*out));
    if (!token || strncmp(token, "d1.", 3) != 0 || token[3] != expected_stream || token[4] != '.') {
        return invalid;
    }
    out->stream = token[3];
    const char *snapshot_start = token + 5;
    const char *snapshot_end = strchr(snapshot_start, '.');
    if (!snapshot_end || snapshot_end - snapshot_start != 32) {
        return invalid;
    }
    for (const char *digit = snapshot_start; digit < snapshot_end; digit++) {
        if (!isxdigit((unsigned char)*digit)) {
            return invalid;
        }
    }
    memcpy(out->snapshot, snapshot_start, 32);
    out->snapshot[32] = '\0';

    const char *hash_start = snapshot_end + 1;
    const char *hash_end = strchr(hash_start, '.');
    if (!hash_end || hash_end - hash_start != 16) {
        return invalid;
    }
    for (const char *digit = hash_start; digit < hash_end; digit++) {
        if (!isxdigit((unsigned char)*digit)) {
            return invalid;
        }
    }
    errno = 0;
    char *parsed_end = NULL;
    unsigned long long parsed_hash = strtoull(hash_start, &parsed_end, 16);
    if (errno == ERANGE || parsed_end != hash_end) {
        return invalid;
    }
    errno = 0;
    long parsed_offset = strtol(hash_end + 1, &parsed_end, 10);
    if (errno == ERANGE || parsed_end == hash_end + 1 || *parsed_end != '\0' || parsed_offset < 1 ||
        parsed_offset > INT_MAX) {
        return invalid;
    }
    out->qhash = (uint64_t)parsed_hash;
    out->offset = (int)parsed_offset;
    if (out->qhash != expected_qhash) {
        return "cursor_params_mismatch: detect_changes cursor belongs to different semantic "
               "arguments - rerun without the cursor";
    }
    if (strcmp(out->snapshot, current_snapshot) != 0) {
        return "snapshot_changed: commits, worktree, or graph changed since this cursor was issued "
               "- rerun detect_changes without the cursor";
    }
    return NULL;
}

static void detect_snapshot_add_field(cbm_sha256_ctx *hash, const char *value) {
    size_t length = value ? strlen(value) : 0;
    char length_text[32];
    int count = snprintf(length_text, sizeof(length_text), "%zu:", length);
    cbm_sha256_update(hash, length_text, (size_t)count);
    if (length > 0) {
        cbm_sha256_update(hash, value, length);
    }
    cbm_sha256_update(hash, "|", 1);
}

static bool detect_snapshot_add_changed_file(cbm_sha256_ctx *hash, const char *root_path,
                                             const char *relative_path) {
    detect_snapshot_add_field(hash, relative_path);
    size_t root_len = strlen(root_path);
    size_t relative_len = strlen(relative_path);
    if (root_len > (size_t)-1 - relative_len - 2U) {
        detect_snapshot_add_field(hash, "path_overflow");
        return false;
    }
    char *absolute = malloc(root_len + relative_len + 2U);
    if (!absolute) {
        detect_snapshot_add_field(hash, "path_oom");
        return false;
    }
    memcpy(absolute, root_path, root_len);
    absolute[root_len] = '/';
    memcpy(absolute + root_len + 1U, relative_path, relative_len + 1U);

    cbm_path_info_t info = {0};
    int info_status = cbm_path_info_utf8(absolute, &info);
    if (info_status == CBM_PATH_INFO_ABSENT) {
        detect_snapshot_add_field(hash, "absent");
        free(absolute);
        return true;
    }
    if (info_status != CBM_PATH_INFO_OK) {
        /* Hash the observed failure state for diagnostics, but do not issue a
         * cursor from incomplete evidence. Offset paging remains available. */
        detect_snapshot_add_field(hash, "metadata_unavailable");
        free(absolute);
        return false;
    }
    char metadata[160];
    snprintf(metadata, sizeof(metadata), "r%d:d%d:l%d:s%lld:m%lld", info.is_regular ? 1 : 0,
             info.is_directory ? 1 : 0, info.is_symlink ? 1 : 0, (long long)info.size,
             (long long)info.mtime_ns);
    detect_snapshot_add_field(hash, metadata);
    bool complete = true;
    if (info.is_regular) {
        FILE *file = cbm_fopen(absolute, "rb");
        if (file) {
            unsigned char buffer[64 * 1024];
            size_t count;
            while ((count = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                cbm_sha256_update(hash, buffer, count);
            }
            bool read_error = ferror(file) != 0;
            detect_snapshot_add_field(hash, read_error ? "read_error" : "read_complete");
            complete = !read_error;
            (void)fclose(file);
        } else {
            detect_snapshot_add_field(hash, "open_error");
            complete = false;
        }
    } else if (info.is_symlink) {
        /* lstat-style metadata identifies the link object but not its target
         * bytes. The cross-platform metadata API intentionally does not expose
         * a readlink/reparse payload, so fail closed instead of pretending the
         * cursor is bound to the changed link identity. */
        detect_snapshot_add_field(hash, "link_identity_unavailable");
        complete = false;
    }
    free(absolute);
    return complete;
}

static bool detect_snapshot_fingerprint(const char *root_path, const char *head_oid,
                                        const char *base_oid, const char *merge_base,
                                        const char *generation, char **files, int file_count,
                                        const cbm_traverse_result_t *impact,
                                        const detect_module_row_t *modules, int module_count,
                                        int module_overflow, char out[33]) {
    cbm_sha256_ctx hash;
    cbm_sha256_init(&hash);
    detect_snapshot_add_field(&hash, "detect_changes_snapshot_v1");
    detect_snapshot_add_field(&hash, head_oid);
    detect_snapshot_add_field(&hash, base_oid);
    detect_snapshot_add_field(&hash, merge_base);
    detect_snapshot_add_field(&hash, generation);
    bool complete = true;
    for (int i = 0; i < file_count; i++) {
        if (!detect_snapshot_add_changed_file(&hash, root_path, files[i])) {
            complete = false;
        }
    }
    if (impact) {
        for (int i = 0; i < impact->visited_count; i++) {
            detect_snapshot_add_field(&hash, impact->visited[i].node.qualified_name);
            detect_snapshot_add_field(&hash, impact->visited[i].node.label);
            detect_snapshot_add_field(&hash, impact->visited[i].node.file_path);
            char row_metadata[64];
            snprintf(row_metadata, sizeof(row_metadata), "%d:%lld", impact->visited[i].hop,
                     (long long)impact->visited[i].node.id);
            detect_snapshot_add_field(&hash, row_metadata);
        }
    }
    for (int i = 0; i < module_count; i++) {
        detect_snapshot_add_field(&hash, modules[i].name);
        char count_text[32];
        snprintf(count_text, sizeof(count_text), "%d", modules[i].count);
        detect_snapshot_add_field(&hash, count_text);
    }
    char overflow_text[32];
    snprintf(overflow_text, sizeof(overflow_text), "%d", module_overflow);
    detect_snapshot_add_field(&hash, overflow_text);

    uint8_t digest[CBM_SHA256_DIGEST_LEN];
    static const char hex[] = "0123456789abcdef";
    cbm_sha256_final(&hash, digest);
    for (int i = 0; i < 16; i++) {
        out[i * 2] = hex[digest[i] >> 4];
        out[i * 2 + 1] = hex[digest[i] & 0x0f];
    }
    out[32] = '\0';
    return complete;
}

/* Run a contained git command. On success *fp_out is the open output stream
 * (caller closes and unlinks output_path); otherwise it is NULL and *cancelled
 * / *run tell the caller how to classify the failure. */
typedef struct {
    char output_path[CBM_SZ_2K];
    cbm_proc_result_t result;
    int run;
    bool cancelled;
    FILE *fp;
} changes_git_t;

static void changes_git_run(const cbm_operation_runtime_t *runtime, const char *cmd,
                            changes_git_t *git) {
    memset(git, 0, sizeof(*git));
    git->run = cbm_operation_run_shell_command(runtime, cmd, git->output_path, &git->result);
    git->cancelled = git->result.cancellation_requested || cbm_operation_runtime_cancelled(runtime);
    if (git->run == 0 && git->result.exit_code == 0 && !git->cancelled) {
        git->fp = cbm_fopen(git->output_path, "rb");
    }
}

static bool changes_git_ok(const changes_git_t *git) {
    return git->run == 0 && git->result.exit_code == 0 && !git->cancelled && git->fp != NULL;
}

static void changes_git_finish(changes_git_t *git) {
    if (git->fp) {
        (void)fclose(git->fp);
        git->fp = NULL;
    }
    if (git->output_path[0]) {
        (void)cbm_unlink(git->output_path);
        git->output_path[0] = '\0';
    }
}

/* Read one object id line (trailing CR tolerated) into out (<= 64 hex + NUL). */
static bool changes_read_oid(FILE *fp, char out[65], bool *oom) {
    bool terminated = false;
    char *record = detect_read_record(fp, '\n', oom, &terminated);
    if (!record) {
        return false;
    }
    size_t length = strlen(record);
    if (length > 0 && record[length - 1] == '\r') {
        record[--length] = '\0';
    }
    bool valid = terminated && detect_valid_object_id(record);
    if (valid) {
        memcpy(out, record, length + 1U);
    }
    free(record);
    return valid;
}

/* The mandatory detect_changes metadata (base/direction/scalars) is not rows:
 * if it alone cannot fit, answer with a small truthful record rather than
 * slicing a path or an identifier. */
static char *detect_budget_floor(bool legacy_json, bool engine_saturated, size_t budget_bytes) {
    if (!legacy_json) {
        cbm_sb_t sb;
        cbm_sb_init(&sb);
        cbm_tree_scalar_str(&sb, "truncation_reason", "output_budget");
        cbm_tree_scalar_bool(&sb, "truncated", true);
        cbm_tree_scalar_bool(&sb, "output_budget_floor_exceeded", true);
        if (engine_saturated) {
            cbm_tree_scalar_bool(&sb, "engine_saturated", true);
        }
        cbm_tree_scalar_int(&sb, "max_output_bytes", (long long)budget_bytes);
        cbm_tree_scalar_str(&sb, "hint",
                            "mandatory detect_changes metadata exceeds the budget; raise "
                            "max_output_tokens (no path or identifier was sliced)");
        return cbm_sb_finish(&sb);
    }
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *floor = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !floor) {
        if (doc) {
            yyjson_mut_doc_free(doc);
        }
        return NULL;
    }
    yyjson_mut_doc_set_root(doc, floor);
    yyjson_mut_obj_add_str(doc, floor, "truncation_reason", "output_budget");
    yyjson_mut_obj_add_bool(doc, floor, "truncated", true);
    yyjson_mut_obj_add_bool(doc, floor, "output_budget_floor_exceeded", true);
    if (engine_saturated) {
        yyjson_mut_obj_add_bool(doc, floor, "engine_saturated", true);
    }
    yyjson_mut_obj_add_uint(doc, floor, "max_output_bytes", budget_bytes);
    yyjson_mut_obj_add_str(doc, floor, "hint",
                           "mandatory detect_changes metadata exceeds the budget; raise "
                           "max_output_tokens (no path or identifier was sliced)");
    char *json = changes_doc_to_str(doc);
    yyjson_mut_doc_free(doc);
    return json;
}

cbm_operation_result_t cbm_changes_operation_execute(const char *args,
                                                     const cbm_operation_runtime_t *runtime) {
    char *project = changes_project_arg(args);
    char *base_branch = changes_string_arg(args, "base_branch");
    char *since = changes_string_arg(args, "since");
    char *scope = changes_string_arg(args, "scope");
    int depth = changes_int_arg(args, "depth", CHANGES_DEFAULT_BFS_DEPTH);
    depth = changes_clamp_depth(depth, "detect_changes");

    /* scope: "files" = changed files only; "impact" = files + symbols (default).
     * Keep accepting the legacy "symbols" spelling for compatibility. */
    bool want_symbols = !scope || strcmp(scope, "symbols") == 0 || strcmp(scope, "impact") == 0;

    /* `since` (e.g. "HEAD~10", "v0.5.0") is the documented diff base but was
     * previously parsed and never used: it takes precedence over base_branch.
     * Route it through base_branch so the shared shell-arg validation and the
     * existing `<base>...HEAD` (three-dot) diff apply unchanged — `since` thus
     * adopts the same merge-base semantics base_branch already uses. */
    if (since && since[0]) {
        cbm_operation_arg_free(base_branch);
        base_branch = since; /* transfer ownership */
        since = NULL;
    }
    cbm_operation_arg_free(since); /* no-op after the swap (since is NULL); frees it otherwise */

    if (!base_branch) {
        base_branch = cbm_mem_strdup(CBM_MEM_CLASS_OPERATION_ARG, "main");
    }

    cbm_operation_result_t result = {0};
    cbm_store_t *store = NULL;
    char *root_path = NULL;
    char *direction = NULL;
    char **files = NULL;
    int file_count = 0;
    int file_cap = 0;
    int64_t *seeds = NULL;
    int seed_count = 0;
    int seed_cap = 0;
    cbm_changed_hunk_t *hunks = NULL;
    int hunk_count = 0;
    cbm_traverse_result_t impact = {0};
    detect_module_row_t *modules = NULL;
    int nmods = 0;
    char *impact_cursor_arg = NULL;
    char *changed_cursor_arg = NULL;
    char *module_cursor_arg = NULL;
    char *out_str = NULL;
    changes_git_t git = {0};

    /* Reject shell metacharacters, and a leading '-', in the user-supplied
     * branch name. base_branch is spliced into `git diff --relative --name-only
     * "<base>"...HEAD`; a value starting with '-' would be read by git as an
     * option rather than a ref (e.g. `--output=<path>` writes the diff to an
     * arbitrary file). A real git ref never begins with '-'. */
    if (!cbm_validate_shell_arg(base_branch) || base_branch[0] == '-' ||
        !changes_validate_windows_cmd_interpolation_arg(base_branch)) {
        result = changes_error("base_branch contains invalid characters");
        goto done;
    }

    cbm_store_open_status_t open_status = CBM_STORE_OPEN_NOT_FOUND;
    store = changes_open_store_and_root(project, &root_path, &open_status);
    if (!store) {
        result = changes_project_error(project, open_status);
        goto done;
    }

    if (!changes_validate_search_path_arg(root_path) ||
        !changes_validate_windows_cmd_interpolation_arg(root_path)) {
        result = changes_error("project path contains invalid characters");
        goto done;
    }

    /* Every detect snapshot and cursor is generation-bound. Validate the
     * metadata immediately after store resolution so fresh requests and cursor
     * replays fail identically before Git work or cursor minting. */
    char generation[96] = "";
    if (cbm_store_generation(store, generation, sizeof(generation)) != CBM_STORE_OK) {
        result = changes_error("index_metadata_error: generation metadata is unreadable; reindex "
                               "before detecting changes");
        goto done;
    }

    /* Direction of impact. Default inbound = the BLAST RADIUS: the transitive
     * CALLERS of the changed symbols, which may need review. outbound = what
     * the changed code depends on; both = union. */
    direction = changes_string_arg(args, "direction");
    if (!direction) {
        direction = cbm_mem_strdup(CBM_MEM_CLASS_OPERATION_ARG, "inbound");
    }
    /* Teaching error, same contract as trace_path: never silently correct an
     * unknown direction — the caller would misread the result's semantics. */
    if (strcmp(direction, "inbound") != 0 && strcmp(direction, "outbound") != 0 &&
        strcmp(direction, "both") != 0) {
        char errbuf[CBM_SZ_256];
        snprintf(errbuf, sizeof(errbuf),
                 "invalid direction \"%s\" — use \"inbound\" (blast radius: transitive callers), "
                 "\"outbound\" (dependencies), or \"both\"",
                 direction);
        result = changes_error(errbuf);
        goto done;
    }
    char *fmt = changes_string_arg(args, "format");
    bool legacy_json = fmt && strcmp(fmt, "json") == 0;
    cbm_operation_arg_free(fmt);

    /* Freeze both endpoints before collecting paths. A failed or unknown base is
     * a request error, never an exact-looking empty diff, and a concurrent HEAD
     * advance cannot mix revisions within one answer. */
    char head_oid[65] = "";
    char base_oid[65] = "";
    char git_prefix[CBM_SZ_4K] = "";
    char cmd[CBM_SZ_2K];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd),
             "git -C \"%s\" rev-parse --show-prefix \"HEAD^{commit}\" \"%s^{commit}\" 2>NUL",
             root_path, base_branch);
#else
    snprintf(cmd, sizeof(cmd),
             "git -C '%s' rev-parse --show-prefix 'HEAD^{commit}' '%s^{commit}' 2>/dev/null",
             root_path, base_branch);
#endif
    changes_git_run(runtime, cmd, &git);
    bool resolve_oom = false;
    bool resolve_ok = false;
    if (git.fp) {
        bool terminated = false;
        char *prefix = detect_read_record(git.fp, '\n', &resolve_oom, &terminated);
        size_t prefix_len = prefix ? strlen(prefix) : 0;
        if (prefix_len && prefix[prefix_len - 1] == '\r')
            prefix[--prefix_len] = '\0';
        bool prefix_ok = prefix && terminated && prefix_len < sizeof(git_prefix) &&
                         (!prefix_len || prefix[prefix_len - 1] == '/');
        if (prefix_ok)
            memcpy(git_prefix, prefix, prefix_len + 1U);
        free(prefix);
        bool head_ok = changes_read_oid(git.fp, head_oid, &resolve_oom);
        bool base_ok = changes_read_oid(git.fp, base_oid, &resolve_oom);
        resolve_ok = prefix_ok && head_ok && base_ok && !resolve_oom;
    }
    if (!resolve_ok) {
        bool cancelled = git.cancelled;
        int run = git.run;
        changes_git_finish(&git);
        result = changes_error(
            cancelled  ? "detect_changes cancelled for this request"
            : run != 0 ? "git revision resolution failed: the contained command could not complete"
                       : "git revision resolution failed: base_branch or HEAD is not a commit");
        goto done;
    }
    changes_git_finish(&git);

    char merge_base[65] = "";
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "git -C \"%s\" merge-base \"%s\" \"%s\" 2>NUL", root_path, base_oid,
             head_oid);
#else
    snprintf(cmd, sizeof(cmd), "git -C '%s' merge-base '%s' '%s' 2>/dev/null", root_path, base_oid,
             head_oid);
#endif
    changes_git_run(runtime, cmd, &git);
    bool mb_oom = false;
    bool mb_ok = git.fp && changes_read_oid(git.fp, merge_base, &mb_oom) && !mb_oom;
    if (!mb_ok) {
        bool cancelled = git.cancelled;
        int run = git.run;
        changes_git_finish(&git);
        result = changes_error(
            cancelled  ? "detect_changes cancelled for this request"
            : run != 0 ? "git merge-base failed: the contained command could not complete"
                       : "git merge-base failed: base_branch has no common ancestor with HEAD");
        goto done;
    }
    changes_git_finish(&git);

    /* Collect exact Git path records. `-z` disables C-style path quoting and
     * keeps embedded newlines intact. Diff and porcelain status stay separate:
     * diff emits bare paths, while status emits typed `XY destination` records
     * and an extra source record for renames/copies. */
    bool changed_path_oom = false;
    bool changed_path_malformed = false;
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd),
             "git -c core.quotePath=false -C \"%s\" diff --relative --name-only -z \"%s\" \"%s\" "
             "-- 2>NUL "
             "&& git -c core.quotePath=false -C \"%s\" diff --relative --name-only -z -- 2>NUL",
             root_path, merge_base, head_oid, root_path);
#else
    snprintf(cmd, sizeof(cmd),
             "git -c core.quotePath=false -C '%s' diff --relative --name-only -z '%s' '%s' -- "
             "2>/dev/null "
             "&& git -c core.quotePath=false -C '%s' diff --relative --name-only -z -- 2>/dev/null",
             root_path, merge_base, head_oid, root_path);
#endif
    changes_git_run(runtime, cmd, &git);
    if (git.fp) {
        for (;;) {
            bool terminated = false;
            char *record = detect_read_record(git.fp, '\0', &changed_path_oom, &terminated);
            if (!record) {
                break;
            }
            if (!terminated) {
                changed_path_malformed = true;
                free(record);
                break;
            }
            if (!detect_add_changed_path(&files, &file_count, &file_cap, record)) {
                changed_path_oom = true;
                free(record);
                break;
            }
            free(record);
        }
    }
    if (!changes_git_ok(&git) || changed_path_oom || changed_path_malformed) {
        bool cancelled = git.cancelled;
        int run = git.run;
        changes_git_finish(&git);
        result =
            changes_error(cancelled                ? "detect_changes cancelled for this request"
                          : changed_path_oom       ? "out of memory while reading changed paths"
                          : changed_path_malformed ? "git diff returned a malformed path record"
                          : run != 0 ? "git diff failed: the contained command could not complete"
                                     : "git diff failed while collecting changed paths");
        goto done;
    }
    changes_git_finish(&git);

#ifdef _WIN32
    snprintf(cmd, sizeof(cmd),
             "git --no-optional-locks -c core.quotePath=false -C \"%s\" status "
             "--porcelain=v1 -z --untracked-files=all -- 2>NUL",
             root_path);
#else
    snprintf(cmd, sizeof(cmd),
             "git --no-optional-locks -c core.quotePath=false -C '%s' status "
             "--porcelain=v1 -z --untracked-files=all -- 2>/dev/null",
             root_path);
#endif
    changes_git_run(runtime, cmd, &git);
    if (git.fp) {
        for (;;) {
            bool terminated = false;
            char *record = detect_read_record(git.fp, '\0', &changed_path_oom, &terminated);
            if (!record) {
                break;
            }
            size_t length = strlen(record);
            bool typed =
                terminated && length > CHANGES_PAIR_LEN + 1U && record[CHANGES_PAIR_LEN] == ' ';
            bool rename_or_copy = typed && (record[0] == 'R' || record[0] == 'C' ||
                                            record[1] == 'R' || record[1] == 'C');
            if (!typed) {
                changed_path_malformed = true;
                free(record);
                break;
            }
            const char *git_path = record + CHANGES_PAIR_LEN + 1U;
            size_t prefix_len = strlen(git_prefix);
            const char *project_path =
                strncmp(git_path, git_prefix, prefix_len) == 0 ? git_path + prefix_len : NULL;
            if (project_path &&
                !detect_add_changed_path(&files, &file_count, &file_cap, project_path)) {
                changed_path_oom = true;
                free(record);
                break;
            }
            free(record);
            if (rename_or_copy) {
                bool source_terminated = false;
                char *source =
                    detect_read_record(git.fp, '\0', &changed_path_oom, &source_terminated);
                if (!source || !source_terminated || !source[0]) {
                    free(source);
                    changed_path_malformed = !changed_path_oom;
                    break;
                }
                free(source);
            }
        }
    }
    if (!changes_git_ok(&git) || changed_path_oom || changed_path_malformed) {
        bool cancelled = git.cancelled;
        int run = git.run;
        changes_git_finish(&git);
        result =
            changes_error(cancelled                ? "detect_changes cancelled for this request"
                          : changed_path_oom       ? "out of memory while reading changed paths"
                          : changed_path_malformed ? "git status returned a malformed path record"
                          : run != 0 ? "git status failed: the contained command could not complete"
                                     : "git status failed while collecting changed paths");
        goto done;
    }
    changes_git_finish(&git);
    if (file_count > 1) {
        qsort(files, (size_t)file_count, sizeof(*files), detect_changed_path_compare);
    }

    /* Per-symbol impact page size. Engine saturation makes the reported total an
     * explicit lower bound, while impact_offset continues every materialized
     * row without identifier truncation. */
    int imp_limit = changes_int_arg(args, "limit", CHANGES_DEFAULT_IMPACT_LIMIT);
    if (imp_limit < 1) {
        imp_limit = 1;
    }
    if (imp_limit > CHANGES_BFS_LIMIT_MAX) {
        imp_limit = CHANGES_BFS_LIMIT_MAX;
    }
    int impact_offset = changes_int_arg(args, "impact_offset", 0);
    if (impact_offset < 0) {
        impact_offset = 0;
    }
    int changed_limit = changes_int_arg(args, "changed_limit", CHANGES_DEFAULT_PAGE_LIMIT);
    int changed_offset = changes_int_arg(args, "changed_offset", 0);
    if (changed_limit < 0) {
        changed_limit = 0;
    } else if (changed_limit > CHANGES_BFS_LIMIT_MAX) {
        changed_limit = CHANGES_BFS_LIMIT_MAX;
    }
    if (changed_offset < 0) {
        changed_offset = 0;
    }
    /* Absent max_output_tokens = no byte budget: detect_changes keeps emitting
     * exactly the response it always has. */
    size_t output_budget_bytes = cbm_output_budget_bytes(cbm_output_budget_tokens(args, 0));
    int module_limit = changes_int_arg(args, "module_limit", CHANGES_DEFAULT_PAGE_LIMIT);
    int module_offset = changes_int_arg(args, "module_offset", 0);
    if (module_limit < 0) {
        module_limit = 0;
    } else if (module_limit > DETECT_MODCAP) {
        module_limit = DETECT_MODCAP;
    }
    if (module_offset < 0) {
        module_offset = 0;
    }
    impact_cursor_arg = changes_string_arg(args, "impact_cursor");
    changed_cursor_arg = changes_string_arg(args, "changed_cursor");
    module_cursor_arg = changes_string_arg(args, "module_cursor");
    uint64_t detect_qhash = detect_params_hash(project, base_branch,
                                               want_symbols ? "impact" : "files", direction, depth);

    /* Hunk line ranges (unified=0 diff), used to scope seed detection to the
     * actually-changed lines instead of every definition in a changed file
     * (see detect_collect_seeds). Best-effort: any failure here just leaves
     * `hunks` empty and every file falls back to its previous whole-file
     * seeding — this is a precision improvement, not a correctness
     * dependency, so it is never treated as a request-level failure.
     *
     * Coordinate systems: `base...HEAD` hunks carry HEAD-side line numbers,
     * the worktree diff carries worktree-side ones, and node line ranges come
     * from the indexed snapshot. These agree while the index is fresh — the
     * watcher reindexes on HEAD movement and on a dirty tree — but a stale
     * index combined with insertions earlier in the file shifts the node lines
     * relative to the hunks and can mis-scope. The failure is bounded by
     * detect_collect_seeds' zero-overlap fallback: a file whose definitions all
     * miss reverts to whole-file seeding rather than dropping out. */
    if (want_symbols) {
#ifdef _WIN32
        snprintf(cmd, sizeof(cmd),
                 "git -C \"%s\" diff --relative --unified=0 \"%s\" \"%s\" -- 2>NUL && "
                 "git -C \"%s\" diff --relative --unified=0 -- 2>NUL",
                 root_path, merge_base, head_oid, root_path);
#else
        snprintf(cmd, sizeof(cmd),
                 "git -C '%s' diff --relative --unified=0 '%s' '%s' -- 2>/dev/null && "
                 "git -C '%s' diff --relative --unified=0 -- 2>/dev/null",
                 root_path, merge_base, head_oid, root_path);
#endif
        changes_git_run(runtime, cmd, &git);
        if (git.fp) {
            (void)fseek(git.fp, 0, SEEK_END);
            long hsz = ftell(git.fp);
            if (hsz > 0) {
                (void)fseek(git.fp, 0, SEEK_SET);
                char *hbuf = malloc((size_t)hsz + CHANGES_SKIP_ONE);
                if (hbuf) {
                    size_t hread = fread(hbuf, CHANGES_SKIP_ONE, (size_t)hsz, git.fp);
                    hbuf[hread] = '\0';
                    enum { HUNK_CAP = 4096 };
                    hunks = changes_realloc(NULL, (size_t)HUNK_CAP * sizeof(cbm_changed_hunk_t));
                    hunk_count = cbm_parse_hunks(hbuf, hunks, HUNK_CAP);
                    /* A filled buffer means the diff was truncated: the hunks
                     * past the cap are gone, so files captured only partially
                     * would still look scoped and silently under-seed. Drop
                     * scoping for the whole request rather than under-report a
                     * large refactor — whole-file seeding is the safe side. */
                    if (hunk_count >= HUNK_CAP) {
                        cbm_log_info("detect_changes.hunks", "action", "scoping_disabled", "reason",
                                     "hunk_cap_reached");
                        free(hunks);
                        hunks = NULL;
                        hunk_count = 0;
                    }
                    free(hbuf);
                }
            }
        }
        bool hunk_cancelled = git.cancelled;
        changes_git_finish(&git);
        if (hunk_cancelled) {
            result = changes_error("detect_changes cancelled for this request");
            goto done;
        }
        /* Hunk parsing is deliberately best-effort, but path collection is not.
         * Seed after both are complete so every exact path uses the same scoping
         * decision. */
        for (int i = 0; i < file_count; i++) {
            detect_collect_seeds(store, project, files[i], hunks, hunk_count, &seeds, &seed_count,
                                 &seed_cap);
        }
    }

    /* The impact traversal: ONE multi-source BFS over all seeds. */
    bool engine_saturated = false;
    if (want_symbols && seed_count > 0) {
        (void)cbm_store_bfs_multi(store, seeds, seed_count, direction, NULL, 0, depth,
                                  CHANGES_BFS_LIMIT_MAX, &impact, &engine_saturated);
    }

    int module_overflow = 0;
    if (want_symbols && impact.visited_count > 0) {
        modules = calloc(DETECT_MODCAP, sizeof(*modules));
        if (modules) {
            nmods = detect_module_rollup(&impact, modules, &module_overflow);
        }
    }
    /* Overflow is one explicit aggregate row: the exact number of pageable
     * rollup rows for the materialized impact set. Its relation becomes `gte`
     * if the traversal hit the engine ceiling. */
    int module_total = nmods + (module_overflow > 0 ? 1 : 0);

    char detect_snapshot[33] = "";
    bool detect_snapshot_complete = detect_snapshot_fingerprint(
        root_path, head_oid, base_oid, merge_base, generation, files, file_count, &impact, modules,
        nmods, module_overflow, detect_snapshot);

    const char *cursor_error = NULL;
    detect_cursor_t decoded_cursor = {0};
    bool cursor_supplied = (changed_cursor_arg && changed_cursor_arg[0]) ||
                           (impact_cursor_arg && impact_cursor_arg[0]) ||
                           (module_cursor_arg && module_cursor_arg[0]);
    if (cursor_supplied && !detect_snapshot_complete) {
        cursor_error = "snapshot_unavailable: changed file bytes could not be fingerprinted - "
                       "rerun without the cursor after the files are readable";
    }
    if (!cursor_error && changed_cursor_arg && changed_cursor_arg[0]) {
        if (changed_offset != 0) {
            cursor_error = "cursor_params_mismatch: changed_cursor cannot be combined with a "
                           "nonzero changed_offset";
        } else {
            cursor_error = detect_cursor_decode(changed_cursor_arg, 'c', detect_snapshot,
                                                detect_qhash, &decoded_cursor);
            if (!cursor_error) {
                changed_offset = decoded_cursor.offset;
            }
        }
    }
    if (!cursor_error && impact_cursor_arg && impact_cursor_arg[0]) {
        if (impact_offset != 0) {
            cursor_error = "cursor_params_mismatch: impact_cursor cannot be combined with a "
                           "nonzero impact_offset";
        } else {
            cursor_error = detect_cursor_decode(impact_cursor_arg, 'i', detect_snapshot,
                                                detect_qhash, &decoded_cursor);
            if (!cursor_error) {
                impact_offset = decoded_cursor.offset;
            }
        }
    }
    if (!cursor_error && module_cursor_arg && module_cursor_arg[0]) {
        if (module_offset != 0) {
            cursor_error = "cursor_params_mismatch: module_cursor cannot be combined with a "
                           "nonzero module_offset";
        } else {
            cursor_error = detect_cursor_decode(module_cursor_arg, 'm', detect_snapshot,
                                                detect_qhash, &decoded_cursor);
            if (!cursor_error) {
                module_offset = decoded_cursor.offset;
            }
        }
    }
    if (cursor_error) {
        result = changes_error(cursor_error);
        goto done;
    }

    int changed_start = changed_offset < file_count ? changed_offset : file_count;
    int changed_returned = file_count - changed_start;
    if (changed_returned > changed_limit) {
        changed_returned = changed_limit;
    }
    int module_start = module_offset < module_total ? module_offset : module_total;
    int module_returned = module_total - module_start;
    if (module_returned > module_limit) {
        module_returned = module_limit;
    }
    int imp_start = impact_offset < impact.visited_count ? impact_offset : impact.visited_count;
    int imp_returned = impact.visited_count - imp_start;
    if (imp_returned > imp_limit) {
        imp_returned = imp_limit;
    }
    bool output_budget_hit = false;
    bool output_budget_floor_exceeded = false;

render_detect_output:;
    bool changed_has_more = changed_start + changed_returned < file_count;
    bool impacted_has_more = want_symbols && imp_start + imp_returned < impact.visited_count;
    bool module_has_more = want_symbols && module_start + module_returned < module_total;
    bool response_truncated = engine_saturated || output_budget_hit || changed_has_more ||
                              impacted_has_more || module_has_more;

    if (!legacy_json) {
        cbm_sb_t sb;
        cbm_sb_init(&sb);
        cbm_tree_scalar_str(&sb, "base", base_branch);
        cbm_tree_scalar_bool(&sb, "working_tree_included", true);
        cbm_tree_scalar_str(&sb, "merge_base", merge_base);
        cbm_tree_scalar_str(&sb, "direction", direction);
        if (output_budget_hit) {
            cbm_tree_scalar_str(&sb, "truncation_reason", "output_budget");
            cbm_tree_scalar_int(&sb, "max_output_bytes", (long long)output_budget_bytes);
        }
        if (output_budget_floor_exceeded) {
            cbm_tree_scalar_bool(&sb, "output_budget_floor_exceeded", true);
        }
        if (engine_saturated) {
            cbm_tree_scalar_bool(&sb, "engine_saturated", true);
        }
        if (!detect_snapshot_complete) {
            cbm_tree_scalar_bool(&sb, "snapshot_cursor_unavailable", true);
        }
        /* Changed files are independently pageable: traversal still used every
         * file, so paging affects presentation only, never the graph answer. */
        cbm_tree_scalar_int(&sb, "changed_total", file_count);
        cbm_tree_scalar_int(&sb, "changed_returned", changed_returned);
        cbm_tree_scalar_bool(&sb, "changed_has_more", changed_has_more);
        if (changed_has_more && changed_returned > 0) {
            cbm_tree_scalar_int(&sb, "changed_next_offset", changed_start + changed_returned);
            if (detect_snapshot_complete) {
                char cursor[80];
                detect_cursor_encode('c', detect_snapshot, detect_qhash,
                                     changed_start + changed_returned, cursor);
                cbm_tree_scalar_str(&sb, "changed_next_cursor", cursor);
            }
        } else if (changed_has_more) {
            cbm_tree_scalar_bool(&sb, "changed_continuation_requires_positive_limit", true);
        }
        static const char *const changed_columns[] = {"path"};
        const char **changed_cells =
            changed_returned > 0 ? calloc((size_t)changed_returned, sizeof(*changed_cells)) : NULL;
        if (changed_returned == 0 || changed_cells) {
            for (int row = 0; row < changed_returned; row++) {
                changed_cells[row] = files[changed_start + row];
            }
            static const bool changed_string_columns[] = {true};
            static const bool changed_prefix_columns[] = {true};
            cbm_tree_table_rows_profiled(&sb, "changed_files", changed_returned, changed_columns, 1,
                                         changed_cells, changed_string_columns,
                                         changed_prefix_columns);
        } else {
            cbm_tree_table_header(&sb, "changed_files", 0, changed_columns, 1);
            cbm_tree_scalar_str(&sb, "changed_files_render_error", "out_of_memory");
        }
        free(changed_cells);
        cbm_tree_scalar_int(&sb, "seed_symbols", seed_count);
        if (want_symbols) {
            detect_emit_impacted_tree(&sb, &impact, imp_start, imp_returned, engine_saturated);
            if (detect_snapshot_complete && impacted_has_more && imp_returned > 0) {
                char cursor[80];
                detect_cursor_encode('i', detect_snapshot, detect_qhash, imp_start + imp_returned,
                                     cursor);
                cbm_tree_scalar_str(&sb, "impacted_next_cursor", cursor);
            }
            /* module rollup: independently pageable, while counts are still
             * computed from the complete impact set. */
            cbm_tree_scalar_int(&sb, "module_total", module_total);
            cbm_tree_scalar_str(&sb, "module_total_relation", engine_saturated ? "gte" : "eq");
            cbm_tree_scalar_int(&sb, "module_returned", module_returned);
            cbm_tree_scalar_bool(&sb, "module_has_more", module_has_more);
            if (module_has_more && module_returned > 0) {
                cbm_tree_scalar_int(&sb, "module_next_offset", module_start + module_returned);
                if (detect_snapshot_complete) {
                    char cursor[80];
                    detect_cursor_encode('m', detect_snapshot, detect_qhash,
                                         module_start + module_returned, cursor);
                    cbm_tree_scalar_str(&sb, "module_next_cursor", cursor);
                }
            } else if (module_has_more) {
                cbm_tree_scalar_bool(&sb, "module_continuation_requires_positive_limit", true);
            }
            static const char *const columns[] = {"module", "count"};
            const char **cells =
                module_returned > 0 ? calloc((size_t)module_returned * 2U, sizeof(*cells)) : NULL;
            char (*count_text)[32] =
                module_returned > 0 ? calloc((size_t)module_returned, sizeof(*count_text)) : NULL;
            if (module_returned == 0 || (cells && count_text)) {
                for (int row = 0; row < module_returned; row++) {
                    int j = module_start + row;
                    cells[(size_t)row * 2U] = j < nmods ? modules[j].name : "(other)";
                    snprintf(count_text[row], sizeof(count_text[row]), "%d",
                             j < nmods ? modules[j].count : module_overflow);
                    cells[(size_t)row * 2U + 1U] = count_text[row];
                }
                static const bool string_cols[] = {true, false};
                static const bool prefix_cols[] = {true, false};
                cbm_tree_table_rows_profiled(&sb, "impacted_modules", module_returned, columns, 2,
                                             cells, string_cols, prefix_cols);
            } else {
                cbm_tree_table_header(&sb, "impacted_modules", 0, columns, 2);
                cbm_tree_scalar_str(&sb, "module_render_error", "out_of_memory");
            }
            free(count_text);
            free(cells);
            if (engine_saturated) {
                cbm_tree_scalar_str(&sb, "hint",
                                    "impact hit the safety ceiling — narrow with a lower "
                                    "'depth' or a smaller diff");
            }
        }
        if (response_truncated) {
            cbm_tree_scalar_bool(&sb, "truncated", true);
        }
        out_str = cbm_sb_finish(&sb);
    } else {
        /* format:"json" = json-stringified tree: same model, structured. */
        yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
        yyjson_mut_val *root_obj = doc ? yyjson_mut_obj(doc) : NULL;
        if (!root_obj) {
            if (doc) {
                yyjson_mut_doc_free(doc);
            }
            result = changes_error("result allocation failed");
            goto done;
        }
        yyjson_mut_doc_set_root(doc, root_obj);
        yyjson_mut_obj_add_strcpy(doc, root_obj, "base", base_branch);
        yyjson_mut_obj_add_bool(doc, root_obj, "working_tree_included", true);
        yyjson_mut_obj_add_strcpy(doc, root_obj, "merge_base", merge_base);
        yyjson_mut_obj_add_strcpy(doc, root_obj, "direction", direction);
        if (output_budget_hit) {
            yyjson_mut_obj_add_str(doc, root_obj, "truncation_reason", "output_budget");
            yyjson_mut_obj_add_uint(doc, root_obj, "max_output_bytes", output_budget_bytes);
        }
        if (output_budget_floor_exceeded) {
            yyjson_mut_obj_add_bool(doc, root_obj, "output_budget_floor_exceeded", true);
        }
        if (engine_saturated) {
            yyjson_mut_obj_add_bool(doc, root_obj, "engine_saturated", true);
        }
        if (!detect_snapshot_complete) {
            yyjson_mut_obj_add_bool(doc, root_obj, "snapshot_cursor_unavailable", true);
        }
        yyjson_mut_obj_add_int(doc, root_obj, "changed_total", file_count);
        yyjson_mut_obj_add_int(doc, root_obj, "changed_returned", changed_returned);
        yyjson_mut_obj_add_bool(doc, root_obj, "changed_has_more", changed_has_more);
        if (changed_has_more && changed_returned > 0) {
            yyjson_mut_obj_add_int(doc, root_obj, "changed_next_offset",
                                   changed_start + changed_returned);
            if (detect_snapshot_complete) {
                char cursor[80];
                detect_cursor_encode('c', detect_snapshot, detect_qhash,
                                     changed_start + changed_returned, cursor);
                yyjson_mut_obj_add_strcpy(doc, root_obj, "changed_next_cursor", cursor);
            }
        } else if (changed_has_more) {
            yyjson_mut_obj_add_bool(doc, root_obj, "changed_continuation_requires_positive_limit",
                                    true);
        }
        yyjson_mut_val *cf = yyjson_mut_arr(doc);
        for (int i = changed_start; i < changed_start + changed_returned; i++) {
            yyjson_mut_arr_add_strcpy(doc, cf, files[i]);
        }
        yyjson_mut_obj_add_val(doc, root_obj, "changed_files", cf);
        yyjson_mut_obj_add_int(doc, root_obj, "seed_symbols", seed_count);
        yyjson_mut_obj_add_int(doc, root_obj, "impacted_total", impact.visited_count);
        yyjson_mut_obj_add_str(doc, root_obj, "impacted_total_relation",
                               engine_saturated ? "gte" : "eq");
        yyjson_mut_obj_add_int(doc, root_obj, "impacted_shown", imp_returned);
        yyjson_mut_val *imp = yyjson_mut_arr(doc);
        for (int i = imp_start; i < imp_start + imp_returned; i++) {
            yyjson_mut_val *o = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_strcpy(
                doc, o, "qn",
                impact.visited[i].node.qualified_name ? impact.visited[i].node.qualified_name : "");
            yyjson_mut_obj_add_strcpy(
                doc, o, "label", impact.visited[i].node.label ? impact.visited[i].node.label : "");
            yyjson_mut_obj_add_strcpy(
                doc, o, "file",
                impact.visited[i].node.file_path ? impact.visited[i].node.file_path : "");
            yyjson_mut_obj_add_int(doc, o, "hop", impact.visited[i].hop);
            yyjson_mut_arr_add_val(imp, o);
        }
        yyjson_mut_obj_add_val(doc, root_obj, "impacted", imp);
        yyjson_mut_obj_add_bool(doc, root_obj, "impacted_has_more", impacted_has_more);
        if (impacted_has_more && imp_returned > 0) {
            yyjson_mut_obj_add_int(doc, root_obj, "impacted_next_offset", imp_start + imp_returned);
            if (detect_snapshot_complete) {
                char cursor[80];
                detect_cursor_encode('i', detect_snapshot, detect_qhash, imp_start + imp_returned,
                                     cursor);
                yyjson_mut_obj_add_strcpy(doc, root_obj, "impacted_next_cursor", cursor);
            }
        }
        /* Model parity with the tree encoding: complete totals, paged rows. */
        if (want_symbols) {
            yyjson_mut_obj_add_int(doc, root_obj, "module_total", module_total);
            yyjson_mut_obj_add_str(doc, root_obj, "module_total_relation",
                                   engine_saturated ? "gte" : "eq");
            yyjson_mut_obj_add_int(doc, root_obj, "module_returned", module_returned);
            yyjson_mut_obj_add_bool(doc, root_obj, "module_has_more", module_has_more);
            if (module_has_more && module_returned > 0) {
                yyjson_mut_obj_add_int(doc, root_obj, "module_next_offset",
                                       module_start + module_returned);
                if (detect_snapshot_complete) {
                    char cursor[80];
                    detect_cursor_encode('m', detect_snapshot, detect_qhash,
                                         module_start + module_returned, cursor);
                    yyjson_mut_obj_add_strcpy(doc, root_obj, "module_next_cursor", cursor);
                }
            } else if (module_has_more) {
                yyjson_mut_obj_add_bool(doc, root_obj,
                                        "module_continuation_requires_positive_limit", true);
            }
            yyjson_mut_val *rollup = yyjson_mut_arr(doc);
            for (int j = module_start; j < module_start + module_returned; j++) {
                yyjson_mut_val *o = yyjson_mut_obj(doc);
                if (j < nmods) {
                    yyjson_mut_obj_add_strcpy(doc, o, "module", modules[j].name);
                    yyjson_mut_obj_add_int(doc, o, "count", modules[j].count);
                } else {
                    yyjson_mut_obj_add_str(doc, o, "module", "(other)");
                    yyjson_mut_obj_add_int(doc, o, "count", module_overflow);
                }
                yyjson_mut_arr_add_val(rollup, o);
            }
            yyjson_mut_obj_add_val(doc, root_obj, "impacted_modules", rollup);
        }
        yyjson_mut_obj_add_bool(doc, root_obj, "truncated", response_truncated);
        out_str = changes_doc_to_str(doc);
        yyjson_mut_doc_free(doc);
    }
    /* Exact serialized-size check with semantic reductions only. Low-value
     * rollups and file names yield as whole sections before graph rows, and
     * every continuation above remains exactly valid for the rows emitted.
     * Nothing is ever byte-sliced. */
    if (output_budget_bytes > 0 && out_str && strlen(out_str) > output_budget_bytes) {
        output_budget_hit = true;
        if (module_returned > 0) {
            module_returned = 0;
        } else if (changed_returned > 0) {
            changed_returned = 0;
        } else if (imp_returned > 0) {
            /* Prefix-directory factoring can make N rows smaller than N-1, so
             * probe every smaller whole-row prefix. */
            imp_returned--;
        } else if (!output_budget_floor_exceeded) {
            output_budget_floor_exceeded = true;
        } else {
            free(out_str);
            out_str = detect_budget_floor(legacy_json, engine_saturated, output_budget_bytes);
            goto detect_output_done;
        }
        free(out_str);
        out_str = NULL;
        goto render_detect_output;
    }

detect_output_done:
    result = out_str ? cbm_operation_result_take(out_str, false)
                     : changes_error("out of memory while rendering detect_changes");
    out_str = NULL;

done:
    changes_git_finish(&git);
    free(out_str);
    cbm_store_traverse_free(&impact);
    detect_module_rollup_free(modules, nmods);
    for (int i = 0; i < file_count; i++) {
        free(files[i]);
    }
    free(files);
    free(seeds);
    free(hunks);
    cbm_operation_arg_free(impact_cursor_arg);
    cbm_operation_arg_free(changed_cursor_arg);
    cbm_operation_arg_free(module_cursor_arg);
    cbm_operation_arg_free(direction);
    free(root_path);
    cbm_operation_arg_free(project);
    cbm_operation_arg_free(base_branch);
    cbm_operation_arg_free(scope);
    if (store) {
        cbm_store_close(store);
    }
    return result;
}
