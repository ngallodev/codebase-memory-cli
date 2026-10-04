#ifndef CBM_OPERATIONS_INDEX_H
#define CBM_OPERATIONS_INDEX_H

#include "foundation/index_policy.h"
#include "operations/operation.h"

#include <stdbool.h>
#include <yyjson/yyjson.h>

/* #2144: the advice every cut-short synchronous index surfaces. A long index
 * can outlive a client's per-call deadline; --async lets the daemon finish it
 * while the client polls with --status. */
#define CBM_INDEX_ASYNC_HINT                                                              \
    "Long indexes can exceed a client's per-call deadline: retry with the index command " \
    "and --async, then poll with the same command and --status until state is "           \
    "succeeded, failed or cancelled."

/* Encode the complete trusted discovery policy on an internal worker request.
 * Callers must remove any untrusted field with the same name before invoking
 * this. */
bool cbm_index_operation_policy_add_to_args(yyjson_mut_doc *doc, yyjson_mut_val *root,
                                            const cbm_index_resource_policy_t *policy);

cbm_operation_result_t cbm_index_operation_execute(const char *args_json,
                                                   const cbm_operation_runtime_t *runtime);

#endif
