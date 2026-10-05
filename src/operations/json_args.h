#ifndef CBM_OPERATIONS_JSON_ARGS_H
#define CBM_OPERATIONS_JSON_ARGS_H

#include <stdbool.h>

void cbm_operation_arg_free(char *arg);

char *cbm_json_string_arg(const char *args_json, const char *key);
int cbm_json_int_arg(const char *args_json, const char *key, int fallback);
bool cbm_json_bool_arg(const char *args_json, const char *key);

#endif
