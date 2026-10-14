/*
 * test_changes_operation.c — the `changes` operation (detect_changes) on small
 * real git repositories: seed listing, project test configuration, the opt-in
 * impact walk (edge_types / fixpoint / via) and scope:"tests".
 *
 * One fixture below: `leaf` is the changed function; a call chain four deep
 * sits on top of it, and `holder` only names it. The repository is indexed
 * through the real index_repository operation.
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "test_operation_host.h"

#include "foundation/compat.h"
#include "foundation/platform.h"
#include "pipeline/pipeline.h"
#include "store/store.h"
#include "yyjson/yyjson.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char repo[512];
    cbm_test_operation_host_t *srv;
    char *project;
} cwo_fixture_t;

#define CWO_SRC_HEAD "def leaf():\n"
#define CWO_SRC_TAIL      \
    "    return x\n"      \
    "\n"                  \
    "\n"                  \
    "def mid1():\n"       \
    "    return leaf()\n" \
    "\n"                  \
    "\n"                  \
    "def mid2():\n"       \
    "    return mid1()\n" \
    "\n"                  \
    "\n"                  \
    "def mid3():\n"       \
    "    return mid2()\n" \
    "\n"                  \
    "\n"                  \
    "def top():\n"        \
    "    return mid3()\n" \
    "\n"                  \
    "\n"                  \
    "def holder():\n"     \
    "    f = leaf\n"      \
    "    return f\n"

static bool cwo_fixture_open(cwo_fixture_t *fx) {
    memset(fx, 0, sizeof(*fx));
    snprintf(fx->repo, sizeof(fx->repo), "%s/cbm-changes-op-XXXXXX", cbm_tmpdir());
    if (!cbm_mkdtemp(fx->repo)) {
        return false;
    }
    char src[600];
    snprintf(src, sizeof(src), "%s/mod.py", fx->repo);
    if (th_write_file(src, CWO_SRC_HEAD "    x = 1\n" CWO_SRC_TAIL) != 0) {
        return false;
    }
    char cmd[1200];
    const char *steps[] = {"init -q", "add -A", "commit -q -m init"};
    for (size_t s = 0; s < sizeof(steps) / sizeof(steps[0]); s++) {
        snprintf(cmd, sizeof(cmd),
                 "git -C \"%s\" -c user.name=t -c user.email=t@t.io -c init.defaultBranch=main "
                 "-c commit.gpgsign=false %s",
                 fx->repo, steps[s]);
        if (system(cmd) != 0) {
            return false;
        }
    }
    fx->srv = cbm_test_operation_host_new(NULL);
    if (!fx->srv) {
        return false;
    }
    char idx_args[700];
    snprintf(idx_args, sizeof(idx_args), "{\"repo_path\":\"%s\",\"mode\":\"full\"}", fx->repo);
    free(cbm_test_operation_execute(fx->srv, "index_repository", idx_args));
    /* The change: one line inside leaf(), line count unchanged. */
    if (th_write_file(src, CWO_SRC_HEAD "    x = 11\n" CWO_SRC_TAIL) != 0) {
        return false;
    }
    fx->project = cbm_project_name_from_path(fx->repo);
    return fx->project != NULL;
}

static void cwo_fixture_close(cwo_fixture_t *fx) {
    free(fx->project);
    if (fx->srv) {
        cbm_test_operation_host_free(fx->srv);
    }
    if (fx->repo[0]) {
        th_rmtree(fx->repo);
    }
}

/* changes on the fixture; `extra` is appended to the argument object (start it
 * with a comma). Returns the operation payload. */
static char *cwo_call(cwo_fixture_t *fx, const char *extra) {
    char args[900];
    snprintf(args, sizeof(args), "{\"project\":\"%s\"%s}", fx->project, extra ? extra : "");
    return cbm_test_operation_execute(fx->srv, "detect_changes", args);
}

/* The impacted row whose qualified name ends in `.name`, from a format:"json"
 * answer. NULL when the row is absent. */
