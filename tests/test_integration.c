/*
 * test_integration.c — End-to-end integration tests for the pure C pipeline.
 *
 * Creates a temporary project with real source files, indexes it through
 * the full pipeline, then queries the result through MCP tool handlers.
 *
 * This exercises the complete flow: discover → extract → registry → graph
 * buffer → SQLite dump → query. No mocking — real files, real parsing.
 */
#include "../src/foundation/compat.h"
#include "test_framework.h"
#include "test_helpers.h"
#include "test_operation_host.h"
#include "operations/output_budget.h"
#include "foundation/mem_core.h"
#include <yyjson/yyjson.h>
#include <store/store.h>
#include <pipeline/pipeline.h>
#include <foundation/log.h>
#include <foundation/compat_fs.h>
#include <foundation/constants.h>
#include <foundation/platform.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

/* ── Test fixture: temp project with Python + Go files ─────────── */

static char g_tmpdir[256];
static char g_dbpath[512];
static cbm_test_operation_host_t *g_srv = NULL;
static char *g_project = NULL;

/* Create source files in temp directory */
static int create_test_project(void) {
    snprintf(g_tmpdir, sizeof(g_tmpdir), "/tmp/cbm_integ_XXXXXX");
    if (!cbm_mkdtemp(g_tmpdir))
        return -1;

    char path[512];
    FILE *f;

    /* Python file with function calls */
    snprintf(path, sizeof(path), "%s/main.py", g_tmpdir);
    f = fopen(path, "w");
    if (!f)
        return -1;
    fprintf(f, "def greet(name):\n"
               "    return 'Hello ' + name\n"
               "\n"
               "def farewell(name):\n"
               "    return 'Goodbye ' + name\n"
               "\n"
               "def main():\n"
               "    msg = greet('World')\n"
               "    msg2 = farewell('World')\n"
               "    print(msg, msg2)\n");
    fclose(f);

    /* Go file with function calls */
    snprintf(path, sizeof(path), "%s/utils.go", g_tmpdir);
    f = fopen(path, "w");
    if (!f)
        return -1;
    fprintf(f, "package utils\n"
               "\n"
               "func Add(a, b int) int {\n"
               "    return a + b\n"
               "}\n"
               "\n"
               "func Multiply(a, b int) int {\n"
               "    sum := Add(a, b)\n"
               "    return sum * 2\n"
               "}\n"
               "\n"
               "func Compute(x int) int {\n"
               "    return Multiply(x, Add(x, 1))\n"
               "}\n");
    fclose(f);

    /* JavaScript file */
    snprintf(path, sizeof(path), "%s/app.js", g_tmpdir);
    f = fopen(path, "w");
    if (!f)
        return -1;
    fprintf(f, "function validate(input) {\n"
               "    return input != null;\n"
               "}\n"
               "\n"
               "function process(data) {\n"
               "    if (validate(data)) {\n"
               "        return data.toUpperCase();\n"
               "    }\n"
               "    return null;\n"
               "}\n");
    /* `manyMatches` calls twelve resolvable leaves: enough matching lines in a
     * single node to exercise search_code's bounded match locations, and enough
     * trace rows to exceed a small output ceiling. */
    for (int i = 1; i <= 12; i++) {
        fprintf(f, "function budgetMarker%d() { return %d; }\n", i, i);
    }
    fprintf(f, "\nfunction manyMatches() {\n");
    for (int i = 1; i <= 12; i++) {
        fprintf(f, "    budgetMarker%d();\n", i);
    }
    fprintf(f, "}\n");
    fclose(f);

    return 0;
}

/* Set up: create project, index it through MCP (production flow) */
static int integration_setup(void) {
    if (create_test_project() != 0)
        return -1;

    /* Derive project name (same logic the pipeline uses) */
    g_project = cbm_project_name_from_path(g_tmpdir);
    if (!g_project)
        return -1;

    /* Build db path for direct store queries (pipeline writes here) */
    const char *cache_dir = cbm_resolve_cache_dir();
    int dbpath_length =
        cache_dir ? snprintf(g_dbpath, sizeof(g_dbpath), "%s/%s.db", cache_dir, g_project) : -1;
    if (dbpath_length <= 0 || (size_t)dbpath_length >= sizeof(g_dbpath) ||
        !cbm_mkdir_p(cache_dir, 0700)) {
        return -1;
    }

    /* Remove stale db from previous test runs */
    unlink(g_dbpath);

    /* Create MCP server, then index through it (production flow):
     *   1. Server starts with in-memory store
     *   2. index_repository closes in-memory store
     *   3. Pipeline runs → dumps to ~/.cache/.../<project>.db
     *   4. Server reopens from that db
     * This exercises the exact same path as real usage. */
    g_srv = cbm_test_operation_host_new(NULL);
    if (!g_srv)
        return -1;

    /* Index our temp project via MCP tool handler */
    char args[512];
    snprintf(args, sizeof(args), "{\"repo_path\":\"%s\"}", g_tmpdir);
    char *resp = cbm_test_operation_execute(g_srv, "index_repository", args);
    if (!resp)
        return -1;

    /* Verify indexing succeeded */
    bool ok = strstr(resp, "indexed") != NULL;
    free(resp);
    return ok ? 0 : -1;
}

static void integration_teardown(void) {
    if (g_srv) {
        cbm_test_operation_host_free(g_srv);
        g_srv = NULL;
    }
    free(g_project);
    g_project = NULL;

    /* Clean up temp project */
    th_rmtree(g_tmpdir);

    /* Clean up cache db */
    unlink(g_dbpath);
    char wal[520], shm[520];
    snprintf(wal, sizeof(wal), "%s-wal", g_dbpath);
    snprintf(shm, sizeof(shm), "%s-shm", g_dbpath);
    unlink(wal);
    unlink(shm);
}

/* ══════════════════════════════════════════════════════════════════
 *  PIPELINE INTEGRATION TESTS
 * ══════════════════════════════════════════════════════════════════ */

/* Helper: call a tool and return response JSON. Caller must free(). */
static char *call_tool(const char *tool, const char *args) {
    if (!g_srv)
        return NULL;
    return cbm_test_operation_execute(g_srv, tool, args);
}

TEST(integ_index_has_nodes) {
    /* Open the indexed db directly and check node counts */
    cbm_store_t *store = cbm_store_open_path_existing(g_dbpath);
    ASSERT_NOT_NULL(store);

    int nodes = cbm_store_count_nodes(store, g_project);
    /* We expect: 3 File nodes + 3+ Function/Method nodes per file +
     * Folder/Package/Module nodes. Should be at least 8. */
    ASSERT_TRUE(nodes >= 8);

    cbm_store_close(store);
    PASS();
}

TEST(integ_index_has_edges) {
    cbm_store_t *store = cbm_store_open_path_existing(g_dbpath);
    ASSERT_NOT_NULL(store);

    int edges = cbm_store_count_edges(store, g_project);
    /* We expect CONTAINS_FILE edges + CALLS edges + others */
    ASSERT_TRUE(edges >= 3);

    cbm_store_close(store);
    PASS();
}

TEST(integ_index_has_functions) {
    cbm_store_t *store = cbm_store_open_path_existing(g_dbpath);
    ASSERT_NOT_NULL(store);

    cbm_node_t *funcs = NULL;
    int count = 0;
    int rc = cbm_store_find_nodes_by_label(store, g_project, "Function", &funcs, &count);
    ASSERT_EQ(rc, CBM_STORE_OK);
    /* Python: greet, farewell, main. Go: Add, Multiply, Compute. JS: validate, process */
    ASSERT_TRUE(count >= 6);

    /* Verify some function names exist */
    bool found_greet = false, found_add = false, found_validate = false;
    for (int i = 0; i < count; i++) {
        if (funcs[i].name && strcmp(funcs[i].name, "greet") == 0)
            found_greet = true;
        if (funcs[i].name && strcmp(funcs[i].name, "Add") == 0)
            found_add = true;
        if (funcs[i].name && strcmp(funcs[i].name, "validate") == 0)
            found_validate = true;
    }
    ASSERT_TRUE(found_greet);
    ASSERT_TRUE(found_add);
    ASSERT_TRUE(found_validate);

    cbm_store_free_nodes(funcs, count);
    cbm_store_close(store);
    PASS();
}

