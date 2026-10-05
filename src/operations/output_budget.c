#include "operations/output_budget.h"
#include "operations/json_args.h"

#include "yyjson/yyjson.h"

#include <stdbool.h>
#include <limits.h>
#include <string.h>

int cbm_output_budget_tokens(const char *args_json, int fallback) {
    const char *json = args_json ? args_json : "{}";
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, "max_output_tokens") : NULL;
    bool present = (yyjson_is_sint(value) && yyjson_get_sint(value) >= INT_MIN &&
                    yyjson_get_sint(value) <= INT_MAX) ||
                   (yyjson_is_uint(value) && yyjson_get_uint(value) <= (uint64_t)INT_MAX);
    int tokens = cbm_json_int_arg(args_json, "max_output_tokens", fallback);
    if (doc) {
        yyjson_doc_free(doc);
    }
    if (!present) {
        /* Absent/unusable argument: keep the caller's default so omitting it
         * never engages a budget. */
        return tokens;
    }
    if (tokens < CBM_OUTPUT_TOKENS_MIN) {
        tokens = CBM_OUTPUT_TOKENS_MIN;
    } else if (tokens > CBM_OUTPUT_TOKENS_MAX) {
        tokens = CBM_OUTPUT_TOKENS_MAX;
    }
    return tokens;
}

size_t cbm_output_budget_bytes(int max_output_tokens) {
    if (max_output_tokens <= 0) {
        return 0;
    }
    if (max_output_tokens < CBM_OUTPUT_TOKENS_MIN) {
        max_output_tokens = CBM_OUTPUT_TOKENS_MIN;
    } else if (max_output_tokens > CBM_OUTPUT_TOKENS_MAX) {
        max_output_tokens = CBM_OUTPUT_TOKENS_MAX;
    }
    return (size_t)max_output_tokens * (size_t)CBM_OUTPUT_BYTES_PER_TOKEN;
}