static yyjson_val *cwo_row(yyjson_doc *doc, const char *name) {
    yyjson_val *rows = doc ? yyjson_obj_get(yyjson_doc_get_root(doc), "impacted") : NULL;
    size_t want = strlen(name);
    size_t idx;
    size_t max;
    yyjson_val *row;
    yyjson_arr_foreach(rows, idx, max, row) {
        const char *qn = yyjson_get_str(yyjson_obj_get(row, "qn"));
        size_t len = qn ? strlen(qn) : 0;
        if (len > want && qn[len - want - 1U] == '.' && strcmp(qn + len - want, name) == 0) {
            return row;
        }
    }
    return NULL;
}

static int cwo_total(yyjson_doc *doc) {
    return doc ? (int)yyjson_get_int(yyjson_obj_get(yyjson_doc_get_root(doc), "impacted_total"))
               : -1;
}

/* Seed rows describe the actual walk inputs and page independently. */
TEST(changes_seed_list_reports_hunk_without_walk) {
    cwo_fixture_t fx;
    ASSERT_TRUE(cwo_fixture_open(&fx));
    char *text = cwo_call(&fx, ",\"format\":\"json\",\"scope\":\"files\",\"seeds\":\"list\"");
    ASSERT_NOT_NULL(text);
    yyjson_doc *doc = yyjson_read(text, strlen(text), 0);
    ASSERT_NOT_NULL(doc);
    yyjson_val *root = yyjson_doc_get_root(doc);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(root, "seed_total")), 1);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(root, "seed_symbols")), 1);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(root, "seed_returned")), 1);
    ASSERT_EQ(cwo_total(doc), 0);
    yyjson_val *row = yyjson_arr_get_first(yyjson_obj_get(root, "seeds"));
    ASSERT_NOT_NULL(row);
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(row, "seed_reason")), "hunk");
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(row, "file")), "mod.py");
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(row, "label")), "Function");
    ASSERT_NOT_NULL(strstr(yyjson_get_str(yyjson_obj_get(row, "qn")), ".leaf"));
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(row, "start")), 1);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(row, "end")), 3);
    yyjson_doc_free(doc);
    free(text);
    /* Spelling the defaults out changes not one byte of either encoding. */
    const char *formats[] = {"json", "tree"};
    for (int i = 0; i < 2; i++) {
        char extra[200];
        snprintf(extra, sizeof(extra), ",\"format\":\"%s\"", formats[i]);
        char *normal = cwo_call(&fx, extra);
        snprintf(extra, sizeof(extra),
                 ",\"format\":\"%s\",\"seeds\":\"none\",\"seed_filter\":\"all\"", formats[i]);
        char *explicit_defaults = cwo_call(&fx, extra);
        ASSERT_NOT_NULL(normal);
        ASSERT_STR_EQ(normal, explicit_defaults);
        free(normal);
        free(explicit_defaults);
    }
    cwo_fixture_close(&fx);
    PASS();
}