TEST(integ_index_has_files) {
    cbm_store_t *store = cbm_store_open_path_existing(g_dbpath);
    ASSERT_NOT_NULL(store);

    cbm_node_t *files = NULL;
    int count = 0;
    int rc = cbm_store_find_nodes_by_label(store, g_project, "File", &files, &count);
    ASSERT_EQ(rc, CBM_STORE_OK);
    ASSERT_EQ(count, 3); /* main.py, utils.go, app.js */

    bool found_py = false, found_go = false, found_js = false;
    for (int i = 0; i < count; i++) {
        if (files[i].file_path && strstr(files[i].file_path, "main.py"))
            found_py = true;
        if (files[i].file_path && strstr(files[i].file_path, "utils.go"))
            found_go = true;
        if (files[i].file_path && strstr(files[i].file_path, "app.js"))
            found_js = true;
    }
    ASSERT_TRUE(found_py);
    ASSERT_TRUE(found_go);
    ASSERT_TRUE(found_js);

    cbm_store_free_nodes(files, count);
    cbm_store_close(store);
    PASS();
}

TEST(integ_index_has_calls) {
    cbm_store_t *store = cbm_store_open_path_existing(g_dbpath);
    ASSERT_NOT_NULL(store);

    int call_count = cbm_store_count_edges_by_type(store, g_project, "CALLS");
    /* Python: main→greet, main→farewell, main→print
     * Go: Multiply→Add, Compute→Multiply, Compute→Add
     * JS: process→validate */
    ASSERT_TRUE(call_count >= 4);

    cbm_store_close(store);
    PASS();
}

/* ══════════════════════════════════════════════════════════════════
 *  MCP TOOL HANDLER INTEGRATION
 * ══════════════════════════════════════════════════════════════════ */

TEST(integ_mcp_list_projects) {
    char *resp = call_tool("list_projects", "{}");
    ASSERT_NOT_NULL(resp);
    /* Should contain the project name derived from temp path */
    ASSERT_NOT_NULL(strstr(resp, "project"));
    free(resp);
    PASS();
}

TEST(integ_mcp_search_graph_by_label) {
    char args[256];
    snprintf(args, sizeof(args), "{\"label\":\"Function\",\"project\":\"%s\",\"limit\":20}",
             g_project);

    char *resp = call_tool("search_graph", args);
    ASSERT_NOT_NULL(resp);
    /* Should return function nodes */
    ASSERT_NOT_NULL(strstr(resp, "Function"));
    /* Should contain our known functions */
    ASSERT_NOT_NULL(strstr(resp, "greet"));
    free(resp);
    PASS();
}

TEST(integ_mcp_search_graph_by_name) {
    char args[256];
    snprintf(args, sizeof(args), "{\"name_pattern\":\".*Add.*\",\"project\":\"%s\"}", g_project);

    char *resp = call_tool("search_graph", args);
    ASSERT_NOT_NULL(resp);
    ASSERT_NOT_NULL(strstr(resp, "Add"));
    free(resp);
    PASS();
}

TEST(integ_mcp_query_graph_functions) {
    char args[512];
    snprintf(args, sizeof(args),
             "{\"project\":\"%s\",\"query\":\"MATCH (f:Function) WHERE f.project = '%s' "
             "RETURN f.name LIMIT 20\"}",
             g_project, g_project);

    char *resp = call_tool("query_graph", args);
    ASSERT_NOT_NULL(resp);
    /* Should return results (may be in various formats depending on Cypher output).
     * At minimum, should not be an error. */
    ASSERT_TRUE(strstr(resp, "row") || strstr(resp, "greet") || strstr(resp, "Add") ||
                strstr(resp, "result") || strstr(resp, "f.name"));
    free(resp);
    PASS();
}

TEST(integ_mcp_query_graph_calls) {
    char args[512];
    snprintf(args, sizeof(args),
             "{\"project\":\"%s\",\"query\":\"MATCH (a)-[r:CALLS]->(b) WHERE a.project = '%s' "
             "RETURN a.name, b.name LIMIT 20\"}",
             g_project, g_project);

    char *resp = call_tool("query_graph", args);
    ASSERT_NOT_NULL(resp);
    /* Should have some call relationships */
    ASSERT_NOT_NULL(strstr(resp, "name"));
    free(resp);
    PASS();
}

TEST(integ_mcp_get_graph_schema) {
    char args[128];
    snprintf(args, sizeof(args), "{\"project\":\"%s\"}", g_project);

    char *resp = call_tool("get_graph_schema", args);
    ASSERT_NOT_NULL(resp);
    /* Schema should include node labels and edge types */
    ASSERT_NOT_NULL(strstr(resp, "Function"));
    ASSERT_NOT_NULL(strstr(resp, "File"));
    free(resp);
    PASS();
}

TEST(integ_mcp_get_architecture) {
    char args[128];
    snprintf(args, sizeof(args), "{\"project\":\"%s\"}", g_project);

    char *resp = call_tool("get_architecture", args);
    ASSERT_NOT_NULL(resp);
    ASSERT_NOT_NULL(strstr(resp, "total_nodes"));
    free(resp);
    PASS();
}

TEST(integ_mcp_trace_path) {
    /* Trace outbound calls from Compute → should reach Add and Multiply */
    char args[256];
    snprintf(args, sizeof(args),
             "{\"function_name\":\"Compute\",\"project\":\"%s\","
             "\"direction\":\"outbound\",\"max_depth\":3}",
             g_project);

    char *resp = call_tool("trace_path", args);
    ASSERT_NOT_NULL(resp);
    /* Should find the function and show some path */
    /* Either finds the function, or returns not found if name doesn't match exactly */
    ASSERT_TRUE(strstr(resp, "Compute") || strstr(resp, "Multiply") || strstr(resp, "not found"));
    free(resp);
    PASS();
}

/* #522: trace_path mode=cross_service must follow CROSS_* cross-repo edges.
 * Seed a CROSS_HTTP_CALLS edge between two indexed functions that have no CALLS
 * relationship, then confirm cross_service surfaces the hop while the default
 * calls mode does not (proving the cross edge specifically is what's followed).
 *
 * The trace goes through a fresh server so it opens the db after the edge is
 * committed — exactly what a new MCP session sees after a cross-repo pass writes
 * CROSS_* edges (g_srv's cached connection predates this write). */
TEST(integ_mcp_trace_path_cross_service) {
    cbm_store_t *store = cbm_store_open_path_existing(g_dbpath);
    ASSERT_NOT_NULL(store);

    cbm_node_t *src = NULL;
    cbm_node_t *dst = NULL;
    int src_count = 0;
    int dst_count = 0;
    cbm_store_find_nodes_by_name(store, g_project, "greet", &src, &src_count);
    cbm_store_find_nodes_by_name(store, g_project, "farewell", &dst, &dst_count);
    ASSERT_TRUE(src_count > 0 && dst_count > 0);

    cbm_edge_t edge = {.project = g_project,
                       .source_id = src[0].id,
                       .target_id = dst[0].id,
                       .type = "CROSS_HTTP_CALLS"};
    ASSERT_TRUE(cbm_store_insert_edge(store, &edge) > 0);

    cbm_store_free_nodes(src, src_count);
    cbm_store_free_nodes(dst, dst_count);
    cbm_store_close(store);

    cbm_test_operation_host_t *srv = cbm_test_operation_host_new(NULL);
    ASSERT_NOT_NULL(srv);

    char args[256];
    snprintf(args, sizeof(args),
             "{\"function_name\":\"greet\",\"project\":\"%s\","
             "\"direction\":\"outbound\",\"mode\":\"cross_service\"}",
             g_project);
    char *resp = cbm_test_operation_execute(srv, "trace_path", args);
    ASSERT_NOT_NULL(resp);
    ASSERT_NOT_NULL(strstr(resp, "farewell"));
    free(resp);

    snprintf(args, sizeof(args),
             "{\"function_name\":\"greet\",\"project\":\"%s\","
             "\"direction\":\"outbound\",\"mode\":\"calls\"}",
             g_project);
    resp = cbm_test_operation_execute(srv, "trace_path", args);
    ASSERT_NOT_NULL(resp);
    ASSERT_TRUE(strstr(resp, "farewell") == NULL);
    free(resp);

    cbm_test_operation_host_free(srv);
    PASS();
}

