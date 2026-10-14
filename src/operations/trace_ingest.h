#ifndef CBM_OPERATIONS_TRACE_INGEST_H
#define CBM_OPERATIONS_TRACE_INGEST_H
#include "operations/operation.h"
cbm_operation_result_t cbm_trace_ingest_operation_execute(const char *args_json,
                                                          const cbm_operation_runtime_t *runtime);
#endif