TEST(changes_seed_pages_bind_semantics_and_snapshot) {
    cwo_fixture_t fx;
    ASSERT_TRUE(cwo_fixture_open(&fx));
    char path[600];
    snprintf(path, sizeof(path), "%s/mod.py", fx.repo);
    /* No definition overlaps this insertion: whole-file fallback. */
    ASSERT_EQ(th_write_file(path, "import os\n" CWO_SRC_HEAD "    x = 1\n" CWO_SRC_TAIL), 0);
    /* Move indexed definitions past the import so this is truly zero overlap. */
    cbm_store_t *store = cbm_store_open(fx.project);
    ASSERT_NOT_NULL(store);
    cbm_node_t *nodes = NULL;
    int count = 0;
    ASSERT_EQ(cbm_store_find_nodes_by_file(store, fx.project, "mod.py", &nodes, &count),
              CBM_STORE_OK);
    for (int i = 0; i < count; i++) {
        nodes[i].start_line += 1;
        nodes[i].end_line += 1;
        ASSERT_GT(cbm_store_upsert_node(store, &nodes[i]), 0);
    }
    cbm_store_free_nodes(nodes, count);
    cbm_store_close(store);
    const char *base = ",\"format\":\"json\",\"scope\":\"files\",\"seeds\":\"list\"";
    char extra[500];
    snprintf(extra, sizeof(extra), "%s,\"seed_limit\":2", base);
    char *first = cwo_call(&fx, extra);
    ASSERT_NOT_NULL(first);
    yyjson_doc *doc = yyjson_read(first, strlen(first), 0);
    ASSERT_NOT_NULL(doc);
    yyjson_val *root = yyjson_doc_get_root(doc);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(root, "seed_total")), 6);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(root, "seed_returned")), 2);
    ASSERT_TRUE(yyjson_get_bool(yyjson_obj_get(root, "seed_has_more")));
    const char *cursor = yyjson_get_str(yyjson_obj_get(root, "seed_next_cursor"));
    ASSERT_NOT_NULL(cursor);
    yyjson_val *rows = yyjson_obj_get(root, "seeds");
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(yyjson_arr_get_first(rows), "seed_reason")),
                  "whole_file");
    const char *last = yyjson_get_str(yyjson_obj_get(yyjson_arr_get(rows, 1), "qn"));
    snprintf(extra, sizeof(extra), "%s,\"seed_limit\":3,\"seed_cursor\":\"%s\"", base, cursor);
    char *next = cwo_call(&fx, extra);
    ASSERT_NOT_NULL(next);
    yyjson_doc *ndoc = yyjson_read(next, strlen(next), 0);
    ASSERT_NOT_NULL(ndoc);
    yyjson_val *nroot = yyjson_doc_get_root(ndoc);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(nroot, "seed_returned")), 3);
    const char *next_qn =
        yyjson_get_str(yyjson_obj_get(yyjson_arr_get_first(yyjson_obj_get(nroot, "seeds")), "qn"));
    ASSERT_LT(strcmp(last, next_qn), 0);
    snprintf(extra, sizeof(extra), "%s,\"seed_limit\":3,\"seed_offset\":2", base);
    char *offset = cwo_call(&fx, extra);
    ASSERT_STR_EQ(next, offset);
    free(offset);
    yyjson_doc_free(ndoc);
    free(next);
    const char *mismatches[] = {",\"seed_filter\":\"product\"", ",\"seed_offset\":1",
                                ",\"depth\":3"};
    for (int i = 0; i < 3; i++) {
        snprintf(extra, sizeof(extra), "%s%s,\"seed_cursor\":\"%s\"", base, mismatches[i], cursor);
        char *bad = cwo_call(&fx, extra);
        ASSERT_NOT_NULL(bad);
        ASSERT_NOT_NULL(strstr(bad, "cursor_params_mismatch"));
        free(bad);
    }
    snprintf(extra, sizeof(extra), "%s,\"seed_limit\":0", base);
    char *zero = cwo_call(&fx, extra);
    ASSERT_NOT_NULL(zero);
    ASSERT_NOT_NULL(strstr(zero, "seed_continuation_requires_positive_limit"));
    free(zero);

    int token_budget = (int)((strlen(first) + 3) / 4);
    snprintf(extra, sizeof(extra), "%s,\"max_output_tokens\":%d", base, token_budget);
    char *budget = cwo_call(&fx, extra);
    ASSERT_NOT_NULL(budget);
    ASSERT_LTE(strlen(budget), (size_t)token_budget * 4);
    yyjson_doc *bdoc = yyjson_read(budget, strlen(budget), 0);
    ASSERT_NOT_NULL(bdoc);
    yyjson_val *broot = yyjson_doc_get_root(bdoc);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(broot, "seed_total")), 6);
    int returned = (int)yyjson_get_int(yyjson_obj_get(broot, "seed_returned"));
    ASSERT_LT(returned, 6);
    ASSERT_TRUE(yyjson_get_bool(yyjson_obj_get(broot, "seed_has_more")));
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(broot, "truncation_reason")), "output_budget");
    ASSERT_NOT_NULL(yyjson_obj_get(broot, returned ? "seed_next_cursor"
                                                   : "seed_continuation_requires_higher_budget"));
    yyjson_doc_free(bdoc);
    free(budget);
    char *tree = cwo_call(
        &fx, ",\"format\":\"tree\",\"scope\":\"files\",\"seeds\":\"list\",\"seed_limit\":1");
    ASSERT_NOT_NULL(tree);
    ASSERT_NOT_NULL(strstr(tree, "seed_total: 6"));
    ASSERT_NOT_NULL(strstr(tree, "seed_returned: 1"));
    ASSERT_NOT_NULL(strstr(tree, "seed_next_cursor:"));
    ASSERT_NOT_NULL(strstr(tree, "whole_file"));
    free(tree);
    /* The file bytes changed since the cursor was minted. */
    ASSERT_EQ(th_write_file(path, "import sys\n" CWO_SRC_HEAD "    x = 1\n" CWO_SRC_TAIL), 0);
    snprintf(extra, sizeof(extra), "%s,\"seed_cursor\":\"%s\"", base, cursor);
    char *stale = cwo_call(&fx, extra);
    ASSERT_NOT_NULL(stale);
    ASSERT_NOT_NULL(strstr(stale, "snapshot_changed"));
    free(stale);
    yyjson_doc_free(doc);
    free(first);
    cwo_fixture_close(&fx);
    PASS();
}