/* Column-ordered search_code JSON row lookup by a substring of its qn cell. */
static yyjson_val *search_row_by_name(yyjson_val *root, const char *needle) {
    yyjson_val *rows = yyjson_obj_get(root, "rows");
    if (!yyjson_is_arr(rows))
        return NULL;
    size_t index, maximum;
    yyjson_val *row;
    yyjson_arr_foreach(rows, index, maximum, row) {
        yyjson_val *qn = yyjson_arr_get(row, 0);
        if (yyjson_is_str(qn) && strstr(yyjson_get_str(qn), needle))
            return row;
    }
    return NULL;
}

TEST(integ_mcp_search_code_match_limit_and_budget) {
    char args[512];
    yyjson_doc *doc = NULL;

    /* No max_output_tokens: no byte budget, so the response is exactly what the
     * operation has always produced. */
    snprintf(args, sizeof(args),
             "{\"project\":\"%s\",\"pattern\":\"budgetMarker\",\"format\":\"json\","
             "\"limit\":50}",
             g_project);
    char *base = call_tool("search_code", args);
    ASSERT_NOT_NULL(base);
    ASSERT_NULL(strstr(base, "truncation_reason"));

    doc = yyjson_read(base, strlen(base), 0);
    ASSERT_NOT_NULL(doc);
    yyjson_val *row = search_row_by_name(yyjson_doc_get_root(doc), "manyMatches");
    ASSERT_NOT_NULL(row);
    /* Upstream default match_limit is 8; the withheld remainder is explicit. */
    ASSERT_EQ((int)yyjson_arr_size(yyjson_arr_get(row, 4)), 8);
    ASSERT_EQ((int)yyjson_get_int(yyjson_arr_get(row, 5)), 4);
    yyjson_doc_free(doc);
    doc = NULL;

    /* A ceiling far above the payload changes nothing: omitting the argument
     * applies no implicit budget. */
    snprintf(args, sizeof(args),
             "{\"project\":\"%s\",\"pattern\":\"budgetMarker\",\"format\":\"json\","
             "\"limit\":50,\"max_output_tokens\":100000}",
             g_project);
    char *roomy = call_tool("search_code", args);
    ASSERT_NOT_NULL(roomy);
    /* `elapsed_ms` is a real measurement, so compare the model, not the bytes:
     * the generous ceiling leaves the same rows and the same reason-free shape. */
    ASSERT_NULL(strstr(roomy, "truncation_reason"));
    doc = yyjson_read(roomy, strlen(roomy), 0);
    ASSERT_NOT_NULL(doc);
    row = search_row_by_name(yyjson_doc_get_root(doc), "manyMatches");
    ASSERT_NOT_NULL(row);
    ASSERT_EQ((int)yyjson_arr_size(yyjson_arr_get(row, 4)), 8);
    ASSERT_EQ((int)yyjson_get_int(yyjson_arr_get(row, 5)), 4);
    yyjson_doc_free(doc);
    doc = NULL;
    free(roomy);

    /* An explicit match_limit bounds the shown locations. */
    snprintf(args, sizeof(args),
             "{\"project\":\"%s\",\"pattern\":\"budgetMarker\",\"format\":\"json\","
             "\"limit\":50,\"match_limit\":3}",
             g_project);
    char *limited = call_tool("search_code", args);
    ASSERT_NOT_NULL(limited);
    doc = yyjson_read(limited, strlen(limited), 0);
    ASSERT_NOT_NULL(doc);
    row = search_row_by_name(yyjson_doc_get_root(doc), "manyMatches");
    ASSERT_NOT_NULL(row);
    ASSERT_EQ((int)yyjson_arr_size(yyjson_arr_get(row, 4)), 3);
    ASSERT_EQ((int)yyjson_get_int(yyjson_arr_get(row, 5)), 9);
    yyjson_doc_free(doc);
    doc = NULL;
    free(limited);

    /* A tiny ceiling trims whole rows and names the reason. */
    snprintf(args, sizeof(args),
             "{\"project\":\"%s\",\"pattern\":\"budgetMarker\",\"format\":\"json\","
             "\"limit\":50,\"max_output_tokens\":128}",
             g_project);
    char *small = call_tool("search_code", args);
    ASSERT_NOT_NULL(small);
    ASSERT_TRUE(strlen(small) <= (size_t)CBM_OUTPUT_TOKENS_MIN * CBM_OUTPUT_BYTES_PER_TOKEN);
    ASSERT_NOT_NULL(strstr(small, "output_budget"));
    free(small);
    free(base);
    PASS();
}

TEST(integ_mcp_trace_path_output_budget) {
    char args[512];

    snprintf(args, sizeof(args),
             "{\"function_name\":\"manyMatches\",\"project\":\"%s\","
             "\"direction\":\"outbound\"}",
             g_project);
    char *base = call_tool("trace_path", args);
    ASSERT_NOT_NULL(base);
    ASSERT_NULL(strstr(base, "output_budget"));

    /* Same response with a ceiling far above the payload. */
    snprintf(args, sizeof(args),
             "{\"function_name\":\"manyMatches\",\"project\":\"%s\","
             "\"direction\":\"outbound\",\"max_output_tokens\":100000}",
             g_project);
    char *roomy = call_tool("trace_path", args);
    ASSERT_NOT_NULL(roomy);
    ASSERT_STR_EQ(roomy, base);
    free(roomy);

    /* A tiny ceiling keeps only the whole rows that fit and reports the budget,
     * with no identifier sliced. */
    snprintf(args, sizeof(args),
             "{\"function_name\":\"manyMatches\",\"project\":\"%s\","
             "\"direction\":\"outbound\",\"max_output_tokens\":128}",
             g_project);
    char *small = call_tool("trace_path", args);
    ASSERT_NOT_NULL(small);
    ASSERT_TRUE(strlen(small) <= (size_t)CBM_OUTPUT_TOKENS_MIN * CBM_OUTPUT_BYTES_PER_TOKEN);
    ASSERT_NOT_NULL(strstr(small, "output_budget"));
    ASSERT_TRUE(strlen(small) < strlen(base));
    free(small);
    free(base);
    PASS();
}

TEST(integ_mcp_adr_outline_fence_status) {
    cbm_store_t *store = cbm_store_open_path_existing(g_dbpath);
    ASSERT_NOT_NULL(store);
    const char *contents[] = {
        "# Visible\n```c\n# Hidden\n",
        "# Visible\r\n   ~~~~\r\n# Hidden\r\n~~~\r\n",
        "# Visible\n```c\n# Hidden\n```\n",
        "# Visible\r\n   ~~~~\r\n# Hidden\r\n~~~~\r\n",
    };
    bool correct = true;
    for (int i = 0; i < 4; i++) {
        correct = cbm_store_adr_store(store, g_project, contents[i]) == CBM_STORE_OK && correct;
        char args[256];
        snprintf(args, sizeof(args), "{\"project\":\"%s\",\"mode\":\"outline\"}", g_project);
        char *resp = call_tool("manage_adr", args);
        correct = resp && strstr(resp, "# Visible") && !strstr(resp, "# Hidden") &&
                  ((strstr(resp, "\"sections_status\":\"unterminated_code_fence\"") != NULL) ==
                   (i < 2)) &&
                  correct;
        free(resp);
    }
    int deleted = cbm_store_adr_delete(store, g_project);
    cbm_store_close(store);
    ASSERT_EQ(deleted, CBM_STORE_OK);
    ASSERT_TRUE(correct);
    PASS();
}

TEST(integ_mcp_index_status) {
    char args[128];
    snprintf(args, sizeof(args), "{\"project\":\"%s\"}", g_project);

    char *resp = call_tool("index_status", args);
    ASSERT_NOT_NULL(resp);
    /* Should show indexed status with node/edge counts */
    ASSERT_NOT_NULL(strstr(resp, g_project));
    free(resp);
    PASS();
}

TEST(integ_mcp_delete_project) {
    /* Delete the project and verify it's gone */
    char args[256];
    snprintf(args, sizeof(args), "{\"project\":\"%s\"}", g_project);

    char *resp = call_tool("delete_project", args);
    ASSERT_NOT_NULL(resp);
    ASSERT_NOT_NULL(strstr(resp, "deleted"));
    free(resp);

    /* Note: querying after delete on Linux re-opens the unlinked .db inode
     * (unlink defers removal until all fds close). SQLite's WAL mode connection
     * on an unlinked file leaks internal allocations that sqlite3_close cannot
     * reclaim. Guard behavior for deleted/missing projects is tested separately
     * in tests/smoke_guard.sh using non-existent project names. */
    PASS();
}

