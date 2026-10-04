#ifndef CBM_OPERATIONS_OUTPUT_BUDGET_H
#define CBM_OPERATIONS_OUTPUT_BUDGET_H

#include <stddef.h>

/* Deterministic serialized-output ceiling shared by the budgeted operations.
 *
 * `max_output_tokens` is model-neutral sizing guidance, not a tokenizer
 * promise: the actual cross-platform contract is a UTF-8 byte ceiling applied
 * only at whole semantic-unit boundaries (a row, a source line, a scalar).
 * No identifier is ever byte-sliced; when even the smallest meaningful unit
 * does not fit, the operation answers with a budget-floor record telling the
 * caller which continuation to use. */
#define CBM_OUTPUT_BYTES_PER_TOKEN 4
#define CBM_OUTPUT_TOKENS_MIN 128
#define CBM_OUTPUT_TOKENS_MAX 1000000

/* Parse `max_output_tokens` from an operation args object. Returns `fallback`
 * when the key is absent or is not an integer; otherwise the requested value
 * clamped to [CBM_OUTPUT_TOKENS_MIN, CBM_OUTPUT_TOKENS_MAX]. A fallback <= 0
 * means "no budget requested". */
int cbm_output_budget_tokens(const char *args_json, int fallback);

/* Byte ceiling for a token count. 0 means unbounded. */
size_t cbm_output_budget_bytes(int max_output_tokens);

#endif
