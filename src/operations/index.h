#ifndef CBM_OPERATIONS_INDEX_H
#define CBM_OPERATIONS_INDEX_H

#include "foundation/index_policy.h"
#include "operations/operation.h"

#include <stdbool.h>
#include <yyjson/yyjson.h>

/* Encode the complete trusted discovery policy on an internal worker request.
 * Callers must remove any untrusted field with the same name before invoking
 * this. */
bool cbm_index_operation_policy_add_to_args(yyjson_mut_doc *doc, yyjson_mut_val *root,
                                            const cbm_index_resource_policy_t *policy);

cbm_operation_result_t cbm_index_operation_execute(const char *args_json,
                                                   const cbm_operation_runtime_t *runtime);

#endif
