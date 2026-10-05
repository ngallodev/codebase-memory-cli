#ifndef CBM_OPERATIONS_JSON_ARGS_H
#define CBM_OPERATIONS_JSON_ARGS_H

#include <stdbool.h>
#include "foundation/mem_core.h"

/* Returned strings belong to CBM_MEM_CLASS_OPERATION_ARG; release with
 * cbm_free(CBM_MEM_CLASS_OPERATION_ARG, value). */

char *cbm_json_string_arg(const char *args_json, const char *key);
void cbm_operation_arg_free(char *value);
int cbm_json_int_arg(const char *args_json, const char *key, int fallback);
bool cbm_json_bool_arg(const char *args_json, const char *key);

#endif
