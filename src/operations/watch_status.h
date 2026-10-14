#ifndef CBM_OPERATIONS_WATCH_STATUS_H
#define CBM_OPERATIONS_WATCH_STATUS_H

#include "operations/operation.h"
#include "yyjson/yyjson.h"

/* Adds the "watch" object: is this project kept fresh by the background
 * watcher right now, and if not, why. Auto-sync covers the active session's
 * project only; anything else needs a new index run. */
void cbm_op_add_watch_status_json(yyjson_mut_doc *doc, yyjson_mut_val *root,
                                  const cbm_operation_runtime_t *runtime, const char *project);

#endif
