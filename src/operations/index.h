#ifndef CBM_OPERATIONS_INDEX_H
#define CBM_OPERATIONS_INDEX_H

#include "foundation/index_policy.h"
#include "operations/index_supervisor.h"
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

/* Decode the trusted policy a supervisor encoded on an internal worker
 * request. False (with error filled) when it is missing or incomplete. */
bool cbm_index_policy_from_internal_args(const char *args_json, cbm_index_resource_policy_t *policy,
                                         char *error, size_t error_size);

/* Request-scoped facts a worker resource failure is reported against. One
 * supervised request can run several worker attempts (crash/hang recovery), and
 * serving_index_preserved describes the request, not only its last attempt:
 * it is true only when a servable index served the project when the request
 * began AND no attempt of the request committed to replacing it (the
 * publication fence, index_supervisor.h) AND that index is still servable. */
typedef struct {
    bool serving_index_existed;
    bool publication_committed;
} cbm_index_request_facts_t;

/* Capture the facts at the start of a request, before its first worker. */
void cbm_index_request_facts_begin(const char *args_json, cbm_index_request_facts_t *facts);
/* Fold one finished attempt into the request facts. */
void cbm_index_request_facts_record(cbm_index_request_facts_t *facts,
                                    const cbm_index_worker_result_t *worker_result);
/* Native result wire (is_error set, code resource_limit_exceeded or
 * resource_probe_failed) for a worker stopped by a resource limit; caller
 * frees. NULL facts claim nothing: serving_index_preserved is then false. */
char *cbm_index_worker_resource_response(const char *args_json,
                                         const cbm_index_worker_result_t *worker_result,
                                         const cbm_index_request_facts_t *facts);

/* The fork's key-value config store (defined in cli/cli.h). Declared here so
 * the encoder below can name it without pulling cli.h into every includer. */
struct cbm_config;

/* Encode a doc_candidates setting the user set to a boolean on an internal
 * worker request. The worker reads no config, so the parent resolves the
 * setting once and passes the decided value on; unset or unparseable adds no
 * key, and the worker's index mode decides. False only when encoding fails. */
bool cbm_index_doc_candidates_add_to_args(yyjson_mut_doc *doc, yyjson_mut_val *root,
                                          struct cbm_config *config);

cbm_operation_result_t cbm_index_operation_execute(const char *args_json,
                                                   const cbm_operation_runtime_t *runtime);

#endif
