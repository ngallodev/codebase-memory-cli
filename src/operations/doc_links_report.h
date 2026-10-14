#ifndef CBM_OPERATIONS_DOC_LINKS_REPORT_H
#define CBM_OPERATIONS_DOC_LINKS_REPORT_H

#include "store/store.h"
#include "yyjson/yyjson.h"

#include <stdbool.h>

/* Adds the "doc_links" object to a status report: MENTIONS edge count,
 * unresolved rows per reason, and the doc-link layer's status ("ok" or
 * "error" with a hint). with_samples adds up to CBM_DOC_LINK_PREVIEW_ROWS
 * bounded, escaped previews of the unresolved rows. Returns false when the
 * report could not be built; the caller must then discard the document. */
bool cbm_op_add_doc_links_report(yyjson_mut_doc *doc, yyjson_mut_val *root, cbm_store_t *store,
                                 const char *project, bool with_samples);

#ifdef CBM_ENABLE_TEST_SEAMS
/* One-shot sample string-copy allocation failure: 0 fails the next copy, 4 the
 * fifth, -1 disables. Setting clears the consumed flag. */
void cbm_op_doc_links_test_fail_sample_alloc_after(int successful_copies);
bool cbm_op_doc_links_test_sample_alloc_failed(void);
#endif

#endif
