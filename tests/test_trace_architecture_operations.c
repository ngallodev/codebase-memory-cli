/*
 * test_trace_architecture_operations.c — ingest_traces and get_architecture
 * (system_structure) as native operations over real project databases in a
 * private cache directory. No mocks: the store, the observed_* tables, the
 * architecture job worker and the mutation lease callbacks are the real ones.
 *
 * These cover the behaviour upstream tested through the MCP server
 * (observed-call storage and eviction, mutation-guard balance, the
 * include_behavior_evidence argument and the generation-keyed job cache).
 */
#include "test_framework.h"
#include "test_helpers.h"

#include "foundation/compat.h"
#include "foundation/compat_fs.h"
#include "foundation/mem_core.h"
#include "foundation/platform.h"
#include "operations/architecture_jobs.h"
#include "operations/operation.h"
#include "operations/store_host.h"
#include "operations/tool_catalog.h"
#include "store/store.h"
#include "yyjson/yyjson.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAO_PROJECT "tao-traces"

typedef struct {
    char cache[512];
    char *saved_cache;
    char db_path[640];
} tao_fixture_t;

typedef struct {
    int begin_count;
    int end_count;
    int deny_begin_call;      /* 1-based call to refuse, 0 = never */
    int cancel_on_begin_call; /* 1-based call after which cancelled() is true */
    atomic_bool cancelled;
    char begin_projects[4][64];
    char end_projects[4][64];
} tao_guard_t;

static bool tao_mutation_begin(void *context, const char *project) {
    tao_guard_t *guard = context;
    int slot = guard->begin_count++;
    if (slot < 4) {
        snprintf(guard->begin_projects[slot], sizeof(guard->begin_projects[slot]), "%s", project);
    }
    if (guard->deny_begin_call == guard->begin_count) {
        return false;
    }
    if (guard->cancel_on_begin_call == guard->begin_count) {
        atomic_store(&guard->cancelled, true);
    }
    return true;
}

static void tao_mutation_end(void *context, const char *project) {
    tao_guard_t *guard = context;
    int slot = guard->end_count++;
    if (slot < 4) {
        snprintf(guard->end_projects[slot], sizeof(guard->end_projects[slot]), "%s", project);
    }
}

static bool tao_cancelled(void *context) {
    return atomic_load(&((tao_guard_t *)context)->cancelled);
}

static cbm_store_t *tao_store_resolve(void *context, const char *project,
                                      bool mutation_already_held, bool nonblocking_recovery,
                                      cbm_operation_store_recovery_status_t *status) {
    return cbm_store_host_resolve((cbm_store_host_t *)context, project, mutation_already_held,
                                  nonblocking_recovery, status);
}

static void tao_store_invalidate(void *context) {
    cbm_store_host_invalidate((cbm_store_host_t *)context);
}

static char *tao_store_error(void *context, const char *project) {
    (void)context;
    return cbm_store_host_error(project);
}

/* Run one operation with a runtime whose mutation lease callbacks are the
 * guard probe. */
static cbm_operation_result_t tao_execute(cbm_operation_id_t operation, tao_guard_t *guard,
                                          const char *args) {
    cbm_store_host_t *stores = cbm_store_host_new_deferred();
    cbm_operation_runtime_t runtime = {0};
    runtime.mutation_begin = tao_mutation_begin;
    runtime.mutation_try_begin = tao_mutation_begin;
    runtime.mutation_end = tao_mutation_end;
    runtime.mutation_context = guard;
    runtime.store_resolve = tao_store_resolve;
    runtime.store_invalidate = tao_store_invalidate;
    runtime.store_error = tao_store_error;
    runtime.store_context = stores;
    runtime.cancelled = tao_cancelled;
    runtime.cancelled_context = guard;
    cbm_operation_context_t context = {.runtime = &runtime};
    cbm_operation_result_t result = cbm_operation_execute(&context, operation, args);
    cbm_store_host_free(stores);
    return result;
}

