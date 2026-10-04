#include "operations/result_wire.h"

#include "operations/compact_out.h"
#include "yyjson/yyjson.h"

#include <stdlib.h>
#include <string.h>

static bool normalize_output_text(yyjson_mut_doc *doc, yyjson_mut_val *value) {
    if (yyjson_mut_is_str(value)) {
        char *encoded = NULL;
        size_t encoded_len = 0;
        if (!cbm_output_encode_text(yyjson_mut_get_str(value), yyjson_mut_get_len(value), &encoded,
                                    &encoded_len))
            return false;
        if (!encoded)
            return true;
        yyjson_mut_val *owned = yyjson_mut_strncpy(doc, encoded, encoded_len);
        free(encoded);
        return owned && yyjson_mut_set_strn(value, yyjson_mut_get_str(owned), encoded_len);
    }
    size_t index, maximum;
    if (yyjson_mut_is_arr(value)) {
        yyjson_mut_val *item;
        yyjson_mut_arr_foreach(value, index, maximum, item) {
            if (!normalize_output_text(doc, item))
                return false;
        }
    } else if (yyjson_mut_is_obj(value)) {
        yyjson_mut_val *key, *item;
        yyjson_mut_obj_foreach(value, index, maximum, key, item) {
            if (!normalize_output_text(doc, key) || !normalize_output_text(doc, item))
                return false;
        }
    }
    return true;
}

char *cbm_operation_json_write(yyjson_mut_doc *doc) {
    yyjson_mut_val *root = doc ? yyjson_mut_doc_get_root(doc) : NULL;
    if (!root || !normalize_output_text(doc, root))
        return NULL;
    return yyjson_mut_write(doc, 0, NULL);
}

char *cbm_operation_result_wire_encode(const cbm_operation_result_t *result) {
    if (!result || !result->payload)
        return NULL;
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root) {
        if (doc)
            yyjson_mut_doc_free(doc);
        return NULL;
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "payload", result->payload);
    yyjson_mut_obj_add_bool(doc, root, "is_error", result->is_error);
    char *wire = yyjson_mut_write(doc, 0, NULL);
    yyjson_mut_doc_free(doc);
    return wire;
}

bool cbm_operation_result_wire_decode(const char *wire, cbm_operation_result_t *result_out) {
    if (!wire || !result_out)
        return false;
    yyjson_doc *doc = yyjson_read(wire, strlen(wire), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *payload = yyjson_is_obj(root) ? yyjson_obj_get(root, "payload") : NULL;
    yyjson_val *is_error = yyjson_is_obj(root) ? yyjson_obj_get(root, "is_error") : NULL;
    bool valid = yyjson_is_str(payload) && yyjson_is_bool(is_error);
    if (valid) {
        *result_out = cbm_operation_result_copy(yyjson_get_str(payload), yyjson_get_bool(is_error));
        valid = result_out->payload != NULL;
    }
    if (doc)
        yyjson_doc_free(doc);
    return valid;
}
