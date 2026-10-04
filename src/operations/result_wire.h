#ifndef CBM_OPERATIONS_RESULT_WIRE_H
#define CBM_OPERATIONS_RESULT_WIRE_H

#include <stdbool.h>
#include "operations/operation.h"
#include "yyjson/yyjson.h"

/* Internal worker/coordinator transport for an operation result. This is not a
 * user-facing protocol and deliberately carries only the neutral payload plus
 * its error bit. */
char *cbm_operation_result_wire_encode(const cbm_operation_result_t *result);
bool cbm_operation_result_wire_decode(const char *wire, cbm_operation_result_t *result_out);

/* Serialize an operation payload document. DB-derived strings may hold
 * non-UTF-8 bytes (paths and identifiers from older runs or foreign
 * filesystems). Standard JSON cannot carry them, so each invalid string is
 * replaced by `@bytes:<hex>` and each valid string that begins with a reserved
 * `@bytes:` / `@utf8:` prefix becomes `@utf8:<original>`, keeping identities
 * lossless instead of failing serialization or emitting invalid JSON. The
 * document's strings are rewritten in place. Returns a heap string (caller
 * frees) or NULL. */
char *cbm_operation_json_write(yyjson_mut_doc *doc);

#endif
