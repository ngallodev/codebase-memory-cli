#include "operations/result_wire.h"
#include "operations/read.h"
#include "operations/coverage.h"
#include "operations/snippet.h"
#include "operations/search.h"
#include "operations/trace.h"
#include "operations/trace_ingest.h"
#include "operations/schema.h"
#include "operations/query.h"
#include "operations/architecture.h"
#include "operations/changes.h"
#include "operations/source_search.h"
#include "operations/file_outline.h"
#include "operations/compare.h"
#include "operations/store_host.h"

#include "foundation/platform.h"
#include "foundation/compat_fs.h"
#include "git/git_context.h"
#include "store/store.h"
#include "yyjson/yyjson.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OP_DB_EXT_LEN 3U
#define OP_COVERAGE_FILE_CAP 500
#define OP_COVERAGE_SUMMARY_SAMPLES 5
#define OP_PROJECTS_MAX_LIMIT 500

static char *copy_string(const char *text);

static int json_int_arg(const char *args_json, const char *name, int fallback) {
    yyjson_doc *doc = args_json ? yyjson_read(args_json, strlen(args_json), 0) : NULL;
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    int result = value && yyjson_is_int(value) ? (int)yyjson_get_sint(value) : fallback;
    if (doc) {
        yyjson_doc_free(doc);
    }
    return result;
}

static bool json_bool_arg(const char *args_json, const char *name, bool fallback) {
    yyjson_doc *doc = args_json ? yyjson_read(args_json, strlen(args_json), 0) : NULL;
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    bool result = value && yyjson_is_bool(value) ? yyjson_get_bool(value) : fallback;
    if (doc) {
        yyjson_doc_free(doc);
    }
    return result;
}

static char *json_string_arg(const char *args_json, const char *name) {
    yyjson_doc *doc = args_json ? yyjson_read(args_json, strlen(args_json), 0) : NULL;
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    const char *text = value && yyjson_is_str(value) ? yyjson_get_str(value) : NULL;
    char *copy = copy_string(text);
    if (doc) {
        yyjson_doc_free(doc);
    }
    return copy;
}

static cbm_operation_result_t json_doc_result(yyjson_mut_doc *doc, bool is_error) {
    if (!doc) {
        return cbm_operation_result_copy("{\"error\":\"result allocation failed\"}", true);
    }
    char *json = cbm_operation_json_write(doc);
    yyjson_mut_doc_free(doc);
    if (!json) {
        return cbm_operation_result_copy("{\"error\":\"result encoding failed\"}", true);
    }
    return cbm_operation_result_take(json, is_error);
}

static cbm_operation_result_t json_error(const char *message, const char *hint) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root) {
        if (doc) {
            yyjson_mut_doc_free(doc);
        }
        return cbm_operation_result_copy(message ? message : "operation failed", true);
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "error", message ? message : "operation failed");
    if (hint) {
        yyjson_mut_obj_add_strcpy(doc, root, "hint", hint);
    }
    return json_doc_result(doc, true);
}

static bool project_db_file(const char *name) {
    if (!name) {
        return false;
    }
    size_t len = strlen(name);
    if (len <= OP_DB_EXT_LEN || strcmp(name + len - OP_DB_EXT_LEN, ".db") != 0) {
        return false;
    }
    return name[0] != '_' && strncmp(name, ":memory:", strlen(":memory:")) != 0;
}

static char *copy_string(const char *text) {
    if (!text) {
        return NULL;
    }
    size_t len = strlen(text);
    char *copy = malloc(len + 1U);
    if (copy) {
        memcpy(copy, text, len + 1U);
    }
    return copy;
}

typedef struct {
    char *name;    /* internal project name (the sort key) */
    char *db_file; /* cache-relative database file name */
} project_ref_t;

static void project_ref_clear(project_ref_t *ref) {
    free(ref->name);
    free(ref->db_file);
    ref->name = NULL;
    ref->db_file = NULL;
}

