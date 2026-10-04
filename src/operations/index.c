#include "operations/index.h"
#include "operations/result_wire.h"
#include "operations/cross_repo.h"
#include "operations/index_supervisor.h"
#include "operations/project_arg.h"

#include "foundation/compat_fs.h"
#include "foundation/compat.h"
#include "foundation/constants.h"
#include "foundation/dump_verify.h"
#include "foundation/log.h"
#include "foundation/mem.h"
#include "foundation/platform.h"
#include "foundation/str_util.h"
#include "foundation/workspace.h"
#include "pipeline/artifact.h"
#include "pipeline/pipeline.h"
#include "store/store.h"
#include "yyjson/yyjson.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static char *index_strdup(const char *text) {
    if (!text)
        return NULL;
    size_t len = strlen(text);
    char *copy = malloc(len + 1U);
    if (copy)
        memcpy(copy, text, len + 1U);
    return copy;
}

static char *index_string_arg(const char *args_json, const char *key) {
    const char *json = args_json ? args_json : "{}";
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, key) : NULL;
    char *out = yyjson_is_str(value) ? index_strdup(yyjson_get_str(value)) : NULL;
    if (doc)
        yyjson_doc_free(doc);
    return out;
}

static bool index_bool_arg(const char *args_json, const char *key) {
    const char *json = args_json ? args_json : "{}";
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, key) : NULL;
    bool out = yyjson_is_bool(value) && yyjson_get_bool(value);
    if (doc)
        yyjson_doc_free(doc);
    return out;
}

static bool index_db_path(const char *project, char *out, size_t out_size);

static const char *index_mode_name(const char *mode_text) {
    if (mode_text && !strcmp(mode_text, "fast")) {
        return "fast";
    }
    if (mode_text && !strcmp(mode_text, "moderate")) {
        return "moderate";
    }
    return "full";
}

static bool index_write_metrics(const char *path, const char *mode, const char *project,
                                const cbm_pipeline_t *pipeline, int rc) {
    if (!path || !path[0])
        return true;
    cbm_pipeline_metrics_t timing = {0};
    cbm_pipeline_get_metrics(pipeline, &timing);
    int nodes = 0, edges = 0;
    cbm_pipeline_get_committed_counts(pipeline, &nodes, &edges);
    char db[CBM_SZ_1K] = {0};
    struct stat st = {0};
    bool have_db = index_db_path(project, db, sizeof(db)) && stat(db, &st) == 0;
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    yyjson_mut_val *times = doc ? yyjson_mut_obj(doc) : NULL,
                   *waits = doc ? yyjson_mut_obj(doc) : NULL;
    yyjson_mut_val *memory = doc ? yyjson_mut_obj(doc) : NULL,
                   *counts = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root || !times || !waits || !memory || !counts) {
        if (doc)
            yyjson_mut_doc_free(doc);
        return false;
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_uint(doc, root, "schema_version", 1);
#ifdef CBM_VERSION
    yyjson_mut_obj_add_str(doc, root, "source_revision", CBM_VERSION);
#else
    yyjson_mut_obj_add_str(doc, root, "source_revision", "unknown");
#endif
    yyjson_mut_obj_add_str(doc, root, "index_mode", mode ? mode : "full");
    yyjson_mut_obj_add_str(doc, root, "outcome",
                           rc == 0                                ? "success"
                           : rc == CBM_PIPELINE_ABORT_PRESERVE_DB ? "cancelled"
                                                                  : "failed");
    yyjson_mut_obj_add_uint(doc, times, "discovery_ms", timing.discovery_ms);
    yyjson_mut_obj_add_uint(doc, times, "index_ms", timing.index_ms);
    yyjson_mut_obj_add_uint(doc, times, "staging_ms", timing.staging_ms);
    yyjson_mut_obj_add_uint(doc, times, "publish_ms", timing.publish_ms);
    yyjson_mut_obj_add_val(doc, root, "wall_times", times);
    yyjson_mut_obj_add_uint(doc, waits, "queue_ms", 0);
    yyjson_mut_obj_add_uint(doc, waits, "lock_ms", 0);
    yyjson_mut_obj_add_val(doc, root, "waits", waits);
    yyjson_mut_obj_add_uint(doc, memory, "worker_peak_rss_bytes", cbm_mem_peak_rss());
    yyjson_mut_obj_add_uint(doc, memory, "worker_aggregate_rss_bytes", cbm_mem_rss());
    yyjson_mut_obj_add_val(doc, root, "memory", memory);
    yyjson_mut_obj_add_int(doc, counts, "files", timing.files);
    yyjson_mut_obj_add_int(doc, counts, "definitions", nodes > 0 ? nodes : 0);
    yyjson_mut_obj_add_int(doc, counts, "edges", edges > 0 ? edges : 0);
    yyjson_mut_obj_add_uint(doc, counts, "database_bytes", have_db ? (uint64_t)st.st_size : 0);
    yyjson_mut_obj_add_val(doc, root, "counts", counts);
    char *json = yyjson_mut_write(doc, 0, NULL);
    yyjson_mut_doc_free(doc);
    if (!json)
        return false;
    char temporary[CBM_SZ_4K];
    int formatted = snprintf(temporary, sizeof(temporary), "%s.tmp.XXXXXX", path);
    int fd = formatted > 0 && (size_t)formatted < sizeof(temporary) ? cbm_mkstemp(temporary) : -1;
    FILE *file = fd >= 0 ? fdopen(fd, "wb") : NULL;
    bool wrote = file && fputs(json, file) >= 0;
    bool closed = file && fclose(file) == 0;
    bool ok = wrote && closed && cbm_rename_replace(temporary, path) == 0;
    if (!ok && fd >= 0)
        (void)cbm_unlink(temporary);
    free(json);
    return ok;
}