TEST(changes_product_filter_does_not_inflate_fallback) {
    cwo_fixture_t fx;
    ASSERT_TRUE(cwo_fixture_open(&fx));
    cbm_store_t *store = cbm_store_open(fx.project);
    ASSERT_NOT_NULL(store);
    cbm_node_t *nodes = NULL;
    int count = 0;
    ASSERT_EQ(cbm_store_find_nodes_by_file(store, fx.project, "mod.py", &nodes, &count),
              CBM_STORE_OK);
    int leaf = -1;
    for (int i = 0; i < count; i++) {
        if (strcmp(nodes[i].name, "leaf") == 0) {
            leaf = i;
        }
    }
    ASSERT_GTE(leaf, 0);
    const char *props[] = {"{\"is_test\":true}", "{\"test_role\":\"case\"}",
                           "{\"test_role\":\"suite\"}", "{\"test_role\":\"helper\"}"};
    const char *saved = nodes[leaf].properties_json;
    for (int i = 0; i < 4; i++) {
        nodes[leaf].properties_json = props[i];
        ASSERT_GT(cbm_store_upsert_node(store, &nodes[leaf]), 0);
        char *text =
            cwo_call(&fx, ",\"format\":\"json\",\"seeds\":\"list\",\"seed_filter\":\"product\"");
        ASSERT_NOT_NULL(text);
        yyjson_doc *doc = yyjson_read(text, strlen(text), 0);
        ASSERT_NOT_NULL(doc);
        yyjson_val *root = yyjson_doc_get_root(doc);
        ASSERT_NOT_NULL(yyjson_obj_get(root, "seed_total"));
        ASSERT_EQ(yyjson_get_int(yyjson_obj_get(root, "seed_total")), 0);
        ASSERT_EQ(yyjson_get_int(yyjson_obj_get(root, "seed_symbols")), 0);
        ASSERT_EQ(cwo_total(doc), 0);
        yyjson_doc_free(doc);
        free(text);
    }
    nodes[leaf].properties_json = saved;
    cbm_store_free_nodes(nodes, count);
    cbm_store_close(store);
    cwo_fixture_close(&fx);
    PASS();
}

/* The operation answers a misuse with plain error text (no result payload). */
TEST(changes_seed_params_reject_misuse) {
    cwo_fixture_t fx;
    ASSERT_TRUE(cwo_fixture_open(&fx));
    static const struct {
        const char *extra;
        const char *expect;
    } cases[] = {
        {",\"seeds\":\"yes\"", "seeds must be"},
        {",\"seeds\":1", "seeds must be"},
        {",\"seeds\":\"list\",\"seed_filter\":\"tests\"", "seed_filter must be"},
        {",\"seeds\":\"list\",\"seed_limit\":-1", "seed_limit must be"},
        {",\"seeds\":\"list\",\"seed_limit\":1001", "seed_limit must be"},
        {",\"seeds\":\"list\",\"seed_limit\":1.5", "seed_limit must be"},
        {",\"seeds\":\"list\",\"seed_offset\":2147483648", "seed_offset must be"},
        {",\"seeds\":\"list\",\"seed_cursor\":7", "seed_cursor must be"},
        {",\"seeds\":\"list\",\"seed_cursor\":\"invalid\"", "invalid_cursor"},
        {",\"seed_cursor\":\"d1.s.x\"", "seed_cursor requires"},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char *text = cwo_call(&fx, cases[i].extra);
        ASSERT_NOT_NULL(text);
        if (!strstr(text, cases[i].expect)) {
            printf("  case %zu: expected \"%s\" in: %.300s\n", i, cases[i].expect, text);
        }
        ASSERT_NOT_NULL(strstr(text, cases[i].expect));
        free(text);
    }
    cwo_fixture_close(&fx);
    PASS();
}