/* Order by project name, then database file, so pages are stable and a list is
 * identical however the filesystem enumerates the cache. */
static int project_ref_cmp(const void *left, const void *right) {
    const project_ref_t *a = left;
    const project_ref_t *b = right;
    int by_name = strcmp(a->name, b->name);
    return by_name != 0 ? by_name : strcmp(a->db_file, b->db_file);
}

static bool primary_project_name(cbm_store_t *store, char *out, size_t out_size) {
    cbm_project_t *projects = NULL;
    int count = 0;
    if (!store || !out || out_size == 0 ||
        cbm_store_list_projects(store, &projects, &count) != CBM_STORE_OK) {
        return false;
    }
    int primary = -1;
    int primary_count = 0;
    for (int i = 0; i < count; ++i) {
        if (projects[i].name && projects[i].name[0] && !strstr(projects[i].name, "::")) {
            primary = i;
            ++primary_count;
        }
    }
    bool ok = primary_count == 1;
    if (ok) {
        (void)snprintf(out, out_size, "%s", projects[primary].name);
    }
    cbm_store_free_projects(projects, count);
    return ok;
}

static void add_project_entry(yyjson_mut_doc *doc, yyjson_mut_val *array, const char *cache_dir,
                              const char *db_name, bool include_details, bool metadata_only) {
    char db_path[4096];
    if (snprintf(db_path, sizeof(db_path), "%s/%s", cache_dir, db_name) >= (int)sizeof(db_path)) {
        return;
    }
    cbm_store_t *store = cbm_store_open_path_query(db_path);
    if (!store) {
        return;
    }
    char project_name[1024];
    if (!primary_project_name(store, project_name, sizeof(project_name))) {
        cbm_store_close(store);
        return;
    }

    cbm_project_t project = {0};
    char root_path[4096] = "";
    if (cbm_store_get_project(store, project_name, &project) == CBM_STORE_OK) {
        if (project.root_path) {
            (void)snprintf(root_path, sizeof(root_path), "%s", project.root_path);
        }
        cbm_project_free_fields(&project);
    }

    yyjson_mut_val *item = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_strcpy(doc, item, "name", project_name);
    yyjson_mut_obj_add_strcpy(doc, item, "root_path", root_path);
    /* The listing stays lean: only the branch (the one git fact that
     * disambiguates same-repo projects). The full git block is one status call
     * away for the project you actually care about. */
    if (!metadata_only) {
        cbm_git_context_t git = {0};
        if (root_path[0] && cbm_git_context_resolve(root_path, &git) && git.is_git && git.branch) {
            yyjson_mut_obj_add_strcpy(doc, item, "branch", git.branch);
        }
        cbm_git_context_free(&git);
    }
    if (include_details) {
        yyjson_mut_obj_add_int(doc, item, "nodes", cbm_store_count_nodes(store, project_name));
        yyjson_mut_obj_add_int(doc, item, "edges", cbm_store_count_edges(store, project_name));
        int64_t size = cbm_file_size(db_path);
        yyjson_mut_obj_add_int(doc, item, "size_bytes", size < 0 ? 0 : size);
    }
    yyjson_mut_arr_add_val(array, item);
    cbm_store_close(store);
}