static cbm_operation_result_t index_text_error(const char *message) {
    return cbm_operation_result_copy(message ? message : "index operation failed", true);
}

static bool index_repo_path_is_absolute(const char *path) {
    if (!path || !path[0])
        return false;
#ifdef _WIN32
    return (path[0] == '/' && path[1] == '/') ||
           (isalpha((unsigned char)path[0]) && path[1] == ':' && path[2] == '/');
#else
    return path[0] == '/';
#endif
}

static char *index_canonicalize_repo_path(char *repo_path) {
    if (!repo_path)
        return NULL;
    bool root_syntax = true;
    for (const char *p = repo_path; *p; ++p) {
        if (*p != '/' && *p != '\\' && *p != ':') {
            root_syntax = false;
            break;
        }
    }
    if (root_syntax)
        return repo_path;
    char real[CBM_SZ_4K];
    if (cbm_canonical_path(repo_path, real, sizeof(real))) {
        cbm_normalize_path_sep(real);
        char *canonical = index_strdup(real);
        if (canonical) {
            free(repo_path);
            return canonical;
        }
    }
    return repo_path;
}

static bool index_resolve_session_path(const cbm_operation_runtime_t *runtime, char **repo_path) {
    if (!runtime || !repo_path || !*repo_path || !runtime->allowed_root_policy_set ||
        !runtime->session_root || !runtime->session_root[0] ||
        index_repo_path_is_absolute(*repo_path)) {
        return true;
    }
    size_t root_len = strlen(runtime->session_root);
    size_t path_len = strlen(*repo_path);
    bool sep = root_len > 0 && runtime->session_root[root_len - 1] != '/';
    if (root_len > SIZE_MAX - path_len - (sep ? 2U : 1U))
        return false;
    size_t size = root_len + (sep ? 1U : 0U) + path_len + 1U;
    char *joined = malloc(size);
    if (!joined)
        return false;
    (void)snprintf(joined, size, "%s%s%s", runtime->session_root, sep ? "/" : "", *repo_path);
    cbm_normalize_path_sep(joined);
    free(*repo_path);
    *repo_path = joined;
    return true;
}

static char *index_args_with_string(const char *args_json, const char *key, const char *value) {
    const char *json = args_json ? args_json : "{}";
    yyjson_doc *source = yyjson_read(json, strlen(json), 0);
    yyjson_val *source_root = source ? yyjson_doc_get_root(source) : NULL;
    if (!yyjson_is_obj(source_root)) {
        if (source)
            yyjson_doc_free(source);
        return NULL;
    }
    yyjson_mut_doc *copy = yyjson_doc_mut_copy(source, NULL);
    yyjson_doc_free(source);
    yyjson_mut_val *root = copy ? yyjson_mut_doc_get_root(copy) : NULL;
    if (!yyjson_mut_is_obj(root)) {
        if (copy)
            yyjson_mut_doc_free(copy);
        return NULL;
    }
    (void)yyjson_mut_obj_remove_key(root, key);
    if (!yyjson_mut_obj_add_strcpy(copy, root, key, value)) {
        yyjson_mut_doc_free(copy);
        return NULL;
    }
    char *out = yyjson_mut_write(copy, 0, NULL);
    yyjson_mut_doc_free(copy);
    return out;
}

static char *index_args_with_repo_path(const char *args_json, const char *repo_path) {
    return index_args_with_string(args_json, "repo_path", repo_path);
}

static bool index_db_path(const char *project, char *out, size_t out_size) {
    const char *cache = cbm_resolve_cache_dir();
    return project && cbm_validate_project_name(project) && cache && cache[0] &&
           snprintf(out, out_size, "%s/%s.db", cache, project) > 0 && strlen(out) < out_size;
}

static char *index_repo_path_from_project(const char *args_json) {
    char *project = cbm_operation_project_arg(args_json);
    if (!project)
        return NULL;
    char path[CBM_SZ_1K] = {0};
    char *root_path = NULL;
    if (index_db_path(project, path, sizeof(path))) {
        cbm_store_t *store = cbm_store_open_path_query(path);
        if (store) {
            cbm_project_t info = {0};
            if (cbm_store_get_project(store, project, &info) == CBM_STORE_OK && info.root_path) {
                root_path = index_strdup(info.root_path);
            }
            cbm_project_free_fields(&info);
            cbm_store_close(store);
        }
    }
    free(project);
    return root_path;
}

static bool index_project_has_adr(cbm_store_t *store, const char *project, const char *root_path) {
    cbm_adr_t adr = {0};
    if (store && project && cbm_store_adr_get(store, project, &adr) == CBM_STORE_OK) {
        cbm_store_adr_free(&adr);
        return true;
    }
    if (!root_path)
        return false;
    char path[CBM_SZ_4K];
    if (snprintf(path, sizeof(path), "%s/.codebase-memory/adr.md", root_path) >= (int)sizeof(path))
        return false;
    struct stat st;
    return stat(path, &st) == 0;
}

