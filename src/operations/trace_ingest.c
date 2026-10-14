#include "operations/trace_ingest.h"
#include "operations/project_arg.h"
#include "foundation/json_writer.h"
#include "foundation/mem_core.h"
#include "store/store.h"
#include "yyjson/yyjson.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

enum {
    INGEST_TRACES_MAX = 10000,    /* trace entries per call */
    INGEST_PATH_MAX = 256,        /* hops per path */
    INGEST_PAIR_CAP = 200000,     /* observed pairs per project */
    INGEST_UNMATCHED_SAMPLE = 10, /* distinct unresolved names reported */
    INGEST_EVICT_MAX_ROUNDS = 32, /* runaway guard for the eviction loop */
};

static cbm_operation_result_t ingest_error(const char *message) {
    return cbm_operation_result_copy(message, true);
}

/* resolve_store opens file-backed projects query-only (and skips schema DDL);
 * observation writes need the dedicated read-write connection, whose open also
 * creates the observed_* tables in pre-existing databases. */
static cbm_store_t *ingest_open_store_for_write(const cbm_operation_runtime_t *runtime,
                                                cbm_store_t *resolved, cbm_store_t **owned_rw) {
    if (!resolved || !owned_rw)
        return NULL;
    const char *path = cbm_store_db_path(resolved);
    if (!path)
        return resolved;
    char *copy = cbm_mem_strdup(CBM_MEM_CLASS_OPERATION_ARG, path);
    if (!copy)
        return NULL;
    if (runtime && runtime->store_invalidate)
        runtime->store_invalidate(runtime->store_context);
    *owned_rw = cbm_store_open_path(copy);
    cbm_free(CBM_MEM_CLASS_OPERATION_ARG, copy);
    return *owned_rw;
}

/* Observed runtime calls: paths of qualified names (adjacent elements are
 * caller->callee) or bare {caller, callee} pairs, stored qn-keyed in
 * observed_calls/observed_paths (reindex-proof), per run label. The response
 * is honest about resolution: only pairs whose BOTH endpoints exist as indexed
 * symbols are stored, and up to 10 unresolved names come back so the producer
 * can fix its naming instead of silently losing data. */
