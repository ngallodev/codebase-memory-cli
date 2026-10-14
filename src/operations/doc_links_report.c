#include "operations/doc_links_report.h"

#include "foundation/constants.h"
#include "foundation/log.h"
#include "pipeline/doc_links.h" /* cbm_doclink_reason_name: the reasons the layer writes */
#include "store/store.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Doc-comment references (status.doc_links): MENTIONS edges, the
 * unresolved rows per reason, and the layer's status. "error" when the
 * generation recorded a failed doc-link layer, when the doc_link_unresolved
 * table is missing (an index from before the layer: reindex), or when it
 * cannot be read. Samples (diagnostics=full) are rows ordered by reason,
 * path and line. */
#ifdef CBM_ENABLE_TEST_SEAMS
static atomic_int op_doc_links_sample_alloc_countdown = ATOMIC_VAR_INIT(CBM_NOT_FOUND);
static atomic_bool op_doc_links_sample_alloc_failed = ATOMIC_VAR_INIT(false);

void cbm_op_doc_links_test_fail_sample_alloc_after(int successful_copies) {
    atomic_store_explicit(&op_doc_links_sample_alloc_failed, false, memory_order_relaxed);
    atomic_store_explicit(&op_doc_links_sample_alloc_countdown,
                          successful_copies < 0 ? CBM_NOT_FOUND : successful_copies,
                          memory_order_relaxed);
}

bool cbm_op_doc_links_test_sample_alloc_failed(void) {
    return atomic_load_explicit(&op_doc_links_sample_alloc_failed, memory_order_relaxed);
}
#endif

/* The seam rejects an actual sample string allocation in yyjson's pool,
 * independently of when that pool needs another backing allocation. */
static yyjson_mut_val *doc_links_sample_string(yyjson_mut_doc *doc, const char *source,
                                               size_t length) {
    if (!doc || !source) {
        return NULL;
    }
#ifdef CBM_ENABLE_TEST_SEAMS
    int remaining =
        atomic_load_explicit(&op_doc_links_sample_alloc_countdown, memory_order_relaxed);
    while (remaining >= 0) {
        int next = remaining == 0 ? CBM_NOT_FOUND : remaining - 1;
        if (atomic_compare_exchange_weak_explicit(&op_doc_links_sample_alloc_countdown, &remaining,
                                                  next, memory_order_relaxed,
                                                  memory_order_relaxed)) {
            if (remaining == 0) {
                atomic_store_explicit(&op_doc_links_sample_alloc_failed, true,
                                      memory_order_relaxed);
                return NULL;
            }
            break;
        }
    }
#endif
    return yyjson_mut_strncpy(doc, source, length);
}

typedef struct {
    char text[CBM_DOC_LINK_PREVIEW_LONG_BYTES + 1];
    size_t length;
    size_t included;
    bool truncated;
    bool escaped;
} doc_link_preview_t;

/* A strict scalar decoder. Invalid input is represented one byte at a time
 * by the caller, including incomplete sequences at the end of the source. */
static size_t doc_link_scalar(const unsigned char *text, size_t length, uint32_t *scalar) {
    if (!length) {
        return 0;
    }
    unsigned char first = text[0];
    if (first < 0x80) {
        *scalar = first;
        return 1;
    }
    size_t width;
    uint32_t value;
    if (first >= 0xC2 && first <= 0xDF) {
        width = 2;
        value = first & 0x1F;
    } else if (first >= 0xE0 && first <= 0xEF) {
        width = 3;
        value = first & 0x0F;
    } else if (first >= 0xF0 && first <= 0xF4) {
        width = 4;
        value = first & 0x07;
    } else {
        return 0;
    }
    if (length < width || (first == 0xE0 && text[1] < 0xA0) || (first == 0xED && text[1] > 0x9F) ||
        (first == 0xF0 && text[1] < 0x90) || (first == 0xF4 && text[1] > 0x8F)) {
        return 0;
    }
    for (size_t i = 1; i < width; i++) {
        if ((text[i] & 0xC0) != 0x80) {
            return 0;
        }
        value = (value << 6) | (text[i] & 0x3F);
    }
    *scalar = value;
    return width;
}

/* `unsigned long` (at least 32 bits by the standard), not uint32_t: the
 * vendored yyjson.h has a fallback typedef chain for uint32_t, and an
 * analysis that cannot evaluate it takes uint32_t for a 16-bit type and the
 * tag block's bounds for out of range. */
static bool doc_link_visible_escape(unsigned long scalar) {
    return scalar < 0x20 || scalar == 0x7F || (scalar >= 0x200B && scalar <= 0x200F) ||
           (scalar >= 0x202A && scalar <= 0x202E) || (scalar >= 0x2066 && scalar <= 0x2069) ||
           (scalar >= 0xE0000 && scalar <= 0xE007F);
}

/* Output tokens are complete scalars or visible escapes. Lookahead bytes do
 * not count as included source, and the shared transport encoder is not used. */