static bool tao_fixture_open(tao_fixture_t *fx) {
    memset(fx, 0, sizeof(*fx));
    snprintf(fx->cache, sizeof(fx->cache), "%s/cbm-trace-arch-op-XXXXXX", cbm_tmpdir());
    if (!cbm_mkdtemp(fx->cache)) {
        return false;
    }
    const char *saved = getenv("CBM_CACHE_DIR");
    fx->saved_cache = saved ? strdup(saved) : NULL;
    snprintf(fx->db_path, sizeof(fx->db_path), "%s/%s.db", fx->cache, TAO_PROJECT);
    return cbm_setenv("CBM_CACHE_DIR", fx->cache, 1) == 0;
}

static void tao_fixture_close(tao_fixture_t *fx) {
    if (fx->saved_cache) {
        cbm_setenv("CBM_CACHE_DIR", fx->saved_cache, 1);
    } else {
        cbm_unsetenv("CBM_CACHE_DIR");
    }
    free(fx->saved_cache);
    if (fx->cache[0]) {
        th_rmtree(fx->cache);
    }
}

/* A project database with three Function nodes in the private cache. */
static bool tao_seed_project(tao_fixture_t *fx, const char *project) {
    char db_path[640];
    snprintf(db_path, sizeof(db_path), "%s/%s.db", fx->cache, project);
    cbm_store_t *store = cbm_store_open_path(db_path);
    if (!store || cbm_store_upsert_project(store, project, fx->cache) != CBM_STORE_OK) {
        cbm_store_close(store);
        return false;
    }
    static const char *const names[3] = {"alpha", "beta", "gamma"};
    for (int i = 0; i < 3; i++) {
        char qn[96];
        snprintf(qn, sizeof(qn), "%s::%s", project, names[i]);
        cbm_node_t node = {.project = project,
                           .label = "Function",
                           .name = names[i],
                           .qualified_name = qn,
                           .file_path = "src/t.c",
                           .start_line = 1,
                           .end_line = 3};
        if (cbm_store_upsert_node(store, &node) <= 0) {
            cbm_store_close(store);
            return false;
        }
    }
    cbm_store_close(store);
    return true;
}

static long long tao_observed(tao_fixture_t *fx, const char *a, const char *b, char *label,
                              size_t label_cap) {
    cbm_store_t *store = cbm_store_open_path_query(fx->db_path);
    long long count = -1;
    if (!store || cbm_store_observed_lookup(store, TAO_PROJECT, a, b, &count, label, label_cap,
                                            NULL, 0) != CBM_STORE_OK) {
        count = -1;
    }
    cbm_store_close(store);
    return count;
}

#define TAO_ALPHA TAO_PROJECT "::alpha"
#define TAO_BETA TAO_PROJECT "::beta"
#define TAO_GAMMA TAO_PROJECT "::gamma"

/* ingest_traces stores observed runtime calls qn-keyed (reindex-proof): paths
 * decompose into pairs, counts accumulate per run label, unresolved names are
 * reported rather than silently dropped, whole runs evict together. */