static void index_try_artifact_bootstrap(const char *project, const char *repo_path) {
    char db[CBM_SZ_1K] = {0};
    if (!index_db_path(project, db, sizeof(db)))
        return;
    if (cbm_file_size(db) < 0 && cbm_artifact_exists(repo_path)) {
        cbm_log_info("index.artifact_bootstrap", "project", project);
        if (cbm_artifact_import(repo_path, db) == 0) {
            (void)cbm_artifact_reconcile_hashes(repo_path, db, project);
        }
    }
}

enum { INDEX_EXCLUDED_DIR_CAP = 5, INDEX_SKIPPED_FILE_CAP = 5 };

static bool index_is_parse_partial(const cbm_file_error_t *entry) {
    return entry && entry->phase && strcmp(entry->phase, "parse_partial") == 0;
}

/* One range covers 80% or more of the file, so listing the lines is useless
 * advice. Also indexed, also not a skip. */
static bool index_is_parse_unusable(const cbm_file_error_t *entry) {
    return entry && entry->phase && strcmp(entry->phase, "parse_unusable") == 0;
}

/* Either coverage phase. Both mean the file WAS indexed, so both stay out of
 * skipped[]: a reader who sees a file there believes it is absent entirely. */
static bool index_is_parse_coverage(const cbm_file_error_t *entry) {
    return index_is_parse_partial(entry) || index_is_parse_unusable(entry);
}

static void index_add_excluded(yyjson_mut_doc *doc, yyjson_mut_val *root, char **dirs, int count) {
    if (!dirs || count <= 0)
        return;
    yyjson_mut_val *excluded = yyjson_mut_obj(doc);
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    int shown = count;
    if (shown > INDEX_EXCLUDED_DIR_CAP)
        shown = INDEX_EXCLUDED_DIR_CAP;
    for (int i = 0; i < shown; ++i)
        if (dirs[i])
            yyjson_mut_arr_add_strcpy(doc, arr, dirs[i]);
    yyjson_mut_obj_add_val(doc, excluded, "dirs", arr);
    yyjson_mut_obj_add_int(doc, excluded, "count", count);
    yyjson_mut_obj_add_bool(doc, excluded, "truncated", count > shown);
    yyjson_mut_obj_add_val(doc, root, "excluded", excluded);
}

static void index_add_ignored(yyjson_mut_doc *doc, yyjson_mut_val *root, cbm_pipeline_t *pipeline) {
    cbm_ignored_file_t *ignored = NULL;
    int stored = 0, total = 0;
    cbm_pipeline_get_ignored(pipeline, &ignored, &stored, &total);
    yyjson_mut_obj_add_int(doc, root, "not_indexed_files_count", total);
    if (!ignored || stored <= 0)
        return;
    yyjson_mut_val *obj = yyjson_mut_obj(doc);
    yyjson_mut_val *files = yyjson_mut_arr(doc);
    int shown = stored < INDEX_SKIPPED_FILE_CAP ? stored : INDEX_SKIPPED_FILE_CAP;
    for (int i = 0; i < shown; ++i) {
        yyjson_mut_val *entry = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_strcpy(doc, entry, "path",
                                  ignored[i].rel_path ? ignored[i].rel_path : "");
        yyjson_mut_obj_add_strcpy(doc, entry, "reason", ignored[i].reason ? ignored[i].reason : "");
        yyjson_mut_arr_add_val(files, entry);
    }
    yyjson_mut_obj_add_val(doc, obj, "files", files);
    yyjson_mut_obj_add_int(doc, obj, "count", total);
    yyjson_mut_obj_add_bool(doc, obj, "truncated", total > shown);
    yyjson_mut_obj_add_str(doc, obj, "note",
                           "Excluded by design (gitignore/.cbmignore/skip-lists); examples only — "
                           "full list in 'logfile'.");
    yyjson_mut_obj_add_val(doc, root, "not_indexed_files", obj);
}

static void index_add_skipped(yyjson_mut_doc *doc, yyjson_mut_val *root,
                              const cbm_file_error_t *errs, int count, const char *logfile) {
    int skips = 0;
    for (int i = 0; i < count; ++i)
        if (!index_is_parse_coverage(&errs[i]))
            ++skips;
    yyjson_mut_obj_add_int(doc, root, "skipped_count", skips);
    if (logfile && logfile[0])
        yyjson_mut_obj_add_strcpy(doc, root, "logfile", logfile);
    if (!errs || skips <= 0)
        return;
    yyjson_mut_val *obj = yyjson_mut_obj(doc);
    yyjson_mut_val *files = yyjson_mut_arr(doc);
    int shown = 0;
    for (int i = 0; i < count && shown < INDEX_SKIPPED_FILE_CAP; ++i) {
        if (index_is_parse_coverage(&errs[i]))
            continue;
        yyjson_mut_val *entry = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_strcpy(doc, entry, "path", errs[i].path ? errs[i].path : "");
        yyjson_mut_obj_add_strcpy(doc, entry, "reason", errs[i].reason ? errs[i].reason : "");
        yyjson_mut_obj_add_strcpy(doc, entry, "phase", errs[i].phase ? errs[i].phase : "");
        yyjson_mut_arr_add_val(files, entry);
        ++shown;
    }
    yyjson_mut_obj_add_val(doc, obj, "files", files);
    yyjson_mut_obj_add_int(doc, obj, "count", skips);
    yyjson_mut_obj_add_bool(doc, obj, "truncated", skips > INDEX_SKIPPED_FILE_CAP);
    yyjson_mut_obj_add_val(doc, root, "skipped", obj);
}