static cbm_operation_result_t execute_projects(const char *args_json) {
    int offset = json_int_arg(args_json, "offset", 0);
    int limit = json_int_arg(args_json, "limit", 50);
    /* `detail:"stats"` and `include_details` are two spellings of the same
     * request for the counts/size projection. */
    char *detail = json_string_arg(args_json, "detail");
    bool include_details = json_bool_arg(args_json, "include_details", false) ||
                           (detail && strcmp(detail, "stats") == 0);
    free(detail);
    bool metadata_only = json_bool_arg(args_json, "metadata_only", false);
    if (metadata_only) {
        include_details = false;
    }
    if (offset < 0) {
        offset = 0;
    }
    if (limit < 1) {
        limit = 1;
    } else if (limit > OP_PROJECTS_MAX_LIMIT) {
        limit = OP_PROJECTS_MAX_LIMIT;
    }

    const char *cache_dir = cbm_resolve_cache_dir();
    errno = 0;
    cbm_dir_t *dir = cache_dir ? cbm_opendir(cache_dir) : NULL;
    if (!dir && errno != ENOENT) {
        return json_error(
            "cannot read cache directory",
            "Check directory permissions or run 'codebase-memory-cli index .' first.");
    }

    project_ref_t *names = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool oom = false;
    cbm_dirent_t *entry = NULL;
    while (dir && (entry = cbm_readdir(dir)) != NULL) {
        if (!project_db_file(entry->name)) {
            continue;
        }
        char db_path[4096];
        if (snprintf(db_path, sizeof(db_path), "%s/%s", cache_dir, entry->name) >=
            (int)sizeof(db_path)) {
            continue;
        }
        cbm_store_t *candidate = cbm_store_open_path_query(db_path);
        char project_name[1024];
        bool valid =
            candidate && primary_project_name(candidate, project_name, sizeof(project_name));
        if (candidate) {
            cbm_store_close(candidate);
        }
        if (!valid) {
            continue;
        }
        if (count == capacity) {
            size_t next = capacity ? capacity * 2U : 32U;
            project_ref_t *grown = realloc(names, next * sizeof(*names));
            if (!grown) {
                oom = true;
                break;
            }
            names = grown;
            capacity = next;
        }
        names[count].name = copy_string(project_name);
        names[count].db_file = copy_string(entry->name);
        if (!names[count].name || !names[count].db_file) {
            project_ref_clear(&names[count]);
            oom = true;
            break;
        }
        ++count;
    }
    if (dir) {
        cbm_closedir(dir);
    }
    if (oom) {
        for (size_t i = 0; i < count; ++i) {
            project_ref_clear(&names[i]);
        }
        free(names);
        return json_error("out of memory while collecting indexed projects", NULL);
    }
    if (count > 1) {
        qsort(names, count, sizeof(*names), project_ref_cmp);
    }

    size_t start = (size_t)offset < count ? (size_t)offset : count;
    size_t end = start + (size_t)limit;
    if (end > count) {
        end = count;
    }
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    yyjson_mut_val *projects = doc ? yyjson_mut_arr(doc) : NULL;
    if (!doc || !root || !projects) {
        for (size_t i = 0; i < count; ++i) {
            project_ref_clear(&names[i]);
        }
        free(names);
        if (doc) {
            yyjson_mut_doc_free(doc);
        }
        return json_error("result allocation failed", NULL);
    }
    yyjson_mut_doc_set_root(doc, root);
    for (size_t i = start; i < end; ++i) {
        add_project_entry(doc, projects, cache_dir, names[i].db_file, include_details,
                          metadata_only);
    }
    for (size_t i = 0; i < count; ++i) {
        project_ref_clear(&names[i]);
    }
    free(names);

    yyjson_mut_obj_add_val(doc, root, "projects", projects);
    yyjson_mut_obj_add_uint(doc, root, "total", count);
    yyjson_mut_obj_add_int(doc, root, "offset", offset);
    yyjson_mut_obj_add_int(doc, root, "limit", limit);
    yyjson_mut_obj_add_uint(doc, root, "returned", yyjson_mut_arr_size(projects));
    yyjson_mut_obj_add_bool(doc, root, "has_more", end < count);
    if (end < count) {
        yyjson_mut_obj_add_uint(doc, root, "next_offset", end);
    }
    if (yyjson_mut_arr_size(projects) == 0) {
        yyjson_mut_obj_add_str(doc, root, "hint",
                               "No projects indexed. Run 'codebase-memory-cli index .' first.");
    }
    return json_doc_result(doc, false);
}