TEST(ingest_traces_stores_observed_calls) {
    tao_fixture_t fx;
    ASSERT_TRUE(tao_fixture_open(&fx));
    ASSERT_TRUE(tao_seed_project(&fx, TAO_PROJECT));
    tao_guard_t guard = {0};

    /* One 3-hop path, one bare pair with a count, one path with an unindexed
     * hop (skipped, reported), one malformed 1-hop path. */
    cbm_operation_result_t r =
        tao_execute(CBM_OPERATION_INGEST_TRACES, &guard,
                    "{\"project\":\"" TAO_PROJECT "\",\"label\":\"run1\",\"traces\":["
                    "{\"path\":[\"" TAO_ALPHA "\",\"" TAO_BETA "\",\"" TAO_GAMMA "\"]},"
                    "{\"caller\":\"" TAO_ALPHA "\",\"callee\":\"" TAO_GAMMA "\",\"count\":5},"
                    "{\"path\":[\"" TAO_ALPHA "\",\"" TAO_PROJECT "::missing\"]},"
                    "{\"path\":[\"" TAO_ALPHA "\"]}]}");
    ASSERT_NOT_NULL(r.payload);
    ASSERT_FALSE(r.is_error);
    ASSERT_NOT_NULL(strstr(r.payload, "\"status\":\"stored\""));
    ASSERT_NOT_NULL(strstr(r.payload, "\"pairs_stored\":3"));
    ASSERT_NOT_NULL(strstr(r.payload, "\"paths_stored\":1"));
    ASSERT_NOT_NULL(strstr(r.payload, "\"pairs_unmatched\":1"));
    ASSERT_NOT_NULL(strstr(r.payload, TAO_PROJECT "::missing"));
    ASSERT_NOT_NULL(strstr(r.payload, "\"malformed\":1"));
    cbm_operation_result_dispose(&r);

    char label[64] = {0};
    ASSERT_EQ(tao_observed(&fx, TAO_ALPHA, TAO_BETA, label, sizeof(label)), 1);
    ASSERT_STR_EQ(label, "run1");
    ASSERT_EQ(tao_observed(&fx, TAO_ALPHA, TAO_GAMMA, NULL, 0), 5);
    /* Never observed: zero count, not an error. */
    ASSERT_EQ(tao_observed(&fx, TAO_BETA, TAO_ALPHA, NULL, 0), 0);

    /* Same pair again under the same label accumulates. */
    r = tao_execute(CBM_OPERATION_INGEST_TRACES, &guard,
                    "{\"project\":\"" TAO_PROJECT "\",\"label\":\"run1\",\"traces\":[{\"caller\":"
                    "\"" TAO_ALPHA "\",\"callee\":\"" TAO_GAMMA "\",\"count\":2}]}");
    ASSERT_FALSE(r.is_error);
    cbm_operation_result_dispose(&r);
    ASSERT_EQ(tao_observed(&fx, TAO_ALPHA, TAO_GAMMA, NULL, 0), 7);

    /* A second label aggregates in the lookup and reports as the newest run. */
    r = tao_execute(CBM_OPERATION_INGEST_TRACES, &guard,
                    "{\"project\":\"" TAO_PROJECT "\",\"label\":\"run2\",\"traces\":[{\"caller\":"
                    "\"" TAO_ALPHA "\",\"callee\":\"" TAO_BETA "\"}]}");
    ASSERT_FALSE(r.is_error);
    cbm_operation_result_dispose(&r);
    ASSERT_EQ(tao_observed(&fx, TAO_ALPHA, TAO_BETA, label, sizeof(label)), 2);
    ASSERT_STR_EQ(label, "run2");

    /* Whole-run eviction: run1's newest observation is older, so run1 goes
     * first and run2 survives intact. */
    cbm_store_t *store = cbm_store_open_path(fx.db_path);
    ASSERT_NOT_NULL(store);
    char *evicted = NULL;
    ASSERT_EQ(cbm_store_observed_evict_oldest_label(store, TAO_PROJECT, &evicted), CBM_STORE_OK);
    ASSERT_NOT_NULL(evicted);
    ASSERT_STR_EQ(evicted, "run1");
    cbm_free(CBM_MEM_CLASS_OTHER, evicted);
    cbm_store_close(store);
    ASSERT_EQ(tao_observed(&fx, TAO_ALPHA, TAO_GAMMA, NULL, 0), 0);
    ASSERT_EQ(tao_observed(&fx, TAO_ALPHA, TAO_BETA, NULL, 0), 1);

    /* Empty traces and a missing project are tool errors, not silent accepts. */
    r = tao_execute(CBM_OPERATION_INGEST_TRACES, &guard,
                    "{\"project\":\"" TAO_PROJECT "\",\"traces\":[]}");
    ASSERT_TRUE(r.is_error);
    cbm_operation_result_dispose(&r);
    r = tao_execute(CBM_OPERATION_INGEST_TRACES, &guard,
                    "{\"traces\":[{\"caller\":\"a\",\"callee\":\"b\"}]}");
    ASSERT_TRUE(r.is_error);
    ASSERT_NULL(strstr(r.payload, "accepted"));
    cbm_operation_result_dispose(&r);

    tao_fixture_close(&fx);
    PASS();
}

