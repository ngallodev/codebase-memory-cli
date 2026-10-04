#include "operations/project_arg.h"

#include "foundation/compat_fs.h"
#include "foundation/constants.h"
#include "foundation/mem_core.h"
#include "foundation/platform.h"
#include "foundation/str_util.h"
#include "pipeline/pipeline.h"
#include "yyjson/yyjson.h"

#include <stdlib.h>
#include <string.h>

static char *project_arg_strdup(const char *text) {
    if (!text)
        return NULL;
    return cbm_mem_strdup(CBM_MEM_CLASS_OPERATION_ARG, text);
}

static bool project_arg_is_db_file(const char *name, size_t len) {
    return name && len >= 4U && strcmp(name + len - 3U, ".db") == 0 && name[0] != '_' &&
           strncmp(name, ":memory:", 8U) != 0;
}

static char *project_arg_normalize(char *project) {
    if (!project)
        return project;
    if (!strchr(project, '/') && !strchr(project, '\\')) {
        bool non_ascii = false;
        for (const unsigned char *p = (const unsigned char *)project; *p; p++)
            non_ascii |= *p >= 0x80;
        char *encoded = non_ascii ? cbm_project_name_sanitize(project) : NULL;
        bool usable = encoded && strcmp(encoded, "root") != 0 && cbm_validate_project_name(encoded);
        if (usable) {
            char *copy = project_arg_strdup(encoded);
            cbm_free(CBM_MEM_CLASS_OPERATION_ARG, project);
            free(encoded);
            return copy;
        }
        free(encoded);
        return project;
    }

    char real[CBM_SZ_4K];
    if (cbm_canonical_path(project, real, sizeof(real))) {
        cbm_normalize_path_sep(real);
        char *canonical = project_arg_strdup(real);
        if (canonical) {
            cbm_free(CBM_MEM_CLASS_OPERATION_ARG, project);
            project = canonical;
        }
    }
    char *normalized = cbm_project_name_from_path(project);
    if (normalized) {
        char *copy = project_arg_strdup(normalized);
        free(normalized);
        cbm_free(CBM_MEM_CLASS_OPERATION_ARG, project);
        return copy;
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
        cbm_free(CBM_MEM_CLASS_OPERATION_ARG, project);
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