static void add_status_coverage(yyjson_mut_doc *doc, yyjson_mut_val *root, cbm_store_t *store,
                                const char *project, const char *indexed_at, int sample_limit) {
    cbm_coverage_row_t *rows = NULL;
    int count = 0;
    (void)cbm_store_coverage_get(store, project, &rows, &count);
    cbm_coverage_meta_t meta = {0};
    bool have_meta = cbm_store_coverage_meta_get(store, project, &meta) == CBM_STORE_OK;

    yyjson_mut_val *partial_files = yyjson_mut_arr(doc);
    yyjson_mut_val *unusable_files = yyjson_mut_arr(doc);
    yyjson_mut_val *skipped_files = yyjson_mut_arr(doc);
    yyjson_mut_val *excluded_dirs = yyjson_mut_arr(doc);
    yyjson_mut_val *excluded_files = yyjson_mut_arr(doc);
    int partial_count = 0;
    int unusable_count = 0;
    int skipped_count = 0;
    int excluded_dir_count = 0;
    int excluded_file_count = 0;
    for (int i = 0; i < count; ++i) {
        const char *kind = rows[i].kind ? rows[i].kind : "";
        if (strcmp(kind, "parse_partial") == 0) {
            if (partial_count < sample_limit) {
                yyjson_mut_val *entry = yyjson_mut_obj(doc);
                yyjson_mut_obj_add_strcpy(doc, entry, "path",
                                          rows[i].rel_path ? rows[i].rel_path : "");
                yyjson_mut_obj_add_strcpy(doc, entry, "error_ranges",
                                          rows[i].detail ? rows[i].detail : "");
                yyjson_mut_arr_add_val(partial_files, entry);
            }
            ++partial_count;
        } else if (strcmp(kind, "parse_unusable") == 0) {
            /* Needs its own branch: the catch-all below builds skipped[], and a
             * reader who finds a file there believes it was never indexed. */
            if (unusable_count < OP_COVERAGE_FILE_CAP) {
                yyjson_mut_val *entry = yyjson_mut_obj(doc);
                yyjson_mut_obj_add_strcpy(doc, entry, "path",
                                          rows[i].rel_path ? rows[i].rel_path : "");
                yyjson_mut_obj_add_bool(doc, entry, "whole_file", true);
                /* The end of the range, not the length of the file: a grammar
                 * can end an error node past the last line. */
                const char *dash = rows[i].detail ? strchr(rows[i].detail, '-') : NULL;
                yyjson_mut_obj_add_int(doc, entry, "range_end", dash ? atoi(dash + 1) : 0);
                yyjson_mut_arr_add_val(unusable_files, entry);
            }
            ++unusable_count;
        } else if (strcmp(kind, "not_indexed_dir") == 0) {
            if (excluded_dir_count < sample_limit) {
                yyjson_mut_arr_add_strcpy(doc, excluded_dirs,
                                          rows[i].rel_path ? rows[i].rel_path : "");
            }
            ++excluded_dir_count;
        } else if (strcmp(kind, "not_indexed_file") == 0) {
            if (excluded_file_count < sample_limit) {
                yyjson_mut_val *entry = yyjson_mut_obj(doc);
                yyjson_mut_obj_add_strcpy(doc, entry, "path",
                                          rows[i].rel_path ? rows[i].rel_path : "");
                yyjson_mut_obj_add_strcpy(doc, entry, "reason",
                                          rows[i].detail ? rows[i].detail : "");
                yyjson_mut_arr_add_val(excluded_files, entry);
            }
            ++excluded_file_count;
        } else {
            if (skipped_count < sample_limit) {
                yyjson_mut_val *entry = yyjson_mut_obj(doc);
                yyjson_mut_obj_add_strcpy(doc, entry, "path",
                                          rows[i].rel_path ? rows[i].rel_path : "");
                yyjson_mut_obj_add_strcpy(doc, entry, "reason",
                                          rows[i].detail ? rows[i].detail : "");
                yyjson_mut_obj_add_strcpy(doc, entry, "phase", kind);
                yyjson_mut_arr_add_val(skipped_files, entry);
            }
            ++skipped_count;
        }
    }
    cbm_store_free_coverage(rows, count);

    /* Discovery retains a bounded sample of ignored files but records the exact
     * uncapped total atomically beside it. Counts-only status must use that
     * authoritative total, or a lean response looks complete exactly when the
     * discovery cap was hit. */
    bool generation_matches =
        have_meta && indexed_at && meta.generation && strcmp(indexed_at, meta.generation) == 0;
    bool ignored_total_authoritative =
        generation_matches && meta.ignored_files_total >= excluded_file_count;
    int excluded_file_total =
        ignored_total_authoritative ? meta.ignored_files_total : excluded_file_count;
    if (have_meta)
        cbm_store_coverage_meta_clear(&meta);
    int excluded_files_shown =
        excluded_file_count < sample_limit ? excluded_file_count : sample_limit;

    yyjson_mut_val *partial = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_val(doc, partial, "files", partial_files);
    yyjson_mut_obj_add_int(doc, partial, "count", partial_count);
    yyjson_mut_obj_add_bool(doc, partial, "truncated", partial_count > sample_limit);
    if (partial_count > sample_limit)
        yyjson_mut_obj_add_int(doc, partial, "omitted", partial_count - sample_limit);
    yyjson_mut_obj_add_val(doc, root, "parse_partial", partial);

    /* Indexed, but the parse failed across nearly the whole file, so naming line
     * ranges helps nobody: read the source instead. */
    yyjson_mut_val *unusable = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_val(doc, unusable, "files", unusable_files);
    yyjson_mut_obj_add_int(doc, unusable, "count", unusable_count);
    yyjson_mut_obj_add_bool(doc, unusable, "truncated", unusable_count > OP_COVERAGE_FILE_CAP);
    yyjson_mut_obj_add_val(doc, root, "parse_unusable", unusable);

    yyjson_mut_val *skipped = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_val(doc, skipped, "files", skipped_files);
    yyjson_mut_obj_add_int(doc, skipped, "count", skipped_count);
    yyjson_mut_obj_add_bool(doc, skipped, "truncated", skipped_count > sample_limit);
    if (skipped_count > sample_limit)
        yyjson_mut_obj_add_int(doc, skipped, "omitted", skipped_count - sample_limit);
    yyjson_mut_obj_add_val(doc, root, "skipped", skipped);

    yyjson_mut_val *excluded = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_val(doc, excluded, "dirs", excluded_dirs);
    yyjson_mut_obj_add_int(doc, excluded, "dirs_count", excluded_dir_count);
    yyjson_mut_obj_add_val(doc, excluded, "files", excluded_files);
    yyjson_mut_obj_add_int(doc, excluded, "files_count", excluded_file_total);
    if (!ignored_total_authoritative)
        yyjson_mut_obj_add_str(doc, excluded, "files_count_relation", "gte");
    yyjson_mut_obj_add_bool(doc, excluded, "truncated",
                            excluded_dir_count > sample_limit ||
                                excluded_file_total > excluded_files_shown);
    if (excluded_dir_count > sample_limit)
        yyjson_mut_obj_add_int(doc, excluded, "dirs_omitted", excluded_dir_count - sample_limit);
    if (excluded_file_total > excluded_files_shown)
        yyjson_mut_obj_add_int(doc, excluded, "files_omitted",
                               excluded_file_total - excluded_files_shown);
    if (sample_limit > 0 && (excluded_dir_count > 0 || excluded_file_total > 0)) {
        yyjson_mut_obj_add_str(doc, excluded, "note",
                               "Purposely not indexed — excluded by ignore rules. Change the "
                               "ignore rules and re-index to include them.");
    }
    yyjson_mut_obj_add_val(doc, root, "not_indexed", excluded);

    if (sample_limit > 0 && (partial_count > 0 || skipped_count > 0)) {
        yyjson_mut_obj_add_str(doc, root, "coverage_note",
                               "Best-effort signal, not a completeness guarantee. Read flagged "
                               "source directly when graph coverage is partial or skipped.");
    }
}