static void index_add_parse_partial(yyjson_mut_doc *doc, yyjson_mut_val *root,
                                    const cbm_file_error_t *errs, int count) {
    int partials = 0;
    for (int i = 0; i < count; ++i)
        if (index_is_parse_partial(&errs[i]))
            ++partials;
    yyjson_mut_obj_add_int(doc, root, "parse_partial_count", partials);
    if (!errs || partials <= 0)
        return;
    yyjson_mut_val *obj = yyjson_mut_obj(doc);
    yyjson_mut_val *files = yyjson_mut_arr(doc);
    int shown = 0;
    for (int i = 0; i < count && shown < INDEX_SKIPPED_FILE_CAP; ++i) {
        if (!index_is_parse_partial(&errs[i]))
            continue;
        yyjson_mut_val *entry = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_strcpy(doc, entry, "path", errs[i].path ? errs[i].path : "");
        yyjson_mut_obj_add_strcpy(doc, entry, "error_ranges", errs[i].reason ? errs[i].reason : "");
        yyjson_mut_arr_add_val(files, entry);
        ++shown;
    }
    yyjson_mut_obj_add_val(doc, obj, "files", files);
    yyjson_mut_obj_add_int(doc, obj, "count", partials);
    yyjson_mut_obj_add_bool(doc, obj, "truncated", partials > INDEX_SKIPPED_FILE_CAP);
    yyjson_mut_obj_add_str(
        doc, obj, "note",
        "Indexed, but constructs in these line ranges may be missing (best-effort signal); "
        "examples only — full list via index_status or 'logfile'.");
    yyjson_mut_obj_add_val(doc, root, "parse_partial", obj);
}

/* Whole-file half of the coverage summary. Always emits "parse_unusable_count"
 * (0 on clean runs) so a CI gate can read it without parsing anything else.
 * "range_end" is the last line the range names, not the file length: a grammar
 * can end an error node past the last line. */
static void index_add_parse_unusable(yyjson_mut_doc *doc, yyjson_mut_val *root,
                                     const cbm_file_error_t *errs, int count) {
    int unusable = 0;
    for (int i = 0; i < count; ++i)
        if (index_is_parse_unusable(&errs[i]))
            ++unusable;
    yyjson_mut_obj_add_int(doc, root, "parse_unusable_count", unusable);
    if (!errs || unusable <= 0)
        return;
    yyjson_mut_val *obj = yyjson_mut_obj(doc);
    yyjson_mut_val *files = yyjson_mut_arr(doc);
    int shown = 0;
    for (int i = 0; i < count && shown < INDEX_SKIPPED_FILE_CAP; ++i) {
        if (!index_is_parse_unusable(&errs[i]))
            continue;
        yyjson_mut_val *entry = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_strcpy(doc, entry, "path", errs[i].path ? errs[i].path : "");
        yyjson_mut_obj_add_bool(doc, entry, "whole_file", true);
        const char *dash = errs[i].reason ? strchr(errs[i].reason, '-') : NULL;
        yyjson_mut_obj_add_int(doc, entry, "range_end", dash ? atoi(dash + 1) : 0);
        yyjson_mut_arr_add_val(files, entry);
        ++shown;
    }
    yyjson_mut_obj_add_val(doc, obj, "files", files);
    yyjson_mut_obj_add_int(doc, obj, "count", unusable);
    yyjson_mut_obj_add_bool(doc, obj, "truncated", unusable > INDEX_SKIPPED_FILE_CAP);
    yyjson_mut_obj_add_str(doc, obj, "note",
                           "Indexed, but the parse failed across nearly the whole file, so line "
                           "ranges are not useful here - read the source directly.");
    yyjson_mut_obj_add_val(doc, root, "parse_unusable", obj);
}

static bool index_add_persisted_failures(yyjson_mut_doc *doc, yyjson_mut_val *root,
                                         cbm_store_t *store, const char *project,
                                         const char *logfile) {
    cbm_coverage_row_t *rows = NULL;
    int row_count = 0;
    if (cbm_store_coverage_get(store, project, &rows, &row_count) != CBM_STORE_OK)
        return false;
    int count = 0;
    for (int i = 0; i < row_count; ++i) {
        const char *kind = rows[i].kind ? rows[i].kind : "";
        if (strcmp(kind, "not_indexed_dir") && strcmp(kind, "not_indexed_file"))
            ++count;
    }
    cbm_file_error_t *failures = count ? calloc((size_t)count, sizeof(*failures)) : NULL;
    if (count && !failures) {
        cbm_store_free_coverage(rows, row_count);
        return false;
    }
    int n = 0;
    for (int i = 0; i < row_count; ++i) {
        const char *kind = rows[i].kind ? rows[i].kind : "";
        if (!strcmp(kind, "not_indexed_dir") || !strcmp(kind, "not_indexed_file"))
            continue;
        failures[n].path = (char *)rows[i].rel_path;
        failures[n].reason = (char *)rows[i].detail;
        failures[n].phase = (char *)rows[i].kind;
        ++n;
    }
    index_add_skipped(doc, root, failures, count, logfile);
    index_add_parse_partial(doc, root, failures, count);
    index_add_parse_unusable(doc, root, failures, count);
    free(failures);
    cbm_store_free_coverage(rows, row_count);
    return true;
}