static bool doc_link_format_preview(const cbm_doc_link_preview_text_t *source, size_t budget,
                                    doc_link_preview_t *out) {
    memset(out, 0, sizeof(*out));
    if (!source->text || budget >= sizeof(out->text) ||
        source->length > budget + CBM_DOC_LINK_PREVIEW_LOOKAHEAD ||
        source->original_bytes < source->length) {
        return false;
    }
    const unsigned char *text = (const unsigned char *)source->text;
    bool reserved = (source->length >= 7 && memcmp(text, "@bytes:", 7) == 0) ||
                    (source->length >= 6 && memcmp(text, "@utf8:", 6) == 0);
    while (out->included < source->length && out->length < budget) {
        const unsigned char *at = text + out->included;
        uint32_t scalar = 0;
        size_t consumed = doc_link_scalar(at, source->length - out->included, &scalar);
        char escape[16];
        const char *token = (const char *)at;
        size_t bytes = consumed;
        bool escaped = false;
        if (!consumed) {
            int written = snprintf(escape, sizeof(escape), "\\x%02X", (unsigned int)at[0]);
            if (written < 0 || (size_t)written >= sizeof(escape)) {
                return false;
            }
            consumed = 1;
            bytes = (size_t)written;
            token = escape;
            escaped = true;
        } else if (scalar == '\\') {
            token = "\\\\";
            bytes = 2;
            escaped = true;
        } else if (doc_link_visible_escape(scalar) || (out->included == 0 && reserved)) {
            int written = snprintf(escape, sizeof(escape), "\\u{%04X}", (unsigned int)scalar);
            if (written < 0 || (size_t)written >= sizeof(escape)) {
                return false;
            }
            bytes = (size_t)written;
            token = escape;
            escaped = true;
        }
        if (bytes > budget - out->length) {
            break;
        }
        memcpy(out->text + out->length, token, bytes);
        out->length += bytes;
        out->included += consumed;
        out->escaped = out->escaped || escaped;
    }
    out->text[out->length] = '\0';
    out->truncated = out->included < source->original_bytes;
    return true;
}

/* Neither array is attached to the report until every row and metadata entry
 * has been constructed. The JSON document owns all intermediate allocations. */
static bool doc_link_preview_samples(yyjson_mut_doc *doc, const cbm_doc_link_preview_row_t *rows,
                                     int count, yyjson_mut_val **samples_out,
                                     yyjson_mut_val **metadata_out) {
    yyjson_mut_val *samples = yyjson_mut_arr(doc);
    yyjson_mut_val *metadata = yyjson_mut_arr(doc);
    if (!samples || !metadata) {
        return false;
    }
    static const char *const names[] = {"rel_path", "syntax", "raw", "reason"};
    int emitted = 0;
    for (int i = 0; i < count; i++) {
        if (!rows[i].rel_path.original_bytes) {
            continue;
        }
        const cbm_doc_link_preview_text_t *fields[] = {&rows[i].rel_path, &rows[i].syntax,
                                                       &rows[i].raw, &rows[i].reason};
        yyjson_mut_val *sample = yyjson_mut_obj(doc);
        if (!sample) {
            return false;
        }
        for (int field = 0; field < 4; field++) {
            size_t budget = field == 0 || field == 2 ? CBM_DOC_LINK_PREVIEW_LONG_BYTES
                                                     : CBM_DOC_LINK_PREVIEW_SHORT_BYTES;
            doc_link_preview_t preview;
            if (!doc_link_format_preview(fields[field], budget, &preview)) {
                return false;
            }
            yyjson_mut_val *value = doc_links_sample_string(doc, preview.text, preview.length);
            if (!value || !yyjson_mut_obj_add_val(doc, sample, names[field], value) ||
                (field == 0 && !yyjson_mut_obj_add_int(doc, sample, "line", rows[i].line))) {
                return false;
            }
            if (preview.truncated || preview.escaped) {
                yyjson_mut_val *entry = yyjson_mut_obj(doc);
                if (!entry || !yyjson_mut_obj_add_int(doc, entry, "sample_index", emitted) ||
                    !yyjson_mut_obj_add_str(doc, entry, "field", names[field]) ||
                    !yyjson_mut_obj_add_uint(doc, entry, "original_bytes",
                                             fields[field]->original_bytes) ||
                    !yyjson_mut_obj_add_uint(doc, entry, "included_source_bytes",
                                             preview.included) ||
                    !yyjson_mut_obj_add_bool(doc, entry, "truncated", preview.truncated) ||
                    !yyjson_mut_obj_add_bool(doc, entry, "escaped", preview.escaped) ||
                    !yyjson_mut_arr_append(metadata, entry)) {
                    return false;
                }
            }
        }
        if (!yyjson_mut_arr_append(samples, sample)) {
            return false;
        }
        emitted++;
    }
    *samples_out = samples;
    *metadata_out = yyjson_mut_arr_size(metadata) ? metadata : NULL;
    return true;
}