static cbm_operation_result_t execute_status(const char *args_json) {
    char *project = json_string_arg(args_json, "project");
    if (!project || !project[0]) {
        free(project);
        yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
        yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
        if (!doc || !root) {
            if (doc)
                yyjson_mut_doc_free(doc);
            return json_error("result allocation failed", NULL);
        }
        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "status", "no_project");
        return json_doc_result(doc, false);
    }

    cbm_store_open_status_t open_status = CBM_STORE_OPEN_OK;
    cbm_store_t *store = cbm_store_host_open_query(project, &open_status);
    if (!store) {
        cbm_operation_result_t error =
            open_status == CBM_STORE_OPEN_CORRUPT
                ? json_error(CBM_STORE_CORRUPT_MESSAGE, CBM_STORE_CORRUPT_HINT)
                : json_error("project not indexed",
                             "Run 'codebase-memory-cli index .' in the repository or specify an "
                             "indexed --project.");
        free(project);
        return error;
    }
    int nodes = cbm_store_count_nodes(store, project);
    int edges = cbm_store_count_edges(store, project);
    bool verbose = json_bool_arg(args_json, "verbose", false);
    /* Counts-only by default; summary samples a few paths per class and full
     * lists up to the per-class cap. */
    char *diagnostics = json_string_arg(args_json, "diagnostics");
    int coverage_samples = 0;
    if (diagnostics && strcmp(diagnostics, "summary") == 0)
        coverage_samples = OP_COVERAGE_SUMMARY_SAMPLES;
    else if (diagnostics && strcmp(diagnostics, "full") == 0)
        coverage_samples = OP_COVERAGE_FILE_CAP;
    free(diagnostics);

    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root) {
        if (doc)
            yyjson_mut_doc_free(doc);
        cbm_store_close(store);
        free(project);
        return json_error("result allocation failed", NULL);
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "project", project);
    bool counts_unreadable = nodes < 0 || edges < 0;
    yyjson_mut_obj_add_int(doc, root, "nodes", counts_unreadable ? 0 : nodes);
    yyjson_mut_obj_add_int(doc, root, "edges", counts_unreadable ? 0 : edges);
    if (counts_unreadable) {
        const char *tables;
        if (nodes < 0 && edges < 0)
            tables = "nodes and edges";
        else if (nodes < 0)
            tables = "nodes";
        else
            tables = "edges";
        char hint[CBM_SZ_512];
        snprintf(hint, sizeof(hint),
                 "The %s table(s) could not be read; the database may be corrupt. "
                 "Re-run codebase-memory-cli index or remove the project cache and re-index.",
                 tables);
        yyjson_mut_obj_add_strcpy(doc, root, "hint", hint);
    }
    const char *status = "empty";
    if (nodes < 0 || edges < 0) {
        status = "error";
    } else if (nodes > 0) {
        status = "ready";
    }
    yyjson_mut_obj_add_str(doc, root, "status", status);

    cbm_project_t info = {0};
    bool have_info = cbm_store_get_project(store, project, &info) == CBM_STORE_OK;
    if (have_info) {
        const char *root_path = info.root_path ? info.root_path : "";
        yyjson_mut_obj_add_strcpy(doc, root, "root_path", root_path);
        yyjson_mut_obj_add_strcpy(doc, root, "indexed_at", info.indexed_at ? info.indexed_at : "");
        if (verbose && root_path[0]) {
            cbm_git_context_t git = {0};
            if (cbm_git_context_resolve(root_path, &git) && git.is_git) {
                yyjson_mut_val *git_obj = yyjson_mut_obj(doc);
                if (git.branch)
                    yyjson_mut_obj_add_strcpy(doc, git_obj, "branch", git.branch);
                if (git.head_sha)
                    yyjson_mut_obj_add_strcpy(doc, git_obj, "head_sha", git.head_sha);
                yyjson_mut_obj_add_val(doc, root, "git", git_obj);
            }
            cbm_git_context_free(&git);
        }
    }
    add_status_coverage(doc, root, store, project, have_info ? info.indexed_at : NULL,
                        coverage_samples);
    if (have_info)
        cbm_project_free_fields(&info);
    if (nodes == 0) {
        yyjson_mut_obj_add_str(
            doc, root, "hint",
            "Project is empty. Re-run 'codebase-memory-cli index .' to populate.");
    }
    cbm_store_close(store);
    free(project);
    return json_doc_result(doc, false);
}