static bool index_write_log(const char *project, const cbm_file_error_t *errs, int count, char *out,
                            size_t out_size) {
    if (!errs || count <= 0)
        return false;
    char path[CBM_SZ_1K];
    const char *override = getenv("CBM_INDEX_LOG");
    if (override && override[0]) {
        snprintf(path, sizeof(path), "%s", override);
    } else {
        const char *cache = cbm_resolve_cache_dir();
        if (!cache)
            return false;
        char dir[CBM_SZ_1K];
        snprintf(dir, sizeof(dir), "%s/logs", cache);
        cbm_mkdir_p_ex(dir, 0755, CBM_MKDIR_FOLLOW_OWNED);
        snprintf(path, sizeof(path), "%s/%s-%lld.log", dir, project ? project : "index",
                 (long long)time(NULL));
    }
    FILE *file = cbm_fopen(path, "wb");
    if (!file)
        return false;
    int partials = 0;
    for (int i = 0; i < count; ++i)
        if (index_is_parse_coverage(&errs[i]))
            ++partials;
    fprintf(file,
            "# codebase-memory-cli index coverage report\n# project=%s skipped=%d "
            "parse_partial=%d\n# columns: phase\treason\tpath\n",
            project ? project : "", count - partials, partials);
    for (int i = 0; i < count; ++i)
        fprintf(file, "%s\t%s\t%s\n", errs[i].phase ? errs[i].phase : "",
                errs[i].reason ? errs[i].reason : "", errs[i].path ? errs[i].path : "");
    fclose(file);
    if (out && out_size)
        snprintf(out, out_size, "%s", path);
    return true;
}

static bool index_build_success(yyjson_mut_doc *doc, yyjson_mut_val *root, const char *project,
                                const char *repo_path, bool persistence, cbm_pipeline_t *pipeline,
                                char **excluded_dirs, int excluded_count,
                                const cbm_file_error_t *file_errors, int file_error_count,
                                const char *logfile) {
    index_add_excluded(doc, root, excluded_dirs, excluded_count);
    index_add_ignored(doc, root, pipeline);
    int expected_nodes = -1, expected_edges = -1;
    cbm_pipeline_get_committed_counts(pipeline, &expected_nodes, &expected_edges);
    char db[CBM_SZ_1K] = {0};
    cbm_store_t *store =
        index_db_path(project, db, sizeof(db)) ? cbm_store_open_path_query(db) : NULL;
    if (!store || !index_add_persisted_failures(doc, root, store, project, logfile)) {
        index_add_skipped(doc, root, file_errors, file_error_count, logfile);
        index_add_parse_partial(doc, root, file_errors, file_error_count);
        index_add_parse_unusable(doc, root, file_errors, file_error_count);
    }
    int nodes = 0, edges = 0;
    bool degraded = false;
    if (!store) {
        degraded = true;
    } else {
        nodes = cbm_store_count_nodes(store, project);
        edges = cbm_store_count_edges(store, project);
        if (nodes < 0) {
            degraded = true;
            nodes = 0;
            if (edges < 0)
                edges = 0;
        } else if (cbm_dump_verify_is_degraded(expected_nodes, nodes, cbm_dump_verify_min_ratio(),
                                               CBM_DUMP_VERIFY_MIN_FLOOR)) {
            (void)cbm_store_checkpoint(store);
            int n2 = cbm_store_count_nodes(store, project),
                e2 = cbm_store_count_edges(store, project);
            if (n2 >= 0)
                nodes = n2;
            if (e2 >= 0)
                edges = e2;
            degraded = cbm_dump_verify_is_degraded(
                expected_nodes, nodes, cbm_dump_verify_min_ratio(), CBM_DUMP_VERIFY_MIN_FLOOR);
        }
    }
    yyjson_mut_obj_add_int(doc, root, "nodes", nodes);
    yyjson_mut_obj_add_int(doc, root, "edges", edges);
    if (expected_nodes >= 0) {
        yyjson_mut_obj_add_int(doc, root, "expected_nodes", expected_nodes);
        yyjson_mut_obj_add_int(doc, root, "expected_edges", expected_edges);
    }
    if (degraded) {
        yyjson_mut_obj_add_str(
            doc, root, "hint",
            store
                ? "Persisted far fewer nodes than indexed — likely durability loss from a "
                  "hard-killed sibling process. Re-run index_repository(repo_path=...) to rebuild."
                : "Index database failed integrity check and was removed. Re-run "
                  "index_repository(repo_path=...) to rebuild.");
    }
    bool adr = index_project_has_adr(store, project, repo_path);
    yyjson_mut_obj_add_bool(doc, root, "adr_present", adr);
    if (!adr && !degraded)
        yyjson_mut_obj_add_str(
            doc, root, "adr_hint",
            "Project indexed. Consider creating an Architecture Decision Record: explore the "
            "codebase with get_architecture(aspects=['all']), then use manage_adr(mode='update') "
            "to persist architectural insights across sessions.");
    bool artifact = cbm_artifact_exists(repo_path);
    yyjson_mut_obj_add_bool(doc, root, "artifact_present", artifact);
    if (persistence && artifact)
        yyjson_mut_obj_add_str(doc, root, "artifact_hint",
                               "Persistent artifact written to .codebase-memory/graph.db.zst. "
                               "Commit this file to share the index with teammates.");
    if (store)
        cbm_store_close(store);
    return degraded;
}