TEST(changes_product_filter_uses_project_config_and_override) {
    cwo_fixture_t fx;
    ASSERT_TRUE(cwo_fixture_open(&fx));
    char path[600];
    snprintf(path, sizeof(path), "%s/.codebase-memory.json", fx.repo);
    ASSERT_EQ(
        th_write_file(path,
                      "{\"test_impact\":{\"version\":1,\"tests\":{\"test_globs\":[\"mod.py\"]}}}"),
        0);
    char *text =
        cwo_call(&fx, ",\"format\":\"json\",\"seeds\":\"list\",\"seed_filter\":\"product\"");
    ASSERT_NOT_NULL(text);
    yyjson_doc *doc = yyjson_read(text, strlen(text), 0);
    ASSERT_NOT_NULL(doc);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(yyjson_doc_get_root(doc), "seed_total")), 0);
    yyjson_doc_free(doc);
    free(text);
    snprintf(path, sizeof(path), "%s/selection.json", fx.repo);
    ASSERT_EQ(
        th_write_file(path,
                      "{\"test_impact\":{\"version\":1,\"tests\":{\"test_globs\":[\"spec/**\"]}}}"),
        0);
    text = cwo_call(&fx, ",\"format\":\"json\",\"seeds\":\"list\",\"seed_filter\":\"product\","
                         "\"config_path\":\"selection.json\"");
    ASSERT_NOT_NULL(text);
    doc = yyjson_read(text, strlen(text), 0);
    ASSERT_NOT_NULL(doc);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(yyjson_doc_get_root(doc), "seed_total")), 1);
    yyjson_doc_free(doc);
    free(text);
    const char *invalid[] = {
        ",\"config_path\":\"missing.json\"",
        ",\"config_path\":7",
        ",\"config_path\":\"\"",
        ",\"config_path\":\"selection.json\\u0000ignored\"",
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        char extra[300];
        snprintf(extra, sizeof(extra), ",\"seed_filter\":\"product\"%s", invalid[i]);
        text = cwo_call(&fx, extra);
        ASSERT_NOT_NULL(text);
        /* Never an answer: no seeds were computed under a config nobody read. */
        ASSERT_NULL(strstr(text, "seed_symbols"));
        free(text);
    }
    cwo_fixture_close(&fx);
    PASS();
}

/* scope:"tests" answers with the test_impact object. The selection reads
 * committed changes only: the fixture's edit is in the worktree, so against
 * main nothing changed. A config_path that does not exist is evidence missing,
 * never a narrower answer: everything runs. */
TEST(changes_scope_tests_answers_test_impact) {
    cwo_fixture_t fx;
    ASSERT_TRUE(cwo_fixture_open(&fx));
    char *text = cwo_call(&fx, ",\"scope\":\"tests\"");
    ASSERT_NOT_NULL(text);
    if (!strstr(text, "\"decision\":\"nothing\"")) {
        printf("  %.400s\n", text);
    }
    ASSERT_TRUE(strncmp(text, "{\"test_impact\":{", 16) == 0);
    ASSERT_NOT_NULL(strstr(text, "\"decision\":\"nothing\""));
    free(text);
    text = cwo_call(&fx, ",\"scope\":\"tests\",\"config_path\":\"missing/config.json\"");
    ASSERT_NOT_NULL(text);
    ASSERT_NOT_NULL(strstr(text, "\"decision\":\"run_all\""));
    ASSERT_NOT_NULL(strstr(text, "CONFIG_INVALID"));
    free(text);
    cwo_fixture_close(&fx);
    PASS();
}

/* The default answer stops at `depth` (2). fixpoint:true walks until nothing
 * new is reached, and spelling the new parameters out as false changes not one
 * byte of the default answer. */
