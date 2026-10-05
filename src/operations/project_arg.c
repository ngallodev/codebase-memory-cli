#include "operations/json_args.h"
#include "foundation/mem_core.h"
#include "operations/project_arg.h"

#include "foundation/compat_fs.h"
#include "foundation/constants.h"
#include "foundation/platform.h"
#include "foundation/workspace.h"
#include "store/store.h"
#include "foundation/str_util.h"
#include "pipeline/pipeline.h"
#include "yyjson/yyjson.h"

#include <stdlib.h>
#include <string.h>

static char *project_arg_strdup(const char *text) {
    return cbm_mem_strdup(CBM_MEM_CLASS_OPERATION_ARG, text);
}

static bool project_arg_is_db_file(const char *name, size_t len) {
    (void)len;
    return cbm_is_project_index_db(name);
}

static char *project_arg_path_owner(const char *path, char *derived) {
    const char *cache = cbm_resolve_cache_dir();
    char db_path[CBM_SZ_4K];
    if (!cache ||
        snprintf(db_path, sizeof(db_path), "%s/%s.db", cache, derived) >= (int)sizeof(db_path) ||
        cbm_file_exists(db_path))
        return derived;
    cbm_dir_t *dir = cbm_opendir(cache);
    if (!dir)
        return derived;
    char *owner = NULL;
    size_t depth = 0;
    bool ambiguous = false;
    cbm_dirent_t *entry;
    while ((entry = cbm_readdir(dir)) != NULL) {
        if (!cbm_is_project_index_db(entry->name) ||
            snprintf(db_path, sizeof(db_path), "%s/%s", cache, entry->name) >= (int)sizeof(db_path))
            continue;
        cbm_store_t *store = cbm_store_open_path_query(db_path);
        cbm_project_t *projects = NULL;
        int count = 0;
        if (store && cbm_store_list_projects(store, &projects, &count) == CBM_STORE_OK &&
            count == 1) {
            char root[CBM_SZ_4K];
            if (projects[0].root_path &&
                cbm_canonical_path(projects[0].root_path, root, sizeof(root))) {
                cbm_normalize_path_sep(root);
                if (cbm_path_within_root(root, path)) {
                    size_t n = strlen(root);
                    if (n > depth) {
                        cbm_operation_arg_free(owner);
                        owner = project_arg_strdup(projects[0].name);
                        depth = n;
                        ambiguous = false;
                    } else if (n == depth) {
                        ambiguous = true;
                    }
                }
            }
        }
        cbm_store_free_projects(projects, count);
        cbm_store_close(store);
    }
    cbm_closedir(dir);
    if (!owner || ambiguous) {
        cbm_operation_arg_free(owner);
        return derived;
    }
    cbm_operation_arg_free(derived);
    return owner;
}

static char *project_arg_normalize(char *project) {
    if (!project)
        return project;
    if (!strchr(project, '/') && !strchr(project, '\\')) {
        bool non_ascii = false;
        for (const unsigned char *p = (const unsigned char *)project; *p; p++)
            non_ascii |= *p >= 0x80;
        char *raw_encoded = non_ascii ? cbm_project_name_sanitize(project) : NULL;
        char *encoded = raw_encoded ? project_arg_strdup(raw_encoded) : NULL;
        free(raw_encoded);
        bool usable = encoded && strcmp(encoded, "root") != 0 && cbm_validate_project_name(encoded);
        cbm_operation_arg_free(usable ? project : encoded);
        return usable ? encoded : project;
    }

    char real[CBM_SZ_4K];
    if (cbm_canonical_path(project, real, sizeof(real))) {
        cbm_normalize_path_sep(real);
        char *canonical = project_arg_strdup(real);
        if (canonical) {
            cbm_operation_arg_free(project);
            project = canonical;
        }
    }
    char *raw_normalized = cbm_project_name_from_path(project);
    char *normalized = raw_normalized ? project_arg_strdup(raw_normalized) : NULL;
    free(raw_normalized);
    if (normalized) {
        normalized = project_arg_path_owner(project, normalized);
        cbm_operation_arg_free(project);
        return normalized;
    }
    return project;
}

static char *project_arg_resolve_tail(char *project) {
    if (!project || !cbm_validate_project_name(project))
        return project;
    const char *cache_dir = cbm_resolve_cache_dir();
    if (!cache_dir || !cache_dir[0])
        return project;

    char exact[CBM_SZ_2K];
    if (snprintf(exact, sizeof(exact), "%s/%s.db", cache_dir, project) >= (int)sizeof(exact)) {
        return project;
    }
    if (cbm_file_exists(exact))
        return project;

    size_t plen = strlen(project);
    char match[CBM_SZ_1K] = "";
    int matches = 0;
    cbm_dir_t *dir = cbm_opendir(cache_dir);
    if (!dir)
        return project;
    cbm_dirent_t *entry;
    while ((entry = cbm_readdir(dir)) != NULL) {
        const char *name = entry->name;
        size_t len = strlen(name);
        if (!project_arg_is_db_file(name, len))
            continue;
        size_t stem_len = len - 3U;
        if (stem_len <= plen + 1U || stem_len >= sizeof(match))
            continue;
        if (name[stem_len - plen - 1U] != '-' ||
            strncmp(name + stem_len - plen, project, plen) != 0) {
            continue;
        }
        matches++;
        if (matches > 1)
            break;
        memcpy(match, name, stem_len);
        match[stem_len] = '\0';
    }
    cbm_closedir(dir);
    if (matches == 1) {
        cbm_operation_arg_free(project);
        return project_arg_strdup(match);
    }
    return project;
}

char *cbm_operation_project_arg(const char *args_json) {
    static const char *const names[] = {"project", "project_name", "project_id", "projectName"};
    const char *json = args_json ? args_json : "{}";
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    char *project = NULL;
    if (yyjson_is_obj(root)) {
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
            yyjson_val *value = yyjson_obj_get(root, names[i]);
            if (yyjson_is_str(value)) {
                project = project_arg_strdup(yyjson_get_str(value));
                break;
            }
        }
    }
    if (doc)
        yyjson_doc_free(doc);
    return project_arg_resolve_tail(project_arg_normalize(project));
}