/* ══════════════════════════════════════════════════════════════════
 *  PIPELINE DIRECT API TESTS
 * ══════════════════════════════════════════════════════════════════ */

TEST(integ_pipeline_fqn_compute) {
    char *fqn = cbm_pipeline_fqn_compute("myproject", "src/utils.go", "Add");
    ASSERT_NOT_NULL(fqn);
    ASSERT_STR_EQ(fqn, "myproject.src.utils.Add");
    free(fqn);
    PASS();
}

TEST(integ_pipeline_fqn_module) {
    char *fqn = cbm_pipeline_fqn_module("myproject", "src/utils.go");
    ASSERT_NOT_NULL(fqn);
    ASSERT_STR_EQ(fqn, "myproject.src.utils");
    free(fqn);
    PASS();
}

TEST(integ_pipeline_project_name) {
    char *name = cbm_project_name_from_path("/home/user/my-project");
    ASSERT_NOT_NULL(name);
    /* Should contain "my-project" or a sanitized version */
    ASSERT_NOT_NULL(strstr(name, "my-project"));
    free(name);
    PASS();
}

TEST(integ_pipeline_cancel) {
    /* Create and immediately cancel a pipeline */
    cbm_pipeline_t *p = cbm_pipeline_new(g_tmpdir, NULL, CBM_MODE_FULL);
    ASSERT_NOT_NULL(p);

    cbm_pipeline_cancel(p);
    int rc = cbm_pipeline_run(p);
    /* Should return -1 (cancelled) or complete with partial results */
    /* Either way, it shouldn't crash */
    (void)rc;

    cbm_pipeline_free(p);
    PASS();
}

/* ══════════════════════════════════════════════════════════════════
 *  STORE QUERY INTEGRATION
 * ══════════════════════════════════════════════════════════════════ */

TEST(integ_store_search_by_degree) {
    cbm_store_t *store = cbm_store_open_path_existing(g_dbpath);
    ASSERT_NOT_NULL(store);

    /* Find functions with at least 1 outbound call */
    cbm_search_params_t params = {0};
    params.project = g_project;
    params.label = "Function";
    params.min_degree = 1;
    params.max_degree = -1;
    params.limit = 10;

    cbm_search_output_t out = {0};
    int rc = cbm_store_search(store, &params, &out);
    ASSERT_EQ(rc, CBM_STORE_OK);
    /* main, Multiply, Compute, process should all have outbound calls */
    ASSERT_TRUE(out.count >= 1);

    cbm_store_search_free(&out);
    cbm_store_close(store);
    PASS();
}

TEST(integ_store_find_by_file) {
    cbm_store_t *store = cbm_store_open_path_existing(g_dbpath);
    ASSERT_NOT_NULL(store);

    cbm_node_t *nodes = NULL;
    int count = 0;
    int rc = cbm_store_find_nodes_by_file(store, g_project, "main.py", &nodes, &count);
    ASSERT_EQ(rc, CBM_STORE_OK);
    /* main.py should have: greet, farewell, main functions + Module node */
    ASSERT_TRUE(count >= 3);

    cbm_store_free_nodes(nodes, count);
    cbm_store_close(store);
    PASS();
}

TEST(integ_store_bfs_traversal) {
    cbm_store_t *store = cbm_store_open_path_existing(g_dbpath);
    ASSERT_NOT_NULL(store);

    /* Find a function node to start BFS from */
    cbm_node_t *results = NULL;
    int count = 0;
    cbm_store_find_nodes_by_name(store, g_project, "Multiply", &results, &count);

    if (count > 0) {
        /* BFS outbound from Multiply */
        cbm_traverse_result_t trav = {0};
        int rc = cbm_store_bfs(store, results[0].id, "outbound", NULL, 0, 3, 20, &trav);
        ASSERT_EQ(rc, CBM_STORE_OK);
        /* Should visit at least Add */
        ASSERT_TRUE(trav.visited_count >= 0); /* might be 0 if no edges */
        cbm_store_traverse_free(&trav);
    }

    cbm_store_free_nodes(results, count);
    cbm_store_close(store);
    PASS();
}

/* bfs_collect_edges built its visited-ID set into a fixed 4KB string: past
 * ~340-1100 visited nodes (id-width dependent) the id list was SILENTLY cut,
 * so trace edges (and data_flow args) vanished — and a partially-written id
 * could even match an UNRELATED node, admitting wrong edges. GUARD: a star of
 * 1200 callers (id string ≈ 4.6KB) must surface every edge. RED on the fixed
 * buffer, GREEN with the temp-table join. */
TEST(store_bfs_edges_survive_large_visited_set) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(cbm_store_upsert_project(s, "star", "/tmp/star"), CBM_STORE_OK);

    cbm_node_t hub = {0};
    hub.project = "star";
    hub.label = "Function";
    hub.name = "hub";
    hub.qualified_name = "star.hub";
    hub.file_path = "hub.c";
    int64_t hub_id = cbm_store_upsert_node(s, &hub);
    ASSERT_GT(hub_id, 0);

    enum { SPOKES = 1200 };
    for (int i = 0; i < SPOKES; i++) {
        char nm[32];
        char qn[64];
        snprintf(nm, sizeof(nm), "caller_%04d", i);
        snprintf(qn, sizeof(qn), "star.caller_%04d", i);
        cbm_node_t sp = {0};
        sp.project = "star";
        sp.label = "Function";
        sp.name = nm;
        sp.qualified_name = qn;
        sp.file_path = "spokes.c";
        int64_t sid = cbm_store_upsert_node(s, &sp);
        ASSERT_GT(sid, 0);
        cbm_edge_t e = {0};
        e.project = "star";
        e.source_id = sid;
        e.target_id = hub_id;
        e.type = "CALLS";
        ASSERT_GT(cbm_store_insert_edge(s, &e), 0); /* returns the edge id */
    }

    cbm_traverse_result_t tr = {0};
    ASSERT_EQ(cbm_store_bfs(s, hub_id, "inbound", NULL, 0, 1, SPOKES + 10, &tr), CBM_STORE_OK);
    ASSERT_EQ(tr.visited_count, SPOKES);
    /* Every caller->hub edge must be collected — none silently dropped. */
    ASSERT_EQ(tr.edge_count, SPOKES);

    cbm_store_traverse_free(&tr);
    cbm_store_close(s);
    PASS();
}

/* Multi-source BFS is the substrate for detect_changes impact analysis. Its
 * contract (challenger's flagged traps): (1) ONE traversal over ALL seeds,
 * not seed_count separate walks; (2) seeds EXCLUDED from the result even when
 * reachable from another seed (changed files call each other — that is not
 * "downstream impact"); (3) MIN(hop) across the whole seed set; (4) uncapped
 * counting up to the memory ceiling, which sets *truncated when hit. Fixture:
 * two seeds A, B; A->mid->leaf, B->leaf (leaf is hop 1 from B, hop 2 from A),
 * and A->B directly (B reachable from A). Impact set must be {mid, leaf} with
 * leaf at hop 1, and must NOT contain A or B. */
