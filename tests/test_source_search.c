#include "test_framework.h"
#include "test_helpers.h"

#include "foundation/compat_fs.h"
#include "foundation/constants.h"
#include "operations/command_runner.h"
#include "operations/source_search.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define SCAN_FS "\t"
#define SCAN_RS '\n'
#else
#define SCAN_FS ":"
#define SCAN_RS '\0'
#endif

typedef struct {
    char dir[512];
    char list[1024];
    char out[1024];
    char files[4][1024];
    int count;
} scan_fixture_t;

static bool scan_fixture_open(scan_fixture_t *fx) {
    memset(fx, 0, sizeof(*fx));
    char *tmp = th_mktempdir("cbm_scan");
    if (!tmp) {
        return false;
    }
    snprintf(fx->dir, sizeof(fx->dir), "%s", tmp);
    snprintf(fx->list, sizeof(fx->list), "%s/list", fx->dir);
    snprintf(fx->out, sizeof(fx->out), "%s/out", fx->dir);
    return true;
}

static bool scan_fixture_add(scan_fixture_t *fx, const char *name, const char *data,
                             size_t length) {
    if (fx->count >= 4) {
        return false;
    }
    char *path = fx->files[fx->count];
    char full_path[sizeof(fx->files[0])];
    int path_length = snprintf(full_path, sizeof(full_path), "%s/%s", fx->dir, name);
    if (path_length < 0 || path_length >= (int)sizeof(full_path)) {
        return false;
    }
    memcpy(path, full_path, (size_t)path_length + 1);
    FILE *fp = cbm_fopen(path, "wb");
    if (!fp) {
        return false;
    }
    bool ok = fwrite(data, 1, length, fp) == length;
    ok = fclose(fp) == 0 && ok;
    fx->count += ok ? 1 : 0;
    return ok;
}

static bool scan_fixture_list(const scan_fixture_t *fx, const char *extra) {
    FILE *fp = cbm_fopen(fx->list, "wb");
    if (!fp) {
        return false;
    }
    bool ok = true;
    for (int i = 0; i < fx->count; i++) {
        ok = ok && fputs(fx->files[i], fp) != EOF && fputc(SCAN_RS, fp) != EOF;
    }
    if (extra) {
        ok = ok && fputs(extra, fp) != EOF && fputc(SCAN_RS, fp) != EOF;
    }
    return fclose(fp) == 0 && ok;
}

static char *scan_fixture_run(const scan_fixture_t *fx, const char *pattern, bool regex,
                              size_t output_limit, int *cause) {
    *cause = cbm_source_search_scan_for_test(NULL, fx->list, pattern, regex, fx->out, output_limit,
                                             UINT64_MAX);
    FILE *fp = cbm_fopen(fx->out, "rb");
    if (!fp) {
        return NULL;
    }
    char *text = calloc(CBM_SZ_64K, 1);
    size_t length = text ? fread(text, 1, CBM_SZ_64K - 1, fp) : 0;
    (void)fclose(fp);
    if (text) {
        text[length] = '\0';
    }
    return text;
}

static void scan_expect(char *buf, size_t cap, const char *path, int line, const char *content) {
    size_t used = strlen(buf);
    snprintf(buf + used, cap - used, "%s" SCAN_FS "%d" SCAN_FS "%s\n", path, line, content);
}

TEST(source_search_scan_records) {
    scan_fixture_t fx;
    ASSERT_TRUE(scan_fixture_open(&fx));
    static const char a[] = "alpha\nbeta gamma\n\nalpha beta\nlast alpha";
    static const char b[] = "nothing\nalpha\n";
    ASSERT_TRUE(scan_fixture_add(&fx, "a.txt", a, sizeof(a) - 1));
    ASSERT_TRUE(scan_fixture_add(&fx, "b.txt", b, sizeof(b) - 1));
    ASSERT_TRUE(scan_fixture_list(&fx, NULL));
    char want[4096] = "";
    int cause = -1;
    scan_expect(want, sizeof(want), fx.files[0], 1, "alpha");
    scan_expect(want, sizeof(want), fx.files[0], 4, "alpha beta");
    scan_expect(want, sizeof(want), fx.files[0], 5, "last alpha");
    scan_expect(want, sizeof(want), fx.files[1], 2, "alpha");
    char *got = scan_fixture_run(&fx, "alpha", false, 0, &cause);
    ASSERT_NOT_NULL(got);
    ASSERT_EQ(cause, CBM_OPERATION_COMMAND_SUCCESS);
    ASSERT_STR_EQ(got, want);
    free(got);

    want[0] = '\0';
    scan_expect(want, sizeof(want), fx.files[0], 2, "beta gamma");
    scan_expect(want, sizeof(want), fx.files[0], 4, "alpha beta");
    got = scan_fixture_run(&fx, "gamma\nbeta", false, 0, &cause);
    ASSERT_NOT_NULL(got);
    ASSERT_EQ(cause, CBM_OPERATION_COMMAND_SUCCESS);
    ASSERT_STR_EQ(got, want);
    free(got);

    want[0] = '\0';
    scan_expect(want, sizeof(want), fx.files[0], 1, "alpha");
    scan_expect(want, sizeof(want), fx.files[0], 2, "beta gamma");
    scan_expect(want, sizeof(want), fx.files[0], 3, "");
    scan_expect(want, sizeof(want), fx.files[0], 4, "alpha beta");
    scan_expect(want, sizeof(want), fx.files[0], 5, "last alpha");
    scan_expect(want, sizeof(want), fx.files[1], 1, "nothing");
    scan_expect(want, sizeof(want), fx.files[1], 2, "alpha");
    got = scan_fixture_run(&fx, "", false, 0, &cause);
    ASSERT_NOT_NULL(got);
    ASSERT_EQ(cause, CBM_OPERATION_COMMAND_SUCCESS);
    ASSERT_STR_EQ(got, want);
    free(got);

    want[0] = '\0';
    scan_expect(want, sizeof(want), fx.files[0], 1, "alpha");
    scan_expect(want, sizeof(want), fx.files[0], 4, "alpha beta");
    scan_expect(want, sizeof(want), fx.files[0], 5, "last alpha");
    scan_expect(want, sizeof(want), fx.files[1], 2, "alpha");
    got = scan_fixture_run(&fx, "^(alpha|last)", true, 0, &cause);
    ASSERT_NOT_NULL(got);
    ASSERT_EQ(cause, CBM_OPERATION_COMMAND_SUCCESS);
    ASSERT_STR_EQ(got, want);
    free(got);
    got = scan_fixture_run(&fx, "zzqqxx", false, 0, &cause);
    ASSERT_NOT_NULL(got);
    ASSERT_EQ(cause, CBM_OPERATION_COMMAND_SUCCESS);
    ASSERT_STR_EQ(got, "");
    free(got);
    th_rmtree(fx.dir);
    PASS();
}