static void index_fill_run_response(yyjson_mut_doc *doc, yyjson_mut_val *root, const char *project,
                                    const char *repo_path, bool persistence,
                                    cbm_pipeline_t *pipeline, int rc, char **excluded,
                                    int excluded_count, const cbm_file_error_t *errors,
                                    int error_count, bool metrics_failed) {
    yyjson_mut_obj_add_strcpy(doc, root, "project", project ? project : "");
    if (rc == 0) {
        char logfile[CBM_SZ_1K] = {0};
        bool has_log = index_write_log(project, errors, error_count, logfile, sizeof(logfile));
        bool degraded =
            index_build_success(doc, root, project, repo_path, persistence, pipeline, excluded,
                                excluded_count, errors, error_count, has_log ? logfile : NULL);
        yyjson_mut_obj_add_str(doc, root, "status", degraded ? "degraded" : "indexed");
        if (metrics_failed) {
            yyjson_mut_obj_add_str(doc, root, "metrics_error", "failed to write metrics_out");
        }
        if (cbm_pipeline_had_format_migration(pipeline)) {
            yyjson_mut_obj_add_bool(doc, root, "format_migration", true);
        }
    } else if (rc == CBM_PIPELINE_ABORT_OVER_BUDGET) {
        /* Resident memory stayed above the budget after back-pressure and one
         * confirmation cycle, so the run stopped before publication. Name the
         * cause, the numbers and the fact that the previous index still serves. */
        int budget_mb = (int)(cbm_mem_budget() / (1024 * 1024));
        int peak_rss_mb = (int)(cbm_mem_peak_rss() / (1024 * 1024));
        char budget_text[CBM_SZ_32];
        char peak_text[CBM_SZ_32];
        (void)snprintf(budget_text, sizeof(budget_text), "%d", budget_mb);
        (void)snprintf(peak_text, sizeof(peak_text), "%d", peak_rss_mb);
        cbm_log_error("index.abort", "reason", "over_memory_budget", "project", project,
                      "budget_mb", budget_text, "peak_rss_mb", peak_text);
        yyjson_mut_obj_add_str(doc, root, "status", "error");
        yyjson_mut_obj_add_str(doc, root, "reason", "over_memory_budget");
        yyjson_mut_obj_add_str(doc, root, "previous_index", "preserved");
        yyjson_mut_obj_add_int(doc, root, "budget_mb", budget_mb);
        yyjson_mut_obj_add_int(doc, root, "peak_rss_mb", peak_rss_mb);
        /* peak_rss_mb is where the run was STOPPED (pinned just above the
         * budget), not what the repo needs, so retrying just above it fails
         * again. Suggest 1.5x the budget; (3*b+1)/2 stays strictly larger than
         * b for every positive budget, unlike b + b/2 at b == 1. */
        int suggested_budget_mb = budget_mb > 0 ? (budget_mb * 3 + 1) / 2 : 0;
        char hint_text[CBM_SZ_512];
        (void)snprintf(hint_text, sizeof(hint_text),
                       "Indexing stopped: resident memory stayed above the budget after "
                       "backpressure; no partial graph was published and the previous index "
                       "still serves. peak_rss_mb is where indexing was STOPPED, not what this "
                       "repo needs - the real requirement is higher, so retrying just above the "
                       "peak will fail again. Retry with CBM_MEM_BUDGET_MB=%d (1.5x the current "
                       "budget) if the machine has the RAM, or lower CBM_WORKERS, or exclude "
                       "large subtrees.",
                       suggested_budget_mb);
        if (suggested_budget_mb > 0) {
            yyjson_mut_obj_add_int(doc, root, "suggested_budget_mb", suggested_budget_mb);
        }
        yyjson_mut_obj_add_strcpy(doc, root, "hint", hint_text);
    } else if (rc == CBM_PIPELINE_ABORT_PRESERVE_DB) {
        /* Aborted pre-publication; the previous index is intact. */
        yyjson_mut_obj_add_str(doc, root, "status", "aborted_previous_preserved");
        yyjson_mut_obj_add_str(doc, root, "hint",
                               "Indexing aborted before publication; the previous index is "
                               "intact and still serving. Causes: files changed while the run "
                               "was in flight, or a discovery/manifest phase failed "
                               "transiently. Retry; if it repeats, check the run log.");
    } else if (rc == CBM_PIPELINE_PERSIST_FAILED) {
        yyjson_mut_obj_add_str(doc, root, "status", "persist_failed");
        yyjson_mut_obj_add_str(doc, root, "hint",
                               "The validated staging database could not be published. Check "
                               "free disk space and permissions on the cache directory; the "
                               "previous index may have been rolled back.");
    } else {
        yyjson_mut_obj_add_str(doc, root, "status", "error");
        yyjson_mut_obj_add_str(doc, root, "hint",
                               "Pipeline failed. Check repo_path exists and contains source files. "
                               "Try mode='fast' for a quicker diagnostic run.");
    }
}