TEST(changes_fixpoint_walks_past_depth) {
    cwo_fixture_t fx;
    ASSERT_TRUE(cwo_fixture_open(&fx));
    char *plain = cwo_call(&fx, ",\"format\":\"json\"");
    char *spelled = cwo_call(&fx, ",\"format\":\"json\",\"fixpoint\":false,\"via\":false");
    char *fixed = cwo_call(&fx, ",\"format\":\"json\",\"fixpoint\":true");
    ASSERT_NOT_NULL(plain);
    ASSERT_NOT_NULL(spelled);
    ASSERT_NOT_NULL(fixed);
    /* fixpoint/via spelled false still select the walk engine; the default
     * traversal answer and the walk's bounded answer carry the same rows. */
    yyjson_doc *pdoc = yyjson_read(plain, strlen(plain), 0);
    ASSERT_NOT_NULL(pdoc);
    ASSERT_EQ(cwo_total(pdoc), 2);
    ASSERT_NOT_NULL(cwo_row(pdoc, "mid2"));
    ASSERT_NULL(cwo_row(pdoc, "mid3"));
    ASSERT_NULL(yyjson_obj_get(yyjson_doc_get_root(pdoc), "fixpoint"));
    ASSERT_NULL(yyjson_obj_get(yyjson_doc_get_root(pdoc), "edge_types"));

    yyjson_doc *sdoc = yyjson_read(spelled, strlen(spelled), 0);
    ASSERT_NOT_NULL(sdoc);
    ASSERT_EQ(cwo_total(sdoc), 2);

    yyjson_doc *fdoc = yyjson_read(fixed, strlen(fixed), 0);
    ASSERT_NOT_NULL(fdoc);
    yyjson_val *froot = yyjson_doc_get_root(fdoc);
    ASSERT_EQ(cwo_total(fdoc), 4);
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(froot, "impacted_total_relation")), "eq");
    ASSERT_TRUE(yyjson_get_bool(yyjson_obj_get(froot, "fixpoint")));
    ASSERT_EQ(yyjson_arr_size(yyjson_obj_get(froot, "edge_types")), 1);
    ASSERT_STR_EQ(yyjson_get_str(yyjson_arr_get_first(yyjson_obj_get(froot, "edge_types"))),
                  "CALLS");
    yyjson_val *top = cwo_row(fdoc, "top");
    ASSERT_NOT_NULL(top);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(top, "hop")), 4);
    /* The changed function is the seed, never its own impact; a name is not a
     * call. */
    ASSERT_NULL(cwo_row(fdoc, "leaf"));
    ASSERT_NULL(cwo_row(fdoc, "holder"));
    /* Without `via` the rows keep their four columns. */
    ASSERT_NULL(yyjson_obj_get(top, "via_qn"));

    /* fixpoint ignores depth. */
    char *deep = cwo_call(&fx, ",\"format\":\"json\",\"fixpoint\":true,\"depth\":1");
    ASSERT_NOT_NULL(deep);
    yyjson_doc *ddoc = yyjson_read(deep, strlen(deep), 0);
    ASSERT_NOT_NULL(ddoc);
    ASSERT_EQ(cwo_total(ddoc), 4);
    yyjson_doc_free(ddoc);
    free(deep);

    /* A bounded walk through the same engine gives the bounded answer. */
    char *bounded = cwo_call(&fx, ",\"format\":\"json\",\"edge_types\":[\"CALLS\"],\"depth\":3");
    ASSERT_NOT_NULL(bounded);
    yyjson_doc *bdoc = yyjson_read(bounded, strlen(bounded), 0);
    ASSERT_NOT_NULL(bdoc);
    ASSERT_EQ(cwo_total(bdoc), 3);
    ASSERT_FALSE(yyjson_get_bool(yyjson_obj_get(yyjson_doc_get_root(bdoc), "fixpoint")));
    ASSERT_NOT_NULL(cwo_row(bdoc, "mid3"));
    ASSERT_NULL(cwo_row(bdoc, "top"));

    yyjson_doc_free(bdoc);
    yyjson_doc_free(fdoc);
    yyjson_doc_free(sdoc);
    yyjson_doc_free(pdoc);
    free(bounded);
    free(fixed);
    free(spelled);
    free(plain);
    cwo_fixture_close(&fx);
    PASS();
}

/* edge_types widens what counts as "depends on": a function that only names
 * the changed one is reached over CALL_REFERENCE, never over CALLS. */