cbm_operation_result_t cbm_trace_ingest_operation_execute(const char *args_json,
                                                          const cbm_operation_runtime_t *runtime) {
    const char *args = args_json ? args_json : "{}";
    char *project = cbm_operation_project_arg(args);
    bool mutation_held = false;
    yyjson_doc *adoc = NULL;
    cbm_store_t *owned_rw = NULL;
    cbm_operation_result_t result = {0};

    if (!project || !project[0]) {
        cbm_operation_arg_free(project);
        return ingest_error("missing required argument: project");
    }
    if (!runtime || !runtime->store_resolve || !runtime->mutation_begin || !runtime->mutation_end) {
        cbm_operation_arg_free(project);
        return ingest_error("project mutation coordination is unavailable for this execution path");
    }
    mutation_held = runtime->mutation_begin(runtime->mutation_context, project);
    if (!mutation_held) {
        result = ingest_error("project operation cancelled or blocked by an active index");
        goto release_request;
    }
    if (runtime->cancelled && runtime->cancelled(runtime->cancelled_context)) {
        result = ingest_error("project operation cancelled for this request");
        goto release_request;
    }

    /* Hold the lease before resolving a cached generation or opening the
     * writer, and through both observation updates and retention eviction. */
    cbm_operation_store_recovery_status_t recovery = CBM_OPERATION_STORE_RECOVERY_NONE;
    cbm_store_t *store =
        runtime->store_resolve(runtime->store_context, project, true, false, &recovery);
    if (!store) {
        char *error_json =
            runtime->store_error ? runtime->store_error(runtime->store_context, project) : NULL;
        result = error_json ? cbm_operation_result_take(error_json, true)
                            : ingest_error("project not found or not indexed");
        goto release_request;
    }

    adoc = yyjson_read(args, strlen(args), 0);
    yyjson_val *aroot = adoc ? yyjson_doc_get_root(adoc) : NULL;
    yyjson_val *traces = yyjson_is_obj(aroot) ? yyjson_obj_get(aroot, "traces") : NULL;
    size_t trace_count = traces && yyjson_is_arr(traces) ? yyjson_arr_size(traces) : 0U;
    if (trace_count == 0U || trace_count > INGEST_TRACES_MAX) {
        result = ingest_error(
            "traces is required: a non-empty array (max 10000) of {path: [qualified_name, ...]} "
            "or {caller, callee} entries");
        goto release_request;
    }
    const char *label = yyjson_get_str(yyjson_obj_get(aroot, "label"));
    if (!label || !label[0])
        label = "unlabeled";

    store = ingest_open_store_for_write(runtime, store, &owned_rw);
    if (!store) {
        result = ingest_error("could not open the project store for writing");
        goto release_request;
    }

    struct sqlite3 *db = cbm_store_get_db(store);
    sqlite3_stmt *exists = NULL;
    if (!db || sqlite3_prepare_v2(db,
                                  "SELECT 1 FROM nodes WHERE project=?1 AND qualified_name=?2 "
                                  "LIMIT 1",
                                  -1, &exists, NULL) != SQLITE_OK) {
        result = ingest_error("store is not queryable");
        goto release_request;
    }

    long long pairs_stored = 0, pairs_unmatched = 0, paths_stored = 0, malformed = 0;
    const char *unmatched_sample[INGEST_UNMATCHED_SAMPLE];
    int unmatched_sample_count = 0;
    (void)sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, NULL);

    size_t ti, tmax;
    yyjson_val *trace;
    yyjson_arr_foreach(traces, ti, tmax, trace) {
        const char *hops[INGEST_PATH_MAX];
        size_t hop_count = 0;
        long long count = 1;
        yyjson_val *count_val = yyjson_obj_get(trace, "count");
        if (yyjson_is_int(count_val) && yyjson_get_int(count_val) > 0)
            count = yyjson_get_int(count_val);

        yyjson_val *path = yyjson_obj_get(trace, "path");
        if (path && yyjson_is_arr(path)) {
            size_t hi, hmax;
            yyjson_val *hop;
            bool ok = yyjson_arr_size(path) >= 2 && yyjson_arr_size(path) <= INGEST_PATH_MAX;
            yyjson_arr_foreach(path, hi, hmax, hop) {
                const char *qn = yyjson_get_str(hop);
                if (!qn || !qn[0] || hop_count >= INGEST_PATH_MAX) {
                    ok = false;
                    break;
                }
                hops[hop_count++] = qn;
            }
            if (!ok) {
                malformed++;
                continue;
            }
        } else {
            const char *caller = yyjson_get_str(yyjson_obj_get(trace, "caller"));
            const char *callee = yyjson_get_str(yyjson_obj_get(trace, "callee"));
            if (!caller || !caller[0] || !callee || !callee[0]) {
                malformed++;
                continue;
            }
            hops[0] = caller;
            hops[1] = callee;
            hop_count = 2;
        }

        /* Resolve every hop before storing anything from this trace: a
         * partially-resolved path is not a real journey. */
        bool all_resolved = true;
        for (size_t h = 0; h < hop_count; h++) {
            sqlite3_reset(exists);
            sqlite3_bind_text(exists, 1, project, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(exists, 2, hops[h], -1, SQLITE_TRANSIENT);
            if (sqlite3_step(exists) != SQLITE_ROW) {
                all_resolved = false;
                bool seen = false;
                for (int u = 0; u < unmatched_sample_count; u++)
                    if (strcmp(unmatched_sample[u], hops[h]) == 0) {
                        seen = true;
                        break;
                    }
                if (!seen && unmatched_sample_count < INGEST_UNMATCHED_SAMPLE)
                    unmatched_sample[unmatched_sample_count++] = hops[h];
            }
        }
        if (!all_resolved) {
            pairs_unmatched += (long long)(hop_count - 1);
            continue;
        }

        for (size_t h = 0; h + 1 < hop_count; h++) {
            if (cbm_store_observed_upsert_pair(store, project, hops[h], hops[h + 1], label,
                                               count) == CBM_STORE_OK)
                pairs_stored++;
        }
        if (path) {
            char *path_json =
                yyjson_val_write_opts(path, 0, cbm_json_writer_allocator(), NULL, NULL);
            if (path_json) {
                if (cbm_store_observed_upsert_path(store, project, path_json, label, count) ==
                    CBM_STORE_OK)
                    paths_stored++;
                cbm_free(CBM_MEM_CLASS_OTHER, path_json);
            }
        }
    }
    sqlite3_finalize(exists);
    (void)sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);

    /* Cap: evict whole oldest runs until under budget, since a partially
     * evicted run could masquerade as a complete recording. */
    long long evicted_runs = 0;
    long long pair_total = cbm_store_observed_pair_total(store, project);
    for (int round = 0; round < INGEST_EVICT_MAX_ROUNDS && pair_total > INGEST_PAIR_CAP; round++) {
        char *evicted_label = NULL;
        if (cbm_store_observed_evict_oldest_label(store, project, &evicted_label) != CBM_STORE_OK)
            break;
        evicted_runs++;
        bool evicted_self = evicted_label && strcmp(evicted_label, label) == 0;
        cbm_free(CBM_MEM_CLASS_OTHER, evicted_label);
        pair_total = cbm_store_observed_pair_total(store, project);
        if (evicted_self)
            break; /* the current run alone exceeds the cap; stop here */
    }

    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root) {
        if (doc)
            yyjson_mut_doc_free(doc);
        result = ingest_error("result allocation failed");
        goto release_request;
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_str(doc, root, "status", "stored");
    yyjson_mut_obj_add_strcpy(doc, root, "label", label);
    yyjson_mut_obj_add_int(doc, root, "pairs_stored", pairs_stored);
    yyjson_mut_obj_add_int(doc, root, "paths_stored", paths_stored);
    yyjson_mut_obj_add_int(doc, root, "pairs_unmatched", pairs_unmatched);
    yyjson_mut_val *sample = yyjson_mut_arr(doc);
    for (int u = 0; u < unmatched_sample_count; u++)
        yyjson_mut_arr_add_strcpy(doc, sample, unmatched_sample[u]);
    yyjson_mut_obj_add_val(doc, root, "unmatched_sample", sample);
    if (malformed > 0)
        yyjson_mut_obj_add_int(doc, root, "malformed", malformed);
    yyjson_mut_obj_add_int(doc, root, "pair_total", pair_total);
    if (evicted_runs > 0)
        yyjson_mut_obj_add_int(doc, root, "evicted_runs", evicted_runs);

    char *json = yyjson_mut_write(doc, 0, NULL);
    yyjson_mut_doc_free(doc);
    result = json ? cbm_operation_result_take(json, false) : ingest_error("result encoding failed");
release_request:
    if (adoc)
        yyjson_doc_free(adoc);
    if (owned_rw)
        cbm_store_close(owned_rw);
    if (mutation_held)
        runtime->mutation_end(runtime->mutation_context, project);
    cbm_operation_arg_free(project);
    return result;
}