#define TAO_GUARD_REQUEST                                                 \
    "{\"project\":\"" TAO_PROJECT "\",\"label\":\"guarded\",\"traces\":[" \
    "{\"path\":[\"" TAO_ALPHA "\",\"" TAO_BETA "\"]}]}"

TEST(ingest_traces_mutation_guard_denial_prevents_writes) {
    tao_fixture_t fx;
    ASSERT_TRUE(tao_fixture_open(&fx));
    ASSERT_TRUE(tao_seed_project(&fx, TAO_PROJECT));
    tao_guard_t guard = {.deny_begin_call = 1};
    cbm_operation_result_t r = tao_execute(CBM_OPERATION_INGEST_TRACES, &guard, TAO_GUARD_REQUEST);
    bool blocked = r.is_error && r.payload && strstr(r.payload, "blocked");
    cbm_operation_result_dispose(&r);
    ASSERT_TRUE(blocked);
    ASSERT_EQ(tao_observed(&fx, TAO_ALPHA, TAO_BETA, NULL, 0), 0);
    ASSERT_EQ(guard.begin_count, 1);
    ASSERT_EQ(guard.end_count, 0);
    ASSERT_STR_EQ(guard.begin_projects[0], TAO_PROJECT);
    tao_fixture_close(&fx);
    PASS();
}

TEST(ingest_traces_mutation_guard_balances_repeated_writes) {
    tao_fixture_t fx;
    ASSERT_TRUE(tao_fixture_open(&fx));
    ASSERT_TRUE(tao_seed_project(&fx, TAO_PROJECT));
    tao_guard_t guard = {0};
    bool stored = true;
    for (int i = 0; i < 2; i++) {
        cbm_operation_result_t r =
            tao_execute(CBM_OPERATION_INGEST_TRACES, &guard, TAO_GUARD_REQUEST);
        stored = stored && !r.is_error && r.payload && strstr(r.payload, "\"status\":\"stored\"") &&
                 strstr(r.payload, "\"pairs_stored\":1") && strstr(r.payload, "\"paths_stored\":1");
        cbm_operation_result_dispose(&r);
    }
    ASSERT_TRUE(stored);
    ASSERT_EQ(tao_observed(&fx, TAO_ALPHA, TAO_BETA, NULL, 0), 2);
    ASSERT_EQ(guard.begin_count, 2);
    ASSERT_EQ(guard.end_count, 2);
    for (int i = 0; i < 2; i++) {
        ASSERT_STR_EQ(guard.begin_projects[i], TAO_PROJECT);
        ASSERT_STR_EQ(guard.end_projects[i], TAO_PROJECT);
    }
    tao_fixture_close(&fx);
    PASS();
}

TEST(ingest_traces_mutation_guard_cancel_releases_without_writes) {
    tao_fixture_t fx;
    ASSERT_TRUE(tao_fixture_open(&fx));
    ASSERT_TRUE(tao_seed_project(&fx, TAO_PROJECT));
    tao_guard_t guard = {.cancel_on_begin_call = 1};
    cbm_operation_result_t r = tao_execute(CBM_OPERATION_INGEST_TRACES, &guard, TAO_GUARD_REQUEST);
    bool cancelled = r.is_error && r.payload && strstr(r.payload, "cancelled");
    cbm_operation_result_dispose(&r);
    ASSERT_TRUE(cancelled);
    ASSERT_EQ(tao_observed(&fx, TAO_ALPHA, TAO_BETA, NULL, 0), 0);
    ASSERT_EQ(guard.begin_count, 1);
    ASSERT_EQ(guard.end_count, 1);
    ASSERT_STR_EQ(guard.end_projects[0], TAO_PROJECT);
    tao_fixture_close(&fx);
    PASS();
}