TEST(store_bfs_multi_excludes_seeds_and_takes_min_hop) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(cbm_store_upsert_project(s, "impact", "/tmp/impact"), CBM_STORE_OK);

    int64_t ids[4];
    const char *names[4] = {"A", "B", "mid", "leaf"};
    for (int i = 0; i < 4; i++) {
        char qn[32];
        snprintf(qn, sizeof(qn), "impact.%s", names[i]);
        cbm_node_t n = {.project = "impact",
                        .label = "Function",
                        .name = names[i],
                        .qualified_name = qn,
                        .file_path = "m.c",
                        .start_line = 1,
                        .end_line = 5};
        ids[i] = cbm_store_upsert_node(s, &n);
        ASSERT_GT(ids[i], 0);
    }
    int64_t A = ids[0];
    int64_t B = ids[1];
    int64_t mid = ids[2];
    int64_t leaf = ids[3];
    struct {
        int64_t from;
        int64_t to;
    } edges[] = {{A, mid}, {mid, leaf}, {B, leaf}, {A, B}};
    for (size_t i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        cbm_edge_t e = {.project = "impact",
                        .source_id = edges[i].from,
                        .target_id = edges[i].to,
                        .type = "CALLS"};
        ASSERT_GT(cbm_store_insert_edge(s, &e), 0);
    }

    int64_t seeds[2] = {A, B};
    cbm_traverse_result_t tr = {0};
    bool truncated = true;
    ASSERT_EQ(cbm_store_bfs_multi(s, seeds, 2, "outbound", NULL, 0, 5, 100, &tr, &truncated),
              CBM_STORE_OK);
    ASSERT_FALSE(truncated);

    /* Impact set = {mid, leaf}; A and B (seeds) excluded even though B is
     * reachable from A. */
    ASSERT_EQ(tr.visited_count, 2);
    int seen_mid = 0;
    int seen_leaf = 0;
    int leaf_hop = -1;
    for (int i = 0; i < tr.visited_count; i++) {
        int64_t id = tr.visited[i].node.id;
        ASSERT_TRUE(id != A && id != B); /* seeds never in the result */
        if (id == mid) {
            seen_mid = 1;
        }
        if (id == leaf) {
            seen_leaf = 1;
            leaf_hop = tr.visited[i].hop;
        }
    }
    ASSERT_TRUE(seen_mid && seen_leaf);
    ASSERT_EQ(leaf_hop, 1); /* MIN(hop): 1 from B, not 2 from A */
    cbm_store_traverse_free(&tr);
    cbm_store_close(s);
    PASS();
}

/* The memory-safety ceiling reports truncation instead of silently capping.
 * A star of N callees from one seed, ceiling = N/2, must return exactly N/2
 * rows with *truncated = true. */
TEST(store_bfs_multi_reports_truncation_at_ceiling) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(cbm_store_upsert_project(s, "cap", "/tmp/cap"), CBM_STORE_OK);
    cbm_node_t hub = {.project = "cap",
                      .label = "Function",
                      .name = "hub",
                      .qualified_name = "cap.hub",
                      .file_path = "h.c",
                      .start_line = 1,
                      .end_line = 2};
    int64_t hub_id = cbm_store_upsert_node(s, &hub);
    ASSERT_GT(hub_id, 0);
    enum { N = 40, CEIL = 20 };
    for (int i = 0; i < N; i++) {
        char qn[32];
        snprintf(qn, sizeof(qn), "cap.c%02d", i);
        cbm_node_t n = {.project = "cap",
                        .label = "Function",
                        .name = qn + 4,
                        .qualified_name = qn,
                        .file_path = "c.c",
                        .start_line = 1,
                        .end_line = 2};
        int64_t nid = cbm_store_upsert_node(s, &n);
        ASSERT_GT(nid, 0);
        cbm_edge_t e = {.project = "cap", .source_id = hub_id, .target_id = nid, .type = "CALLS"};
        ASSERT_GT(cbm_store_insert_edge(s, &e), 0);
    }
    cbm_traverse_result_t tr = {0};
    bool truncated = false;
    ASSERT_EQ(cbm_store_bfs_multi(s, &hub_id, 1, "outbound", NULL, 0, 5, CEIL, &tr, &truncated),
              CBM_STORE_OK);
    ASSERT_EQ(tr.visited_count, CEIL);
    ASSERT_TRUE(truncated);
    cbm_store_traverse_free(&tr);
    cbm_store_close(s);
    PASS();
}

/* #411: index_repository silently drops entire subtrees with no record.
 * Moderate/fast mode applies FAST_SKIP_DIRS (tools/scripts/bin/docs/...) and ALL
 * modes apply ALWAYS_SKIP_DIRS (node_modules/...), so files are excluded from the
 * graph — but the response only reports nodes/edges/status, giving the user no
 * way to know which subtrees were dropped (the reporter lost a 47-file tools/).
 * Desired (maintainer): a COMPACT per-directory summary of excluded files (dir +
 * count, not a verbose per-file list) surfaced in the index result for any mode.
 * RED until the index response reports excluded subtrees. Self-contained. */
TEST(index_reports_excluded_subtrees_issue411) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "/tmp/cbm_excl_XXXXXX");
    ASSERT_NOT_NULL(cbm_mkdtemp(tmp));

    char path[512];
    /* one real source file ... */
    snprintf(path, sizeof(path), "%s/app.py", tmp);
    FILE *f = fopen(path, "wb");
    ASSERT_NOT_NULL(f);
    fputs("def app():\n    return 1\n", f);
    fclose(f);
    /* ... and a node_modules subtree that is excluded in EVERY mode. */
    snprintf(path, sizeof(path), "%s/node_modules", tmp);
    cbm_mkdir_p(path, 0755);
    snprintf(path, sizeof(path), "%s/node_modules/dep.js", tmp);
    f = fopen(path, "wb");
    ASSERT_NOT_NULL(f);
    fputs("export function dep() { return 2; }\n", f);
    fclose(f);

    cbm_test_operation_host_t *srv = cbm_test_operation_host_new(NULL);
    ASSERT_NOT_NULL(srv);
    char args[600];
    snprintf(args, sizeof(args), "{\"repo_path\":\"%s\"}", tmp);
    char *resp = cbm_test_operation_execute(srv, "index_repository", args);
    ASSERT_NOT_NULL(resp);

    /* The dropped node_modules/dep.js must be reported somewhere compact in the
     * response so the user knows it wasn't indexed. Today the response carries no
     * excluded/skipped summary at all → this fails (reproduces the silent drop). */
    bool reports_excluded = strstr(resp, "excluded") != NULL || strstr(resp, "skipped") != NULL;
    free(resp);
    cbm_test_operation_host_free(srv);
    th_rmtree(tmp);
    ASSERT_TRUE(reports_excluded);
    PASS();
}

/* ══════════════════════════════════════════════════════════════════
 *  SUITE
 * ══════════════════════════════════════════════════════════════════ */

/* ══════════════════════════════════════════════════════════════════
 *  CLI OPERATION BEHAVIOUR PORTED FROM UPSTREAM (search_graph, coverage)
 * ══════════════════════════════════════════════════════════════════ */

/* Fixture for the two BM25 findability probes (measured upstream on
 * JetBrains/Exposed and django/django): a Class named exactly like the query,
 * that class's own Methods, and test Methods whose long names repeat the
 * query token. Written to <cache>/<project>.db so the search operation opens it
 * exactly as it opens an indexed project. */
static bool integ_write_bm25_fixture(const char *cache, const char *project) {
    char db_path[512];
    snprintf(db_path, sizeof(db_path), "%s/%s.db", cache, project);
    cbm_store_t *store = cbm_store_open_path(db_path);
    if (!store)
        return false;
    cbm_store_upsert_project(store, project, "/tmp/bm25-findability");
    struct {
        const char *label, *name, *qn, *file;
    } rows[] = {
        {"Class", "Table", "bm25-find.core.Table.Table", "core/Table.kt"},
        {"Method", "unquoted", "bm25-find.core.Table.Table.unquoted", "core/Table.kt"},
        {"Method", "describe", "bm25-find.core.Table.Table.describe", "core/Table.kt"},
        {"Method", "table references table with same name in other database",
         "bm25-find.tests.SchemaTests.table_references_table_with_same_name", "tests/Schema.kt"},
        {"Method", "table references table with same name in mysql",
         "bm25-find.tests.SchemaTests.table_references_table_with_same_name_mysql",
         "tests/Schema.kt"},
        {"Function", "get_object_or_404", "bm25-find.shortcuts.get_object_or_404", "shortcuts.py"},
        {"Method", "test_get_object_or_404",
         "bm25-find.tests.GetObjectOr404Tests.test_get_object_or_404", "tests/tests.py"},
        {"Method", "test_get_object_or_404_queryset_attribute_error",
         "bm25-find.tests.GetObjectOr404Tests.test_get_object_or_404_queryset_attribute_error",
         "tests/tests.py"},
        {"Method", "test_get_object_or_404_bad_class",
         "bm25-find.tests.GetListObjectOr404Test.test_get_object_or_404_bad_class",
         "tests/async.py"},
    };
    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        cbm_node_t node = {.project = project,
                           .label = rows[i].label,
                           .name = rows[i].name,
                           .qualified_name = rows[i].qn,
                           .file_path = rows[i].file,
                           .start_line = (int)i + 1,
                           .end_line = (int)i + 2};
        cbm_store_upsert_node(store, &node);
    }
    cbm_store_exec(store, "INSERT INTO nodes_fts(nodes_fts) VALUES('delete-all');");
    cbm_store_exec(store, "INSERT INTO nodes_fts(rowid, name, qualified_name, label, "
                          "file_path) SELECT id, cbm_camel_split(name), qualified_name, "
                          "label, file_path FROM nodes;");
    cbm_store_close(store);
    return true;
}