TEST(source_search_scan_binary_crlf_and_limits) {
    scan_fixture_t fx;
    ASSERT_TRUE(scan_fixture_open(&fx));
    static const char bin[] = "alpha\n\0alpha\n";
    ASSERT_TRUE(scan_fixture_add(&fx, "bin.dat", bin, sizeof(bin) - 1));
    enum { FILLER_LINES = 4000 };
    size_t cap = (size_t)FILLER_LINES * 10 + 64;
    char *late = malloc(cap);
    ASSERT_NOT_NULL(late);
    size_t used = 0;
    for (int i = 0; i < FILLER_LINES; i++) {
        memcpy(late + used, "xxxxxxxxx\n", 10);
        used += 10;
    }
    static const char tail[] = "alpha one\nalpha two\0\nalpha three\n";
    memcpy(late + used, tail, sizeof(tail) - 1);
    used += sizeof(tail) - 1;
    ASSERT_TRUE(scan_fixture_add(&fx, "late.txt", late, used));
    free(late);
    static const char crlf[] = "alpha\r\nbeta\r\n";
    ASSERT_TRUE(scan_fixture_add(&fx, "crlf.txt", crlf, sizeof(crlf) - 1));
    ASSERT_TRUE(scan_fixture_list(&fx, NULL));
    char want[1024] = "";
    scan_expect(want, sizeof(want), fx.files[1], FILLER_LINES + 1, "alpha one");
    scan_expect(want, sizeof(want), fx.files[2], 1, "alpha\r");
    int cause = -1;
    char *got = scan_fixture_run(&fx, "alpha", false, 0, &cause);
    ASSERT_NOT_NULL(got);
    ASSERT_EQ(cause, CBM_OPERATION_COMMAND_SUCCESS);
    ASSERT_STR_EQ(got, want);
    free(got);

    want[0] = '\0';
#ifdef _WIN32
    scan_expect(want, sizeof(want), fx.files[2], 2, "beta\r");
#endif
    got = scan_fixture_run(&fx, "^beta$", true, 0, &cause);
    ASSERT_NOT_NULL(got);
    ASSERT_EQ(cause, CBM_OPERATION_COMMAND_SUCCESS);
    ASSERT_STR_EQ(got, want);
    free(got);
    got = scan_fixture_run(&fx, "alpha", false, 8, &cause);
    free(got);
    ASSERT_EQ(cause, CBM_OPERATION_COMMAND_OUTPUT_LIMIT);
    got = scan_fixture_run(&fx, "(", true, 0, &cause);
    free(got);
    ASSERT_EQ(cause, CBM_OPERATION_COMMAND_FAILURE);
    ASSERT_EQ(cbm_source_search_scan_for_test(NULL, fx.list, "alpha", false, fx.out, 0, 0),
              CBM_OPERATION_COMMAND_DEADLINE);
    th_rmtree(fx.dir);
    PASS();
}

TEST(source_search_scan_fails_closed_on_missing_file) {
    scan_fixture_t fx;
    ASSERT_TRUE(scan_fixture_open(&fx));
    ASSERT_TRUE(scan_fixture_list(&fx, "missing.txt"));
    ASSERT_EQ(cbm_source_search_scan_for_test(NULL, fx.list, "alpha", false, fx.out, 0, UINT64_MAX),
              CBM_OPERATION_COMMAND_FAILURE);
    th_rmtree(fx.dir);
    PASS();
}

SUITE(source_search) {
    RUN_TEST(source_search_scan_records);
    RUN_TEST(source_search_scan_binary_crlf_and_limits);
    RUN_TEST(source_search_scan_fails_closed_on_missing_file);
}

#undef SCAN_FS
#undef SCAN_RS
