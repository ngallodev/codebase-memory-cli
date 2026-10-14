#ifndef CBM_OPERATIONS_DOC_CANDIDATES_H
#define CBM_OPERATIONS_DOC_CANDIDATES_H

#include "operations/compact_out.h"
#include "store/store.h"
#include "yyjson/yyjson.h"

/* Doc -> code candidates: the functions, whole files and folders a documentation
 * Section is possibly about, and (reversed) the sections that possibly describe
 * a function or a file. Candidates with a judged probability for an agent to
 * verify, never links. */
extern const char CBM_DOC_CANDIDATES_NOTE[];

enum {
    CBM_DOC_CANDIDATES_MAX = 5,
    CBM_DOC_CANDIDATES_SECTION_MAX = 10,
};

/* Append the shown candidate rows to `arr` (objects), resolving the other end
 * of each row against the store and skipping rows whose other end no longer
 * exists (a delta update renamed or removed it). by_section resolves
 * rows[i].target_qn and shows qualified_name/label/end_line; otherwise it
 * resolves rows[i].section_qn and shows section. Returns how many resolved. */
int cbm_doc_candidates_array(yyjson_mut_doc *doc, yyjson_mut_val *arr, cbm_store_t *store,
                             const char *project, const cbm_doc_candidate_t *rows, int n,
                             bool by_section);

/* On a Section node: add possibly_about + candidates_note (the functions and
 * files it is possibly about). On a Function/Method: add
 * possibly_described_in + candidates_note (the sections possibly describing it).
 * Any other label, or no resolvable rows, adds nothing. */
void cbm_doc_candidates_add_node(yyjson_mut_doc *doc, yyjson_mut_val *root, cbm_store_t *store,
                                 const cbm_node_t *node);

/* The sections possibly about `path` as a whole or about a folder holding it,
 * as a JSON array in `doc`, highest p first; NULL when none resolve. */
yyjson_mut_val *cbm_doc_candidates_for_path(yyjson_mut_doc *doc, cbm_store_t *store,
                                            const char *project, const char *path);

/* The tree form of cbm_doc_candidates_for_path (no-op when none resolve). */
void cbm_doc_candidates_tree(cbm_sb_t *sb, cbm_store_t *store, const char *project,
                             const char *path);

#endif /* CBM_OPERATIONS_DOC_CANDIDATES_H */