TEST(changes_edge_types_reach_references) {
    cwo_fixture_t fx;
    ASSERT_TRUE(cwo_fixture_open(&fx));
    char *calls = cwo_call(&fx, ",\"format\":\"json\",\"edge_types\":[\"CALLS\"]");
    char *both =
        cwo_call(&fx, ",\"format\":\"json\",\"edge_types\":[\"CALLS\",\"CALL_REFERENCE\"]");
    ASSERT_NOT_NULL(calls);
    ASSERT_NOT_NULL(both);
    yyjson_doc *cdoc = yyjson_read(calls, strlen(calls), 0);
    yyjson_doc *bdoc = yyjson_read(both, strlen(both), 0);
    ASSERT_NOT_NULL(cdoc);
    ASSERT_NOT_NULL(bdoc);
    ASSERT_NULL(cwo_row(cdoc, "holder"));
    yyjson_val *holder = cwo_row(bdoc, "holder");
    ASSERT_NOT_NULL(holder);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(holder, "hop")), 1);
    ASSERT_NOT_NULL(cwo_row(bdoc, "mid1"));
    ASSERT_EQ(yyjson_arr_size(yyjson_obj_get(yyjson_doc_get_root(bdoc), "edge_types")), 2);

    yyjson_doc_free(bdoc);
    yyjson_doc_free(cdoc);
    free(both);
    free(calls);
    cwo_fixture_close(&fx);
    PASS();
}

/* via:true says how each row was reached: the parent one hop closer and the
 * edge type that led there. Both encodings carry it. */
TEST(changes_via_names_the_parent) {
    cwo_fixture_t fx;
    ASSERT_TRUE(cwo_fixture_open(&fx));
    char *json = cwo_call(&fx, ",\"format\":\"json\",\"via\":true,\"fixpoint\":true,"
                               "\"edge_types\":[\"CALLS\",\"CALL_REFERENCE\"]");
    ASSERT_NOT_NULL(json);
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    ASSERT_NOT_NULL(doc);
    yyjson_val *mid1 = cwo_row(doc, "mid1");
    yyjson_val *mid2 = cwo_row(doc, "mid2");
    yyjson_val *holder = cwo_row(doc, "holder");
    ASSERT_NOT_NULL(mid1);
    ASSERT_NOT_NULL(mid2);
    ASSERT_NOT_NULL(holder);
    /* A row one hop out names the seed, though seeds are not rows. */
    const char *leaf_qn = yyjson_get_str(yyjson_obj_get(mid1, "via_qn"));
    ASSERT_NOT_NULL(leaf_qn);
    ASSERT_NOT_NULL(strstr(leaf_qn, ".leaf"));
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(mid1, "via_edge")), "CALLS");
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(mid2, "via_qn")),
                  yyjson_get_str(yyjson_obj_get(mid1, "qn")));
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(mid2, "via_edge")), "CALLS");
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(holder, "via_qn")), leaf_qn);
    ASSERT_STR_EQ(yyjson_get_str(yyjson_obj_get(holder, "via_edge")), "CALL_REFERENCE");

    char *tree = cwo_call(&fx, ",\"via\":true");
    ASSERT_NOT_NULL(tree);
    ASSERT_NOT_NULL(strstr(tree, "via_qn"));
    ASSERT_NOT_NULL(strstr(tree, "via_edge"));
    char *plain_tree = cwo_call(&fx, "");
    ASSERT_NOT_NULL(plain_tree);
    ASSERT_NULL(strstr(plain_tree, "via_qn"));

    free(plain_tree);
    free(tree);
    yyjson_doc_free(doc);
    free(json);
    cwo_fixture_close(&fx);
    PASS();
}

/* Every misuse is a teaching error; none of them is corrected silently, because
 * a walk over the wrong edges reads as "nothing depends on this". */