TEST(ingest_traces_mutation_guard_missing_store_releases) {
    tao_fixture_t fx;
    ASSERT_TRUE(tao_fixture_open(&fx));
    tao_guard_t guard = {0};
    cbm_operation_result_t r = tao_execute(CBM_OPERATION_INGEST_TRACES, &guard, TAO_GUARD_REQUEST);
    bool missing = r.is_error && r.payload &&
                   (strstr(r.payload, "not found") || strstr(r.payload, "not indexed"));
    cbm_operation_result_dispose(&r);
    bool created = cbm_file_exists(fx.db_path);
    ASSERT_TRUE(missing);
    ASSERT_FALSE(created);
    ASSERT_EQ(guard.begin_count, 1);
    ASSERT_EQ(guard.end_count, 1);
    tao_fixture_close(&fx);
    PASS();
}

/* ingest_traces mutates observation data, so it must not run as a local read
 * and it needs a project; the descriptor is what the CLI routes on. */
TEST(ingest_traces_descriptor_is_a_project_mutation) {
    const cbm_operation_descriptor_t *op = cbm_operation_find("ingest_traces");
    ASSERT_NOT_NULL(op);
    ASSERT_FALSE(op->read_only);
    ASSERT_TRUE(op->requires_project);
    const char *schema = cbm_tool_catalog_input_schema("ingest_traces");
    ASSERT_NOT_NULL(schema);
    ASSERT_NOT_NULL(strstr(schema, "\"label\""));
    ASSERT_NOT_NULL(strstr(schema, "\"path\""));
    PASS();
}

TEST(get_architecture_catalog_lists_system_structure) {
    const char *schema = cbm_tool_catalog_input_schema("get_architecture");
    ASSERT_NOT_NULL(schema);
    yyjson_doc *doc = yyjson_read(schema, strlen(schema), 0);
    ASSERT_NOT_NULL(doc);
    yyjson_val *properties = yyjson_obj_get(yyjson_doc_get_root(doc), "properties");
    yyjson_val *aspects = yyjson_obj_get(yyjson_obj_get(properties, "aspects"), "items");
    yyjson_val *enum_arr = yyjson_obj_get(aspects, "enum");
    ASSERT_TRUE(yyjson_is_arr(enum_arr));
    static const char *const expected[] = {
        "all",       "overview",  "structure",    "dependencies", "routes",
        "languages", "packages",  "entry_points", "hotspots",     "boundaries",
        "layers",    "file_tree", "clusters",     "cycles",       "system_structure"};
    size_t expected_count = sizeof(expected) / sizeof(expected[0]);
    ASSERT_EQ(yyjson_arr_size(enum_arr), expected_count);
    for (size_t i = 0; i < expected_count; i++) {
        ASSERT_STR_EQ(yyjson_get_str(yyjson_arr_get(enum_arr, i)), expected[i]);
    }
    yyjson_val *flag = yyjson_obj_get(properties, "include_behavior_evidence");
    ASSERT_NOT_NULL(flag);
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(flag, "type")), "boolean");
    ASSERT_TRUE(yyjson_is_false(yyjson_obj_get(flag, "default")));
    ASSERT_NOT_NULL(yyjson_obj_get(properties, "entry_node_id"));
    ASSERT_NOT_NULL(yyjson_obj_get(properties, "target_node_id"));
    ASSERT_NOT_NULL(yyjson_obj_get(properties, "expected_generation"));
    yyjson_doc_free(doc);
    const char *description = cbm_tool_catalog_description("get_architecture");
    ASSERT_NOT_NULL(description);
    ASSERT_NOT_NULL(strstr(description, "include_behavior_evidence=true"));
    ASSERT_NOT_NULL(strstr(description, "call arguments"));
    ASSERT_NOT_NULL(strstr(description, "parameter declarations"));
    ASSERT_NOT_NULL(strstr(description, "return types"));
    ASSERT_NOT_NULL(strstr(description, "not full dataflow"));
    PASS();
}