static char *index_encode_run_response(const char *project, const char *repo_path, bool persistence,
                                       cbm_pipeline_t *pipeline, int rc, char **excluded,
                                       int excluded_count, const cbm_file_error_t *errors,
                                       int error_count, bool metrics_failed) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!root) {
        if (doc) {
            yyjson_mut_doc_free(doc);
        }
        return NULL;
    }
    yyjson_mut_doc_set_root(doc, root);
    index_fill_run_response(doc, root, project, repo_path, persistence, pipeline, rc, excluded,
                            excluded_count, errors, error_count, metrics_failed);
    char *payload = cbm_operation_json_write(doc);
    yyjson_mut_doc_free(doc);
    return payload;
}

static cbm_operation_result_t index_run_physical(const char *repo_path, const char *args_json,
                                                 const cbm_operation_runtime_t *runtime) {
    char *mode_text = index_string_arg(args_json, "mode");
    char *name = index_string_arg(args_json, "name");
    char *metrics_out = index_string_arg(args_json, "metrics_out");
    char *mutation_project = cbm_project_name_from_path(name && name[0] ? name : repo_path);
    if (!mutation_project) {
        free(mode_text);
        free(name);
        free(metrics_out);
        return index_text_error("could not resolve index project name");
    }
    if (!runtime || !runtime->mutation_begin || !runtime->mutation_end ||
        !runtime->mutation_begin(runtime->mutation_context, mutation_project)) {
        free(mode_text);
        free(name);
        free(metrics_out);
        free(mutation_project);
        return index_text_error("index operation blocked by another mutation for this project");
    }
    if (runtime->cancelled && runtime->cancelled(runtime->cancelled_context)) {
        (void)index_write_metrics(metrics_out, index_mode_name(mode_text), mutation_project, NULL,
                                  CBM_PIPELINE_ABORT_PRESERVE_DB);
        runtime->mutation_end(runtime->mutation_context, mutation_project);
        free(mode_text);
        free(name);
        free(metrics_out);
        free(mutation_project);
        return index_text_error("index operation cancelled for this request");
    }
    cbm_index_mode_t mode = CBM_MODE_FULL;
    if (mode_text && !strcmp(mode_text, "fast"))
        mode = CBM_MODE_FAST;
    else if (mode_text && !strcmp(mode_text, "moderate"))
        mode = CBM_MODE_MODERATE;
    const char *mode_name = index_mode_name(mode_text);
    free(mode_text);
    bool persistence = index_bool_arg(args_json, "persistence");
    cbm_pipeline_t *pipeline = cbm_pipeline_new(repo_path, NULL, mode);
    if (!pipeline) {
        (void)index_write_metrics(metrics_out, mode_name, mutation_project, NULL, CBM_NOT_FOUND);
        runtime->mutation_end(runtime->mutation_context, mutation_project);
        free(name);
        free(metrics_out);
        free(mutation_project);
        return index_text_error("failed to create pipeline");
    }
    if (name && name[0] && !cbm_pipeline_set_project_name(pipeline, name)) {
        cbm_pipeline_free(pipeline);
        runtime->mutation_end(runtime->mutation_context, mutation_project);
        free(name);
        free(metrics_out);
        free(mutation_project);
        return index_text_error("invalid project name");
    }
    free(name);
    cbm_pipeline_set_persistence(pipeline, persistence);
    char *project = index_strdup(cbm_pipeline_project_name(pipeline));
    index_try_artifact_bootstrap(project, repo_path);
    if (runtime->project_invalidate)
        runtime->project_invalidate(runtime->project_invalidate_context, project);
    cbm_pipeline_lock();
    if (runtime->cancel_flag)
        cbm_pipeline_bind_cancel_flag(pipeline, runtime->cancel_flag);
    int rc = cbm_pipeline_run(pipeline);
    cbm_pipeline_unlock();
    char **excluded = NULL;
    int excluded_count = 0;
    cbm_pipeline_get_excluded(pipeline, &excluded, &excluded_count);
    cbm_file_error_t *errors = NULL;
    int error_count = 0;
    cbm_pipeline_get_file_errors(pipeline, &errors, &error_count);
    cbm_mem_collect();
    bool metrics_failed = !index_write_metrics(metrics_out, mode_name, project, pipeline, rc);
    if (metrics_failed) {
        cbm_log_warn("index.metrics_write_failed", "path", metrics_out);
    }
    free(metrics_out);
    if (runtime->project_invalidate)
        runtime->project_invalidate(runtime->project_invalidate_context, project);
    char *payload =
        index_encode_run_response(project, repo_path, persistence, pipeline, rc, excluded,
                                  excluded_count, errors, error_count, metrics_failed);
    if (cbm_index_worker_active())
        cbm_log_info("index.worker.fast_exit", "skip", "pipeline_free");
    else
        cbm_pipeline_free(pipeline);
    runtime->mutation_end(runtime->mutation_context, mutation_project);
    free(project);
    free(mutation_project);
    return payload ? cbm_operation_result_take(payload, rc != 0)
                   : index_text_error("result encoding failed");
}