TEST(changes_walk_params_teach_on_misuse) {
    cwo_fixture_t fx;
    ASSERT_TRUE(cwo_fixture_open(&fx));
    static const struct {
        const char *extra;
        const char *expect;
    } cases[] = {
        {",\"edge_types\":[\"CALL\"]", "unknown edge type \"CALL\""},
        {",\"edge_types\":[\"CALL\"]", "CALLS"}, /* the error lists what the index has */
        {",\"edge_types\":[]", "edge_types must be"},
        {",\"edge_types\":\"CALLS\"", "edge_types must be"},
        {",\"edge_types\":[\"CALLS\",7]", "edge_types must be"},
        {",\"edge_types\":[\"CALLS\",\"CALLS\"]", "listed twice"},
        {",\"fixpoint\":\"yes\"", "fixpoint must be true or false"},
        {",\"via\":1", "via must be true or false"},
        {",\"fixpoint\":true,\"direction\":\"outbound\"", "direction \"inbound\""},
        {",\"via\":true,\"direction\":\"both\"", "direction \"inbound\""},
        {",\"edge_types\":[\"CALLS\"],\"direction\":\"outbound\"", "direction \"inbound\""},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char *text = cwo_call(&fx, cases[i].extra);
        ASSERT_NOT_NULL(text);
        if (!strstr(text, cases[i].expect)) {
            printf("  case %zu: expected \"%s\" in: %.300s\n", i, cases[i].expect, text);
        }
        ASSERT_NOT_NULL(strstr(text, cases[i].expect));
        free(text);
    }
    cwo_fixture_close(&fx);
    PASS();
}

/* A cursor continues the question it was minted for. The walk parameters are
 * part of that question. */
TEST(changes_cursor_bound_to_walk_params) {
    cwo_fixture_t fx;
    ASSERT_TRUE(cwo_fixture_open(&fx));
    char *first = cwo_call(&fx, ",\"format\":\"json\",\"fixpoint\":true,\"limit\":1");
    ASSERT_NOT_NULL(first);
    yyjson_doc *doc = yyjson_read(first, strlen(first), 0);
    ASSERT_NOT_NULL(doc);
    const char *cursor =
        yyjson_get_str(yyjson_obj_get(yyjson_doc_get_root(doc), "impacted_next_cursor"));
    ASSERT_NOT_NULL(cursor);

    char extra[400];
    snprintf(extra, sizeof(extra),
             ",\"format\":\"json\",\"fixpoint\":true,\"limit\":1,\"impact_cursor\":\"%s\"", cursor);
    char *second = cwo_call(&fx, extra);
    ASSERT_NOT_NULL(second);
    yyjson_doc *sdoc = yyjson_read(second, strlen(second), 0);
    ASSERT_NOT_NULL(sdoc);
    /* Rows come in (hop, qualified name) order: the second page is hop 2. */
    ASSERT_EQ(cwo_total(sdoc), 4);
    yyjson_val *mid2 = cwo_row(sdoc, "mid2");
    ASSERT_NOT_NULL(mid2);
    ASSERT_EQ(yyjson_get_int(yyjson_obj_get(mid2, "hop")), 2);

    static const char *const other_questions[] = {
        ",\"format\":\"json\",\"limit\":1",
        ",\"format\":\"json\",\"fixpoint\":true,\"via\":true,\"limit\":1",
        ",\"format\":\"json\",\"fixpoint\":true,\"limit\":1,\"edge_types\":[\"CALL_REFERENCE\"]",
    };
    for (size_t i = 0; i < sizeof(other_questions) / sizeof(other_questions[0]); i++) {
        snprintf(extra, sizeof(extra), "%s,\"impact_cursor\":\"%s\"", other_questions[i], cursor);
        char *refused = cwo_call(&fx, extra);
        ASSERT_NOT_NULL(refused);
        ASSERT_NOT_NULL(strstr(refused, "cursor_params_mismatch"));
        free(refused);
    }

    yyjson_doc_free(sdoc);
    free(second);
    yyjson_doc_free(doc);
    free(first);
    cwo_fixture_close(&fx);
    PASS();
}

SUITE(changes_operation) {
    RUN_TEST(changes_seed_list_reports_hunk_without_walk);
    RUN_TEST(changes_seed_pages_bind_semantics_and_snapshot);
    RUN_TEST(changes_product_filter_does_not_inflate_fallback);
    RUN_TEST(changes_seed_params_reject_misuse);
    RUN_TEST(changes_product_filter_uses_project_config_and_override);
    RUN_TEST(changes_scope_tests_answers_test_impact);
    RUN_TEST(changes_fixpoint_walks_past_depth);
    RUN_TEST(changes_edge_types_reach_references);
    RUN_TEST(changes_via_names_the_parent);
    RUN_TEST(changes_walk_params_teach_on_misuse);
    RUN_TEST(changes_cursor_bound_to_walk_params);
}