TEST(get_architecture_behavior_evidence_validation) {
    tao_fixture_t fx;
    ASSERT_TRUE(tao_fixture_open(&fx));
    ASSERT_TRUE(tao_seed_project(&fx, "arch-flag"));
    tao_guard_t guard = {0};

    const char *invalid[] = {"\"true\"", "1", "0", "null", "[]", "{}"};
    const char *aspects[] = {"", ",\"aspects\":[\"system_structure\"]"};
    for (size_t a = 0; a < sizeof(aspects) / sizeof(aspects[0]); a++) {
        for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
            char args[256];
            snprintf(args, sizeof(args),
                     "{\"project\":\"arch-flag\"%s,\"include_behavior_evidence\":%s}", aspects[a],
                     invalid[i]);
            cbm_operation_result_t r = tao_execute(CBM_OPERATION_ARCHITECTURE, &guard, args);
            ASSERT_TRUE(r.is_error);
            ASSERT_NOT_NULL(r.payload);
            ASSERT_NOT_NULL(strstr(r.payload, "include_behavior_evidence must be a boolean"));
            cbm_operation_result_dispose(&r);
        }
    }
    cbm_operation_result_t wrong_scope =
        tao_execute(CBM_OPERATION_ARCHITECTURE, &guard,
                    "{\"project\":\"arch-flag\",\"include_behavior_evidence\":true}");
    ASSERT_TRUE(wrong_scope.is_error);
    ASSERT_NOT_NULL(strstr(wrong_scope.payload, "requires aspects"));
    cbm_operation_result_dispose(&wrong_scope);

    /* system_structure is requested alone, without a path. */
    cbm_operation_result_t with_path = tao_execute(
        CBM_OPERATION_ARCHITECTURE, &guard,
        "{\"project\":\"arch-flag\",\"aspects\":[\"system_structure\"],\"path\":\"src\"}");
    ASSERT_TRUE(with_path.is_error);
    ASSERT_NOT_NULL(strstr(with_path.payload, "system_structure alone"));
    cbm_operation_result_dispose(&with_path);
    cbm_operation_result_t two_aspects =
        tao_execute(CBM_OPERATION_ARCHITECTURE, &guard,
                    "{\"project\":\"arch-flag\",\"aspects\":[\"system_structure\",\"languages\"]}");
    ASSERT_TRUE(two_aspects.is_error);
    cbm_operation_result_dispose(&two_aspects);
    cbm_operation_result_t target_only = tao_execute(
        CBM_OPERATION_ARCHITECTURE, &guard,
        "{\"project\":\"arch-flag\",\"aspects\":[\"system_structure\"],\"target_node_id\":2}");
    ASSERT_TRUE(target_only.is_error);
    cbm_operation_result_dispose(&target_only);

    cbm_operation_result_t implicit =
        tao_execute(CBM_OPERATION_ARCHITECTURE, &guard, "{\"project\":\"arch-flag\"}");
    cbm_operation_result_t explicit_false =
        tao_execute(CBM_OPERATION_ARCHITECTURE, &guard,
                    "{\"project\":\"arch-flag\",\"include_behavior_evidence\":false}");
    ASSERT_FALSE(implicit.is_error);
    ASSERT_NOT_NULL(implicit.payload);
    ASSERT_NOT_NULL(explicit_false.payload);
    ASSERT_STR_EQ(implicit.payload, explicit_false.payload);
    cbm_operation_result_dispose(&implicit);
    cbm_operation_result_dispose(&explicit_false);

    cbm_architecture_jobs_shared_release();
    tao_fixture_close(&fx);
    PASS();
}

/* Poll the stable terminal job state, without asserting a transient pending
 * window or a particular number of worker polls. */
static char *tao_architecture_wait(tao_guard_t *guard, const char *args) {
    uint64_t deadline = cbm_now_ms() + 10000;
    for (;;) {
        cbm_operation_result_t r = tao_execute(CBM_OPERATION_ARCHITECTURE, guard, args);
        char *text = r.payload ? strdup(r.payload) : NULL;
        cbm_operation_result_dispose(&r);
        yyjson_doc *doc = text ? yyjson_read(text, strlen(text), 0) : NULL;
        const char *status =
            doc ? yyjson_get_str(yyjson_obj_get(yyjson_doc_get_root(doc), "status")) : NULL;
        bool pending = status && strcmp(status, "pending") == 0;
        yyjson_doc_free(doc);
        if (!pending || cbm_now_ms() >= deadline) {
            return text;
        }
        free(text);
        cbm_usleep(1000);
    }
}