/* Resolve metrics_out, require its parent directory to exist inside the allowed
 * roots, and rewrite the worker argument to the canonical parent + basename so
 * the worker writes exactly the validated path. */
static bool index_validate_metrics_out(const char *json, const cbm_operation_runtime_t *runtime,
                                       const char *allowed_root, char **worker_args, char *err,
                                       size_t err_size) {
    char *metrics = index_string_arg(json, "metrics_out");
    if (!metrics || !metrics[0]) {
        free(metrics);
        return true;
    }
    cbm_normalize_path_sep(metrics);
    if (!index_resolve_session_path(runtime, &metrics)) {
        free(metrics);
        (void)snprintf(err, err_size, "failed to resolve metrics_out");
        return false;
    }
    char *slash = strrchr(metrics, '/');
    const char *base = metrics;
    if (slash) {
        base = slash;
        base++;
    }
    if (!base[0] || !strcmp(base, ".") || !strcmp(base, "..")) {
        free(metrics);
        (void)snprintf(err, err_size, "metrics_out must name a file");
        return false;
    }
    char parent[CBM_SZ_4K];
    size_t parent_len = slash ? (size_t)(slash - metrics) : 0;
    if (!slash) {
        (void)snprintf(parent, sizeof(parent), ".");
    } else if (parent_len == 0) {
        (void)snprintf(parent, sizeof(parent), "/");
    } else if (parent_len >= sizeof(parent)) {
        free(metrics);
        (void)snprintf(err, err_size, "metrics_out path too long");
        return false;
    } else {
        memcpy(parent, metrics, parent_len);
        parent[parent_len] = '\0';
    }
    char real[CBM_SZ_4K];
    if (!cbm_canonical_path(parent, real, sizeof(real))) {
        free(metrics);
        (void)snprintf(err, err_size, "metrics_out parent directory does not exist");
        return false;
    }
    cbm_normalize_path_sep(real);
    if (!cbm_workspace_root_allowed(real, cbm_workspace_home_dir(), cbm_workspace_cache_dir(),
                                    allowed_root, err, err_size)) {
        free(metrics);
        return false;
    }
    char final_path[CBM_SZ_4K];
    const char *last_sep = strrchr(real, '/');
    int n = snprintf(final_path, sizeof(final_path), "%s%s%s", real,
                     last_sep && !strcmp(last_sep, "/") ? "" : "/", base);
    free(metrics);
    if (n <= 0 || (size_t)n >= sizeof(final_path)) {
        (void)snprintf(err, err_size, "metrics_out path too long");
        return false;
    }
    char *rewritten = index_args_with_string(*worker_args, "metrics_out", final_path);
    if (!rewritten) {
        (void)snprintf(err, err_size, "failed to prepare index request");
        return false;
    }
    free(*worker_args);
    *worker_args = rewritten;
    return true;
}

cbm_operation_result_t cbm_index_operation_execute(const char *args_json,
                                                   const cbm_operation_runtime_t *runtime) {
    const char *json = args_json ? args_json : "{}";
    char *repo_path = index_string_arg(json, "repo_path");
    if (!repo_path)
        repo_path = index_repo_path_from_project(json);
    cbm_normalize_path_sep(repo_path);
    if (!repo_path)
        return index_text_error("repo_path is required");
    if (!index_resolve_session_path(runtime, &repo_path)) {
        free(repo_path);
        return index_text_error("failed to resolve repo_path");
    }
    repo_path = index_canonicalize_repo_path(repo_path);
    const char *allowed_root = runtime && runtime->allowed_root_policy_set
                                   ? runtime->allowed_root
                                   : getenv("CBM_ALLOWED_ROOT");
    char boundary[CBM_SZ_1K];
    if (!cbm_workspace_root_allowed(repo_path, cbm_workspace_home_dir(), cbm_workspace_cache_dir(),
                                    allowed_root, boundary, sizeof(boundary))) {
        free(repo_path);
        return index_text_error(boundary);
    }
    char *mode = index_string_arg(json, "mode");
    if (mode && !strcmp(mode, "cross-repo-intelligence")) {
        free(mode);
        cbm_operation_result_t out = cbm_cross_repo_operation_execute(repo_path, json, runtime);
        free(repo_path);
        return out;
    }
    free(mode);
    char *worker_args = index_args_with_repo_path(json, repo_path);
    if (!worker_args) {
        free(repo_path);
        return index_text_error("failed to prepare index request");
    }
    if (!index_validate_metrics_out(json, runtime, allowed_root, &worker_args, boundary,
                                    sizeof(boundary))) {
        free(worker_args);
        free(repo_path);
        return index_text_error(boundary);
    }
    if (runtime && runtime->index_execute) {
        cbm_operation_result_t out =
            runtime->index_execute(runtime->index_execute_context, repo_path, worker_args);
        free(worker_args);
        free(repo_path);
        return out;
    }
    cbm_operation_result_t out = index_run_physical(repo_path, worker_args, runtime);
    free(worker_args);
    free(repo_path);
    return out;
}