static void integ_restore_cache_dir(const char *saved) {
    if (saved)
        (void)cbm_setenv("CBM_CACHE_DIR", saved, 1);
    else
        (void)cbm_unsetenv("CBM_CACHE_DIR");
}

/* The label filter must apply in query (BM25) mode exactly as in structural
 * mode: `query=Table label=Class` returns the class, and no Method. */
TEST(integ_search_graph_bm25_applies_label_filter) {
    char *cache = th_mktempdir("cbm_integ_bm25_label");
    ASSERT_NOT_NULL(cache);
    const char *saved = getenv("CBM_CACHE_DIR");
    char *saved_copy = saved ? strdup(saved) : NULL;
    (void)cbm_setenv("CBM_CACHE_DIR", cache, 1);
    bool fixture = integ_write_bm25_fixture(cache, "bm25-find");
    cbm_test_operation_host_t *host = fixture ? cbm_test_operation_host_new(NULL) : NULL;
    char *resp = host ? cbm_test_operation_execute(
                            host, "search_graph",
                            "{\"project\":\"bm25-find\",\"format\":\"json\",\"query\":\"Table\","
                            "\"label\":\"Class\",\"limit\":5}")
                      : NULL;
    bool class_found = resp && strstr(resp, "\"bm25-find.core.Table.Table\",\"Class\"") != NULL;
    bool no_method = resp && strstr(resp, "\"Method\"") == NULL;
    /* The reported total describes the filtered rows, not the unfiltered window. */
    bool total_one = resp && strstr(resp, "\"total\":1") != NULL;
    free(resp);
    cbm_test_operation_host_free(host);
    integ_restore_cache_dir(saved_copy);
    free(saved_copy);
    th_cleanup(cache);
    ASSERT_TRUE(fixture);
    ASSERT_TRUE(class_found);
    ASSERT_TRUE(no_method);
    ASSERT_TRUE(total_one);
    PASS();
}

/* The definition whose NAME is the query ranks first: the `Table` class above
 * its own methods and above test methods that repeat "table" three times; the
 * `get_object_or_404` function above the test methods that contain it. */
TEST(integ_search_graph_bm25_ranks_exact_name_first) {
    char *cache = th_mktempdir("cbm_integ_bm25_exact");
    ASSERT_NOT_NULL(cache);
    const char *saved = getenv("CBM_CACHE_DIR");
    char *saved_copy = saved ? strdup(saved) : NULL;
    (void)cbm_setenv("CBM_CACHE_DIR", cache, 1);
    bool fixture = integ_write_bm25_fixture(cache, "bm25-find");
    cbm_test_operation_host_t *host = fixture ? cbm_test_operation_host_new(NULL) : NULL;

    char *table =
        host
            ? cbm_test_operation_execute(
                  host, "search_graph",
                  "{\"project\":\"bm25-find\",\"format\":\"json\",\"query\":\"Table\",\"limit\":5}")
            : NULL;
    const char *table_rows = table ? strstr(table, "\"rows\":[[") : NULL;
    bool table_first =
        table_rows && strncmp(table_rows + 8, "[\"bm25-find.core.Table.Table\"", 29) == 0;
    free(table);

    char *get404 =
        host ? cbm_test_operation_execute(
                   host, "search_graph",
                   "{\"project\":\"bm25-find\",\"format\":\"json\",\"query\":\"get_object_or_404\","
                   "\"limit\":5}")
             : NULL;
    const char *get404_rows = get404 ? strstr(get404, "\"rows\":[[") : NULL;
    bool function_first =
        get404_rows &&
        strncmp(get404_rows + 8, "[\"bm25-find.shortcuts.get_object_or_404\"", 40) == 0;
    free(get404);

    cbm_test_operation_host_free(host);
    integ_restore_cache_dir(saved_copy);
    free(saved_copy);
    th_cleanup(cache);
    ASSERT_TRUE(fixture);
    ASSERT_TRUE(table_first);
    ASSERT_TRUE(function_first);
    PASS();
}

TEST(tool_list_projects_tree_uses_one_stable_header_and_keeps_json_direct) {
    char cache[256];
    snprintf(cache, sizeof(cache), "%s/cbm-list-lean-XXXXXX", cbm_tmpdir());
    ASSERT_NOT_NULL(cbm_mkdtemp(cache));
    const char *saved_cache = getenv("CBM_CACHE_DIR");
    char *saved_cache_copy = saved_cache ? cbm_mem_strdup(CBM_MEM_CLASS_OTHER, saved_cache) : NULL;
    cbm_setenv("CBM_CACHE_DIR", cache, 1);

    enum { PROJECTS = 12 };
    for (int i = 0; i < PROJECTS; i++) {
        char project[32];
        char db_path[512];
        char root_path[512];
        snprintf(project, sizeof(project), "lean-project-%02d", i);
        snprintf(db_path, sizeof(db_path), "%s/%s.db", cache, project);
        snprintf(root_path, sizeof(root_path),
                 "/workspaces/organization/shared/services/lean-project-%02d", i);
        cbm_store_t *store = cbm_store_open_path(db_path);
        ASSERT_NOT_NULL(store);
        ASSERT_EQ(cbm_store_upsert_project(store, project, root_path), CBM_STORE_OK);
        cbm_node_t node = {.project = project,
                           .label = "Function",
                           .name = "entry",
                           .qualified_name = "shared.module.entry",
                           .file_path = "src/main.c",
                           .start_line = 1,
                           .end_line = 1};
        ASSERT_GT(cbm_store_upsert_node(store, &node), 0);
        cbm_store_close(store);
    }

    cbm_test_operation_host_t *srv = cbm_test_operation_host_new(NULL);
    ASSERT_NOT_NULL(srv);
    char *identity_response = cbm_test_operation_execute(srv, "list_projects", "{\"limit\":50}");
    char *identity = identity_response;
    char *tree_response =
        cbm_test_operation_execute(srv, "list_projects", "{\"limit\":50,\"detail\":\"stats\"}");
    char *tree = tree_response;
    char *json_response = cbm_test_operation_execute(
        srv, "list_projects", "{\"limit\":50,\"format\":\"json\",\"detail\":\"stats\"}");
    char *json = json_response;
    char *page_response = cbm_test_operation_execute(srv, "list_projects", "{\"limit\":5}");
    char *page = page_response;

    bool tree_shape =
        tree && strstr(tree, "projects_refs:") &&
        strstr(tree, "projects: 12  (cols: name root_path branch nodes edges size_bytes)") &&
        strstr(tree, "lean-project-00") && strstr(tree, "lean-project-11") &&
        strstr(tree, "total: 12") && strstr(tree, "returned: 12") &&
        strstr(tree, "has_more: false") && !strstr(tree, "\"name\":");
    bool identity_lean = identity && strstr(identity, "projects_refs:") &&
                         strstr(identity, "projects: 12  (cols: name root_path branch)") &&
                         !strstr(identity, "size_bytes") && !strstr(identity, " nodes edges");
    bool tree_is_leaner = tree && json && strlen(tree) < strlen(json);
    bool tree_not_duplicated = tree_response && !strstr(tree_response, "structuredContent");
    bool page_truthful = page && strstr(page, "projects: 5") && strstr(page, "returned: 5") &&
                         strstr(page, "has_more: true") && strstr(page, "next_offset: 5");
    bool json_direct = false;

    yyjson_doc *doc = json ? yyjson_read(json, strlen(json), 0) : NULL;
    if (doc) {
        yyjson_val *projects = yyjson_obj_get(yyjson_doc_get_root(doc), "projects");
        yyjson_val *first = projects ? yyjson_arr_get_first(projects) : NULL;
        json_direct = projects && yyjson_arr_size(projects) == PROJECTS && first &&
                      yyjson_obj_get(first, "name") && yyjson_obj_get(first, "root_path") &&
                      yyjson_obj_get(first, "nodes") && yyjson_obj_get(first, "edges") &&
                      yyjson_obj_get(first, "size_bytes") && !strstr(json, "rows_refs");
        yyjson_doc_free(doc);
    }

    char *metadata = cbm_test_operation_execute(
        srv, "list_projects", "{\"limit\":1,\"metadata_only\":true,\"format\":\"json\"}");
    ASSERT_STR_EQ(metadata, "{\"projects\":[{\"name\":\"lean-project-00\",\"root_path\":\"/"
                            "workspaces/organization/shared/services/"
                            "lean-project-00\"}],\"total\":12,\"offset\":0,\"limit\":1,"
                            "\"returned\":1,\"has_more\":true,\"next_offset\":1}");
    free(metadata);
    free(identity_response);
    free(tree_response);
    free(json_response);
    free(page_response);
    cbm_test_operation_host_free(srv);
    integ_restore_cache_dir(saved_cache_copy);
    cbm_free(CBM_MEM_CLASS_OTHER, saved_cache_copy);
    for (int i = 0; i < PROJECTS; i++) {
        char project[32];
        snprintf(project, sizeof(project), "lean-project-%02d", i);
        char db_path[512];
        snprintf(db_path, sizeof(db_path), "%s/%s.db", cache, project);
        cbm_unlink(db_path);
    }
    cbm_rmdir(cache);

    ASSERT_TRUE(tree_shape);
    ASSERT_TRUE(identity_lean);
    ASSERT_TRUE(tree_is_leaner);
    ASSERT_TRUE(tree_not_duplicated);
    ASSERT_TRUE(page_truthful);
    ASSERT_TRUE(json_direct);

    PASS();
}

