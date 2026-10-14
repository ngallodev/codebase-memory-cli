#include "operations/doc_candidates.h"

#include "foundation/constants.h"
#include "operations/compact_out.h"
#include "store/store.h"
#include "yyjson/yyjson.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Doc -> code candidates (doc_link_candidates): on a Section, the functions,
 * whole files and the home folder it is possibly about; on a function or
 * method, the sections that possibly describe it; on a file (get_file_outline),
 * the sections that possibly describe the file or a folder holding it --
 * highest p first. A row whose other end no longer resolves (a delta update
 * renamed or removed it) is skipped. */
const char CBM_DOC_CANDIDATES_NOTE[] =
    "doc<->code candidates by shared terms or the doc's folder, not links: p is the judged "
    "probability the section is about the target; verify";

/* The evidence fields of a candidate row as shown: kind, position, and (on
 * the section side) the shared terms and name tokens. */
static void doc_candidate_evidence(yyjson_mut_doc *doc, yyjson_mut_val *o, const char *evidence,
                                   bool counts) {
    yyjson_doc *ev = evidence ? yyjson_read(evidence, strlen(evidence), 0) : NULL;
    yyjson_val *root = ev ? yyjson_doc_get_root(ev) : NULL;
    static const char *const STRS[] = {"kind", "position", NULL};
    for (int k = 0; root && STRS[k]; k++) {
        yyjson_val *v = yyjson_obj_get(root, STRS[k]);
        if (v && yyjson_is_str(v)) {
            yyjson_mut_obj_add_strcpy(doc, o, STRS[k], yyjson_get_str(v));
        }
    }
    static const char *const INTS[] = {"shared_terms", "name_tokens", NULL};
    for (int k = 0; counts && root && INTS[k]; k++) {
        yyjson_val *v = yyjson_obj_get(root, INTS[k]);
        if (v && yyjson_is_int(v)) {
            yyjson_mut_obj_add_int(doc, o, INTS[k], yyjson_get_int(v));
        }
    }
    yyjson_doc_free(ev);
}

/* p is shown rounded to three decimals: the judged probability is a coarse
 * ranking signal, not a measurement. */
static const double CBM_DOC_P_SCALE = 1000.0;
static const double CBM_DOC_P_HALF = 0.5;

int cbm_doc_candidates_array(yyjson_mut_doc *doc, yyjson_mut_val *arr, cbm_store_t *store,
                             const char *project, const cbm_doc_candidate_t *rows, int n,
                             bool by_section) {
    int added = 0;
    for (int i = 0; i < n; i++) {
        cbm_node_t other = {0};
        if (cbm_store_find_node_by_qn(store, project,
                                      by_section ? rows[i].target_qn : rows[i].section_qn,
                                      &other) != CBM_STORE_OK) {
            continue;
        }
        yyjson_mut_val *o = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_strcpy(doc, o, by_section ? "qualified_name" : "section",
                                  other.qualified_name ? other.qualified_name : "");
        if (by_section) {
            yyjson_mut_obj_add_strcpy(doc, o, "label", other.label ? other.label : "");
        }
        yyjson_mut_obj_add_strcpy(doc, o, "file_path", other.file_path ? other.file_path : "");
        yyjson_mut_obj_add_int(doc, o, "start_line", other.start_line);
        if (by_section) {
            yyjson_mut_obj_add_int(doc, o, "end_line", other.end_line);
        }
        double rounded_p =
            (double)(int)((rows[i].p * CBM_DOC_P_SCALE) + CBM_DOC_P_HALF) / CBM_DOC_P_SCALE;
        yyjson_mut_obj_add_real(doc, o, "p", rounded_p);
        doc_candidate_evidence(doc, o, rows[i].evidence, by_section);
        yyjson_mut_arr_append(arr, o);
        added++;
        cbm_node_free_fields(&other);
    }
    return added;
}

void cbm_doc_candidates_add_node(yyjson_mut_doc *doc, yyjson_mut_val *root, cbm_store_t *store,
                                 const cbm_node_t *node) {
    if (!store || !node->label || !node->qualified_name || !node->project) {
        return;
    }
    bool section = strcmp(node->label, "Section") == 0;
    if (!section && strcmp(node->label, "Function") != 0 && strcmp(node->label, "Method") != 0) {
        return;
    }
    cbm_doc_candidate_t *rows = NULL;
    int n = 0;
    if (cbm_store_doc_candidates_get(store, node->project, section ? node->qualified_name : NULL,
                                     section ? NULL : node->qualified_name,
                                     section ? CBM_DOC_CANDIDATES_SECTION_MAX
                                             : CBM_DOC_CANDIDATES_MAX,
                                     &rows, &n) != CBM_STORE_OK) {
        return;
    }
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    int added = cbm_doc_candidates_array(doc, arr, store, node->project, rows, n, section);
    cbm_store_doc_candidates_free(rows, n);
    if (added > 0) {
        yyjson_mut_obj_add_val(doc, root, section ? "possibly_about" : "possibly_described_in",
                               arr);
        yyjson_mut_obj_add_str(doc, root, "candidates_note", CBM_DOC_CANDIDATES_NOTE);
    }
}

yyjson_mut_val *cbm_doc_candidates_for_path(yyjson_mut_doc *doc, cbm_store_t *store,
                                            const char *project, const char *path) {
    cbm_doc_candidate_t *rows = NULL;
    int n = 0;
    if (cbm_store_doc_candidates_for_path(store, project, path, CBM_DOC_CANDIDATES_MAX, &rows,
                                          &n) != CBM_STORE_OK) {
        return NULL;
    }
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    int added = cbm_doc_candidates_array(doc, arr, store, project, rows, n, false);
    cbm_store_doc_candidates_free(rows, n);
    return added > 0 ? arr : NULL;
}

static const char *mut_str(yyjson_mut_val *o, const char *key) {
    yyjson_mut_val *v = yyjson_mut_obj_get(o, key);
    return v && yyjson_mut_is_str(v) ? yyjson_mut_get_str(v) : "";
}

void cbm_doc_candidates_tree(cbm_sb_t *sb, cbm_store_t *store, const char *project,
                             const char *path) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *arr = doc ? cbm_doc_candidates_for_path(doc, store, project, path) : NULL;
    if (arr) {
        static const char *const columns[] = {"section", "file_path", "line", "p", "kind"};
        cbm_tree_table_header(sb, "possibly_described_in", (int)yyjson_mut_arr_size(arr), columns,
                              (int)(sizeof(columns) / sizeof(columns[0])));
        size_t i = 0;
        size_t max = 0;
        yyjson_mut_val *o = NULL;
        yyjson_mut_arr_foreach(arr, i, max, o) {
            char line[CBM_SZ_32];
            char p[CBM_SZ_32];
            (void)snprintf(line, sizeof(line), "%d",
                           yyjson_mut_get_int(yyjson_mut_obj_get(o, "start_line")));
            (void)snprintf(p, sizeof(p), "%.2f", yyjson_mut_get_real(yyjson_mut_obj_get(o, "p")));
            cbm_tree_row_begin(sb);
            cbm_tree_cell_str(sb, mut_str(o, "section"), true);
            cbm_tree_cell_str(sb, mut_str(o, "file_path"), false);
            cbm_tree_cell_str(sb, line, false);
            cbm_tree_cell_str(sb, p, false);
            cbm_tree_cell_str(sb, mut_str(o, "kind"), false);
            cbm_tree_row_end(sb);
        }
        cbm_tree_scalar_str(sb, "candidates_note", CBM_DOC_CANDIDATES_NOTE);
    }
    yyjson_mut_doc_free(doc);
}