bool cbm_read_operation_supported(cbm_operation_id_t operation) {
    return operation == CBM_OPERATION_PROJECTS || operation == CBM_OPERATION_STATUS ||
           operation == CBM_OPERATION_COVERAGE || operation == CBM_OPERATION_SEARCH ||
           operation == CBM_OPERATION_SNIPPET || operation == CBM_OPERATION_TRACE ||
           operation == CBM_OPERATION_SCHEMA || operation == CBM_OPERATION_QUERY ||
           operation == CBM_OPERATION_ARCHITECTURE || operation == CBM_OPERATION_CHANGES ||
           operation == CBM_OPERATION_SOURCE_SEARCH || operation == CBM_OPERATION_FILE_OUTLINE ||
           operation == CBM_OPERATION_COMPARE || operation == CBM_OPERATION_INGEST_TRACES;
}

cbm_operation_result_t cbm_read_operation_execute(cbm_operation_id_t operation,
                                                  const char *args_json,
                                                  const cbm_operation_runtime_t *runtime) {
    (void)runtime;
    if (operation == CBM_OPERATION_PROJECTS) {
        return execute_projects(args_json);
    }
    if (operation == CBM_OPERATION_STATUS) {
        return execute_status(args_json);
    }
    if (operation == CBM_OPERATION_COVERAGE) {
        return cbm_coverage_operation_execute(args_json);
    }
    if (operation == CBM_OPERATION_SEARCH) {
        return cbm_search_operation_execute(args_json);
    }
    if (operation == CBM_OPERATION_SNIPPET) {
        return cbm_snippet_operation_execute(args_json);
    }
    if (operation == CBM_OPERATION_TRACE) {
        return cbm_trace_operation_execute(args_json);
    }
    if (operation == CBM_OPERATION_SCHEMA) {
        return cbm_schema_operation_execute(args_json);
    }
    if (operation == CBM_OPERATION_QUERY) {
        return cbm_query_operation_execute(args_json);
    }
    if (operation == CBM_OPERATION_ARCHITECTURE) {
        return cbm_architecture_operation_execute(args_json);
    }
    if (operation == CBM_OPERATION_CHANGES) {
        return cbm_changes_operation_execute(args_json, runtime);
    }
    if (operation == CBM_OPERATION_SOURCE_SEARCH) {
        return cbm_source_search_operation_execute(args_json, runtime);
    }
    if (operation == CBM_OPERATION_FILE_OUTLINE) {
        return cbm_file_outline_operation_execute(args_json, runtime);
    }
    if (operation == CBM_OPERATION_COMPARE) {
        return cbm_compare_operation_execute(args_json, runtime);
    }
    if (operation == CBM_OPERATION_INGEST_TRACES) {
        return cbm_trace_ingest_operation_execute(args_json);
    }
    return cbm_operation_result_copy("native read operation not implemented", true);
}