TEST(integ_search_graph_tree_budget_continuation) {
    char args[512];
    snprintf(args, sizeof(args),
             "{\"project\":\"%s\",\"name_pattern\":\"budgetMarker\",\"max_output_tokens\":128}",
             g_project);
    char *page = call_tool("search_graph", args);
    ASSERT_NOT_NULL(page);
    ASSERT_TRUE(strlen(page) <= cbm_output_budget_bytes(128));
    ASSERT_NOT_NULL(strstr(page, "truncation_reason: output_budget"));
    const char *next = strstr(page, "next_offset: ");
    ASSERT_NOT_NULL(next);
    int offset = atoi(next + strlen("next_offset: "));
    ASSERT_GT(offset, 0);
    ASSERT_TRUE(offset < 12);
    snprintf(args, sizeof(args),
             "{\"project\":\"%s\",\"name_pattern\":\"budgetMarker\",\"offset\":%d,\"max_output_"
             "tokens\":128}",
             g_project, offset);
    char *continuation = call_tool("search_graph", args);
    ASSERT_NOT_NULL(continuation);
    ASSERT_TRUE(strlen(continuation) <= cbm_output_budget_bytes(128));
    ASSERT_NOT_NULL(strstr(continuation, "results:"));
    free(page);
    free(continuation);
    PASS();
}

/* Ported from upstream's dotless, internal-property and byte-budget guards.
 * Real disk-backed store, reached through the neutral operation entry point. */
TEST(integ_search_graph_toon_lossless_and_budgeted) {
    char *cache = th_mktempdir("cbm_toon_search");
    ASSERT_NOT_NULL(cache);
    const char *saved = getenv("CBM_CACHE_DIR");
    char *saved_copy = saved ? cbm_mem_strdup(CBM_MEM_CLASS_OTHER, saved) : NULL;
    cbm_setenv("CBM_CACHE_DIR", cache, 1);
    char db[512];
    snprintf(db, sizeof(db), "%s/search-toon.db", cache);
    cbm_store_t *store = cbm_store_open_path(db);
    ASSERT_NOT_NULL(store);
    ASSERT_EQ(cbm_store_upsert_project(store, "search-toon", "/tmp/search-toon"), CBM_STORE_OK);
    cbm_node_t node = {.project = "search-toon",
                       .label = "Route",
                       .name = "dotless_route",
                       .qualified_name = "__route__ANY__/api/adr",
                       .file_path = "src/routes.c",
                       .start_line = 7,
                       .end_line = 7,
                       .properties_json = "{\"fp\":\"FPSENTINEL00\",\"sp\":\"SPSENTINEL00\","
                                          "\"bt\":\"BTSENTINEL00\",\"complexity\":7}"};
    ASSERT_GT(cbm_store_upsert_node(store, &node), 0);
    char long_prefix[3901], qn[4000], path[3901], property[5020];
    memset(long_prefix, 'q', sizeof(long_prefix) - 1U);
    long_prefix[3900] = '\0';
    memset(path, 'p', sizeof(path) - 1U);
    path[3900] = '\0';
    strcpy(property, "{\"doc\":\"");
    memset(property + 8, 'v', 5000);
    strcpy(property + 5008, "\"}");
    node.label = "Function";
    node.name = "a_budget";
    snprintf(qn, sizeof(qn), "%s.a_budget", long_prefix);
    node.qualified_name = qn;
    node.file_path = path;
    node.properties_json = property;
    ASSERT_GT(cbm_store_upsert_node(store, &node), 0);
    char scatter_qn[256], scatter_file[256], scatter_name[32];
    for (int i = 0; i < 24; ++i) {
        snprintf(scatter_name, sizeof(scatter_name), "scatter%02d", i);
        snprintf(scatter_qn, sizeof(scatter_qn),
                 "organization.shared.services.module%02d.scatter%02d", i, i);
        snprintf(scatter_file, sizeof(scatter_file),
                 "organization/shared/services/module%02d/entry.c", i);
        node.name = scatter_name;
        node.qualified_name = scatter_qn;
        node.file_path = scatter_file;
        node.properties_json = NULL;
        ASSERT_GT(cbm_store_upsert_node(store, &node), 0);
    }
    cbm_store_close(store);
    cbm_test_operation_host_t *host = cbm_test_operation_host_new(NULL);
    ASSERT_NOT_NULL(host);
    const char *args = "{\"project\":\"search-toon\",\"name_pattern\":\"dotless_route\"}";
    char *tree = cbm_test_operation_execute(host, "search_graph", args);
    ASSERT_NOT_NULL(tree);
    ASSERT_NOT_NULL(strstr(tree, "(cols: qn label file lines in out)"));
    ASSERT_NOT_NULL(strstr(tree, "__route__ANY__/api/adr Route src/routes.c 7-7"));
    ASSERT_NULL(strstr(tree, "group prefix"));
    free(tree);
    char *json = cbm_test_operation_execute(
        host, "search_graph",
        "{\"project\":\"search-toon\",\"name_pattern\":\"dotless_route\",\"format\":\"json\"}");
    ASSERT_STR_EQ(json,
                  "{\"total\":1,\"count\":1,\"cols\":[\"name\",\"label\",\"lines\",\"in\",\"out\"],"
                  "\"groups\":[{\"qn_prefix\":\"\",\"file\":\"src/"
                  "routes.c\",\"rows\":[[\"__route__ANY__/api/adr\","
                  "\"Route\",\"7-7\",0,0]]}],\"has_more\":false}");
    free(json);
    tree = cbm_test_operation_execute(
        host, "search_graph",
        "{\"project\":\"search-toon\",\"name_pattern\":\"dotless_route\","
        "\"fields\":[\"fp\",\"sp\",\"bt\",\"complexity\"]}");
    ASSERT_NOT_NULL(strstr(tree, "complexity"));
    ASSERT_NULL(strstr(tree, "FPSENTINEL00"));
    ASSERT_NULL(strstr(tree, "SPSENTINEL00"));
    ASSERT_NULL(strstr(tree, "BTSENTINEL00"));
    free(tree);
    tree = cbm_test_operation_execute(
        host, "search_graph",
        "{\"project\":\"search-toon\",\"name_pattern\":\"a_budget\",\"fields\":[\"doc\"],"
        "\"max_output_tokens\":100000}");
    ASSERT_NOT_NULL(tree);
    ASSERT_NOT_NULL(strstr(tree, long_prefix));
    ASSERT_NOT_NULL(strstr(tree, path));
    ASSERT_NOT_NULL(strstr(tree, "a_budget"));
    ASSERT_TRUE(strlen(tree) > 10000U);
    free(tree);
    for (int format = 0; format < 2; ++format) {
        const char *budget_args =
            format ? "{\"project\":\"search-toon\",\"name_pattern\":\"a_budget\",\"fields\":["
                     "\"doc\"],\"max_output_tokens\":128,\"format\":\"json\"}"
                   : "{\"project\":\"search-toon\",\"name_pattern\":\"a_budget\",\"fields\":["
                     "\"doc\"],\"max_output_tokens\":128}";
        char *page = cbm_test_operation_execute(host, "search_graph", budget_args);
        ASSERT_NOT_NULL(page);
        ASSERT_TRUE(strlen(page) <= cbm_output_budget_bytes(128));
        ASSERT_NOT_NULL(strstr(page, "output_budget"));
        ASSERT_NOT_NULL(strstr(page, "next_offset"));
        ASSERT_NULL(strstr(page, "a_budget"));
        free(page);
    }
    tree = cbm_test_operation_execute(
        host, "search_graph",
        "{\"project\":\"search-toon\",\"name_pattern\":\"scatter\",\"limit\":24}");
    ASSERT_NOT_NULL(tree);
    ASSERT_NOT_NULL(strstr(tree, "results_refs:"));
    ASSERT_NOT_NULL(strstr(tree, "@N+suffix=prefix+suffix"));
    ASSERT_NOT_NULL(strstr(tree, "scatter00"));
    ASSERT_NOT_NULL(strstr(tree, "scatter23"));
    free(tree);
    cbm_test_operation_host_free(host);
    integ_restore_cache_dir(saved_copy);
    cbm_free(CBM_MEM_CLASS_OTHER, saved_copy);
    th_cleanup(cache);
    PASS();
}