/* The behaviour-evidence flag is part of the job-cache key: a result computed
 * without evidence is never served for a request with it, or the reverse. */
TEST(get_architecture_behavior_evidence_cache_isolation) {
    tao_fixture_t fx;
    ASSERT_TRUE(tao_fixture_open(&fx));
    const char *project = "arch-evidence-cache";
    char db_path[640];
    snprintf(db_path, sizeof(db_path), "%s/%s.db", fx.cache, project);
    cbm_store_t *store = cbm_store_open_path(db_path);
    ASSERT_NOT_NULL(store);
    ASSERT_EQ(cbm_store_upsert_project(store, project, fx.cache), CBM_STORE_OK);
    cbm_node_t entry = {.project = project,
                        .label = "Function",
                        .name = "entry",
                        .qualified_name = "arch-evidence-cache.entry",
                        .file_path = "src/entry.c",
                        .start_line = 1,
                        .end_line = 5,
                        .properties_json = "{\"return_type\":\"EvidenceCacheSentinel\"}"};
    int64_t entry_id = cbm_store_upsert_node(store, &entry);
    ASSERT_GT(entry_id, 0);
    cbm_node_t target = {.project = project,
                         .label = "Function",
                         .name = "target",
                         .qualified_name = "arch-evidence-cache.target",
                         .file_path = "src/target.c",
                         .start_line = 1,
                         .end_line = 3,
                         .properties_json = "{\"return_type\":\"EvidenceCacheSentinel\"}"};
    int64_t target_id = cbm_store_upsert_node(store, &target);
    ASSERT_GT(target_id, 0);
    cbm_edge_t edge = {.project = project,
                       .source_id = entry_id,
                       .target_id = target_id,
                       .type = "CALLS",
                       .properties_json = "{\"line\":2}"};
    ASSERT_GT(cbm_store_insert_edge(store, &edge), 0);
    cbm_store_close(store);

    tao_guard_t guard = {0};
    char args[512];
    const char *flags[] = {"", ",\"include_behavior_evidence\":true",
                           ",\"include_behavior_evidence\":false",
                           ",\"include_behavior_evidence\":true"};
    char *results[4] = {0};
    for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
        snprintf(args, sizeof(args),
                 "{\"project\":\"%s\",\"aspects\":[\"system_structure\"],"
                 "\"entry_node_id\":%lld%s}",
                 project, (long long)entry_id, flags[i]);
        results[i] = tao_architecture_wait(&guard, args);
        ASSERT_NOT_NULL(results[i]);
        ASSERT_NOT_NULL(strstr(results[i], "\"status\":\"ready\""));
        if (i % 2 == 0) {
            ASSERT_NULL(strstr(results[i], "EvidenceCacheSentinel"));
            ASSERT_NULL(strstr(results[i], "\"return_type\""));
        } else {
            ASSERT_NOT_NULL(strstr(results[i], "EvidenceCacheSentinel"));
            ASSERT_NOT_NULL(strstr(results[i], "\"return_type\""));
        }
    }
    ASSERT_STR_EQ(results[0], results[2]);
    ASSERT_STR_EQ(results[1], results[3]);
    for (size_t i = 0; i < sizeof(results) / sizeof(results[0]); i++) {
        free(results[i]);
    }
    cbm_architecture_jobs_shared_release();
    tao_fixture_close(&fx);
    PASS();
}

SUITE(trace_architecture_operations) {
    RUN_TEST(ingest_traces_stores_observed_calls);
    RUN_TEST(ingest_traces_mutation_guard_denial_prevents_writes);
    RUN_TEST(ingest_traces_mutation_guard_balances_repeated_writes);
    RUN_TEST(ingest_traces_mutation_guard_cancel_releases_without_writes);
    RUN_TEST(ingest_traces_mutation_guard_missing_store_releases);
    RUN_TEST(ingest_traces_descriptor_is_a_project_mutation);
    RUN_TEST(get_architecture_catalog_lists_system_structure);
    RUN_TEST(get_architecture_behavior_evidence_validation);
    RUN_TEST(get_architecture_behavior_evidence_cache_isolation);
}