/* True for a reason the doc-link layer writes. The table is read back from
 * the database: any other text in it was not written by this layer, and
 * never becomes a key of the report. */
static bool doc_link_reason_known(const char *reason) {
    for (int r = 0; r < CBM_DOCLINK_REASON_COUNT; r++) {
        if (strcmp(reason, cbm_doclink_reason_name(r)) == 0) {
            return true;
        }
    }
    return false;
}

bool cbm_op_add_doc_links_report(yyjson_mut_doc *doc, yyjson_mut_val *root, cbm_store_t *store,
                                 const char *project, bool with_samples) {
    int mentions = cbm_store_count_edges_by_type(store, project, "MENTIONS");
    cbm_doc_link_reason_count_t *reasons = NULL;
    int nreasons = 0;
    cbm_doc_link_row_t *unused_samples = NULL;
    int unused_count = 0;
    bool present = false;
    int rc = cbm_store_doc_links_summary(store, project, &reasons, &nreasons, &unused_samples,
                                         &unused_count, 0, &present);
    cbm_doc_link_preview_row_t *rows = NULL;
    int count = 0;
    yyjson_mut_val *samples = NULL;
    yyjson_mut_val *metadata = NULL;
    bool sample_failed = with_samples && rc != CBM_STORE_OK;
    if (with_samples && rc == CBM_STORE_OK) {
        bool preview_present = false;
        int preview_rc =
            cbm_store_doc_links_preview(store, project, &rows, &count, &preview_present);
        sample_failed = preview_rc != CBM_STORE_OK || preview_present != present;
        if (!sample_failed) {
            sample_failed = !doc_link_preview_samples(doc, rows, count, &samples, &metadata);
        }
    }
    bool error = rc != CBM_STORE_OK || mentions < 0 || !present || sample_failed;
    bool built = false;
    int64_t unrecognized = 0;
    yyjson_mut_val *dl = yyjson_mut_obj(doc);
    yyjson_mut_val *unresolved = yyjson_mut_obj(doc);
    if (!dl || !unresolved) {
        goto cleanup;
    }
    for (int i = 0; i < nreasons; i++) {
        if (strcmp(reasons[i].reason, "error") == 0) {
            error = true;
            continue;
        }
        if (!doc_link_reason_known(reasons[i].reason)) {
            /* rows this layer did not write: counted under one fixed key */
            unrecognized += reasons[i].count;
            error = true;
            continue;
        }
        yyjson_mut_val *key = yyjson_mut_strcpy(doc, reasons[i].reason);
        yyjson_mut_val *number = yyjson_mut_int(doc, reasons[i].count);
        if (!key || !number || !yyjson_mut_obj_add(unresolved, key, number)) {
            goto cleanup;
        }
    }
    if (unrecognized > 0 &&
        !yyjson_mut_obj_add_int(doc, unresolved, "unrecognized_reason", unrecognized)) {
        goto cleanup;
    }
    if (!yyjson_mut_obj_add_int(doc, dl, "mentions", mentions < 0 ? 0 : mentions) ||
        !yyjson_mut_obj_add_val(doc, dl, "unresolved", unresolved) ||
        !yyjson_mut_obj_add_str(doc, dl, "status", error ? "error" : "ok")) {
        goto cleanup;
    }
    if (error) {
        const char *hint =
            sample_failed && rc == CBM_STORE_OK
                ? "Doc-link samples could not be prepared; retry status."
                : (!present && rc == CBM_STORE_OK
                       ? "This index predates doc-comment links; re-run "
                         "'codebase-memory-cli index .'."
                       : "The doc-link layer failed or its table could not be read; re-run "
                         "'codebase-memory-cli index .'.");
        if (!yyjson_mut_obj_add_str(doc, dl, "hint", hint)) {
            goto cleanup;
        }
    }
    if (with_samples && !sample_failed) {
        if (!yyjson_mut_obj_add_val(doc, dl, "samples", samples)) {
            goto cleanup;
        }
        if (metadata &&
            (!yyjson_mut_obj_add_val(doc, dl, "samples_preview", metadata) ||
             !yyjson_mut_obj_add_str(doc, dl, "samples_preview_note",
                                     "Sample text is shown as previews; complete values remain "
                                     "in doc_link_unresolved."))) {
            goto cleanup;
        }
    }
    /* A failed append leaves this whole report unattached. The caller discards
     * the document and answers with its allocation-error result. */
    built = yyjson_mut_obj_add_val(doc, root, "doc_links", dl);
cleanup:
    if (error && (rc != CBM_STORE_OK || sample_failed)) {
        cbm_log_warn("status.doc_links", "project", project, "reason", "read_failed");
    }
    cbm_store_free_doc_link_reasons(reasons, nreasons);
    cbm_store_free_doc_links(unused_samples, unused_count);
    cbm_store_free_doc_link_previews(rows, count);
    return built;
}
