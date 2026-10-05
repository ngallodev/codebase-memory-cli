#include "operations/json_args.h"
#include "foundation/mem_core.h"
#include "yyjson/yyjson.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>

void cbm_operation_arg_free(char *value) {
    cbm_free(CBM_MEM_CLASS_OPERATION_ARG, value);
}

char *cbm_json_string_arg(const char *args_json, const char *key) {
    if (!key)
        return NULL;
    const char *json = args_json ? args_json : "{}";
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, key) : NULL;
    char *out = yyjson_is_str(value)
                    ? cbm_mem_strdup(CBM_MEM_CLASS_OPERATION_ARG, yyjson_get_str(value))
                    : NULL;
    if (doc)
        yyjson_doc_free(doc);
    return out;
}

int cbm_json_int_arg(const char *args_json, const char *key, int fallback) {
    if (!key)
        return fallback;
    const char *json = args_json ? args_json : "{}";
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, key) : NULL;
    int out = fallback;
    if (yyjson_is_sint(value)) {
        int64_t parsed = yyjson_get_sint(value);
        if (parsed >= INT_MIN && parsed <= INT_MAX)
            out = (int)parsed;
    } else if (yyjson_is_uint(value)) {
        uint64_t parsed = yyjson_get_uint(value);
        if (parsed <= (uint64_t)INT_MAX)
            out = (int)parsed;
    }
    if (doc)
        yyjson_doc_free(doc);
    return out;
}

bool cbm_json_bool_arg(const char *args_json, const char *key) {
    if (!key)
        return false;
    const char *json = args_json ? args_json : "{}";
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, key) : NULL;
    bool out = yyjson_is_bool(value) && yyjson_get_bool(value);
    if (doc)
        yyjson_doc_free(doc);
    return out;
}