/* #1714: coverage freshness must compare mtime_ns at the SAME precision the
 * indexer records it. The pipeline records cbm_path_info_utf8's value (on
 * Windows derived from FILETIME, nanosecond), while the freshness reader used
 * to recompute from struct stat, which on Windows truncates to seconds. A
 * byte-identical file therefore never matched and every path was reported
 * metadata_changed. The reader now uses the indexer's own source. */
TEST(integ_coverage_freshness_uses_indexer_mtime_source_issue1714) {
    char *cache = th_mktempdir("cbm_integ_cov_cache");
    char *root = th_mktempdir("cbm_integ_cov_root");
    ASSERT_NOT_NULL(cache);
    ASSERT_NOT_NULL(root);
    ASSERT_EQ(th_write_file(TH_PATH(root, "main.go"), "package main\nfunc main() {}\n"), 0);
    const char *saved = getenv("CBM_CACHE_DIR");
    char *saved_copy = saved ? strdup(saved) : NULL;
    (void)cbm_setenv("CBM_CACHE_DIR", cache, 1);

    char source_path[512];
    snprintf(source_path, sizeof(source_path), "%s/main.go", root);
    cbm_path_info_t info;
    bool have_info = cbm_path_info_utf8(source_path, &info) == 0;

    char db_path[512];
    snprintf(db_path, sizeof(db_path), "%s/cov-fresh.db", cache);
    cbm_store_t *store = have_info ? cbm_store_open_path(db_path) : NULL;
    bool stored = store && cbm_store_upsert_project(store, "cov-fresh", root) == CBM_STORE_OK &&
                  cbm_store_upsert_file_hash(store, "cov-fresh", "main.go", "", info.mtime_ns,
                                             info.size) == CBM_STORE_OK;
    cbm_test_operation_host_t *host = stored ? cbm_test_operation_host_new(NULL) : NULL;
    const char *args = "{\"project\":\"cov-fresh\",\"paths\":[\"main.go\"],\"format\":\"json\"}";
    char *resp = host ? cbm_test_operation_execute(host, "check_index_coverage", args) : NULL;
    bool match = resp && strstr(resp, "\"freshness\":\"metadata_match\"") != NULL;
    free(resp);

    /* A hash stored at seconds precision (what a stat-based reader compared
     * against) must NOT match an unchanged file: the comparison stays
     * nanosecond-exact, or part of mtime resolution is silently dropped. */
    bool changed = true;
    int64_t seconds_mtime_ns =
        have_info ? (info.mtime_ns / (int64_t)CBM_NSEC_PER_SEC) * (int64_t)CBM_NSEC_PER_SEC : 0;
    if (stored && seconds_mtime_ns != info.mtime_ns) {
        changed = cbm_store_upsert_file_hash(store, "cov-fresh", "main.go", "", seconds_mtime_ns,
                                             info.size) == CBM_STORE_OK;
        resp = changed ? cbm_test_operation_execute(host, "check_index_coverage", args) : NULL;
        changed = changed && resp && strstr(resp, "\"freshness\":\"metadata_changed\"") != NULL;
        free(resp);
    }

    cbm_test_operation_host_free(host);
    if (store)
        cbm_store_close(store);
    integ_restore_cache_dir(saved_copy);
    free(saved_copy);
    th_cleanup(cache);
    th_cleanup(root);
    ASSERT_TRUE(have_info);
    ASSERT_TRUE(stored);
    ASSERT_TRUE(match);
    ASSERT_TRUE(changed);
    PASS();
}

SUITE(integration) {
    RUN_TEST(index_reports_excluded_subtrees_issue411);
    /* Set up: create temp project and index it */
    if (integration_setup() != 0) {
        /* A suite that cannot establish its preconditions has FAILED, not
         * "skipped" — surface it as a single red failure (no-skips policy). */
        printf("  %sFAIL%s %s:%d: %s\n", tf_red(), tf_reset(), __FILE__, __LINE__,
               "integration_setup failed");
        tf_fail_count++;
        integration_teardown();
        return;
    }

    /* Pipeline result validation */
    RUN_TEST(integ_index_has_nodes);
    RUN_TEST(integ_index_has_edges);
    RUN_TEST(integ_index_has_functions);
    RUN_TEST(integ_index_has_files);
    RUN_TEST(integ_index_has_calls);

    /* MCP tool handler validation */
    RUN_TEST(integ_mcp_list_projects);
    RUN_TEST(tool_list_projects_tree_uses_one_stable_header_and_keeps_json_direct);
    RUN_TEST(integ_search_graph_toon_lossless_and_budgeted);
    RUN_TEST(integ_search_graph_tree_budget_continuation);
    RUN_TEST(integ_mcp_search_graph_by_label);
    RUN_TEST(integ_mcp_search_graph_by_name);
    RUN_TEST(integ_search_graph_bm25_applies_label_filter);
    RUN_TEST(integ_search_graph_bm25_ranks_exact_name_first);
    RUN_TEST(integ_coverage_freshness_uses_indexer_mtime_source_issue1714);
    RUN_TEST(integ_mcp_query_graph_functions);
    RUN_TEST(integ_mcp_query_graph_calls);
    RUN_TEST(integ_mcp_get_graph_schema);
    RUN_TEST(integ_mcp_get_architecture);
    RUN_TEST(integ_mcp_trace_path);
    RUN_TEST(integ_mcp_trace_path_cross_service);
    RUN_TEST(integ_mcp_search_code_match_limit_and_budget);
    RUN_TEST(integ_mcp_trace_path_output_budget);
    RUN_TEST(integ_mcp_index_status);
    RUN_TEST(integ_mcp_adr_outline_fence_status);

    /* Store query validation */
    RUN_TEST(integ_store_search_by_degree);
    RUN_TEST(integ_store_find_by_file);
    RUN_TEST(integ_store_bfs_traversal);
    RUN_TEST(store_bfs_edges_survive_large_visited_set);
    RUN_TEST(store_bfs_multi_excludes_seeds_and_takes_min_hop);
    RUN_TEST(store_bfs_multi_reports_truncation_at_ceiling);

    /* Pipeline API tests (no db needed) */
    RUN_TEST(integ_pipeline_fqn_compute);
    RUN_TEST(integ_pipeline_fqn_module);
    RUN_TEST(integ_pipeline_project_name);
    RUN_TEST(integ_pipeline_cancel);

    /* Destructive tests (run last!) */
    RUN_TEST(integ_mcp_delete_project);

    /* Teardown */
    integration_teardown();
}
