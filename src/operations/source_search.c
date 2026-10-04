#include "operations/result_wire.h"
#include "operations/source_search.h"

#include "foundation/compat.h"
#include "foundation/compat_fs.h"
#include "foundation/compat_regex.h"
#include "foundation/constants.h"
#include "foundation/log.h"
#include "foundation/platform.h"
#include "foundation/workspace.h"
#include "operations/command_runner.h"
#include "operations/compact_out.h"
#include "operations/store_host.h"
#include "store/store.h"
#include "yyjson/yyjson.h"

#ifdef _WIN32
#include <io.h>
#define source_fdopen _fdopen
#define source_close _close
#else
#include <unistd.h>
#define source_fdopen fdopen
#define source_close close
#endif

#ifndef _WIN32
#include <fnmatch.h>
#endif

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOURCE_PS_UTF8_PRELUDE "[Console]::OutputEncoding=[System.Text.Encoding]::UTF8; "

enum {
    SOURCE_DEFAULT_LIMIT = 10,
    SOURCE_RETURN_FILES = 2,
    SOURCE_PAIR_LEN = 2,
    SOURCE_SKIP_ONE = 1,
    SOURCE_MAX_RESULT_LIMIT = 500,
    SOURCE_DEFAULT_RAW_LIMIT = 5,
    SOURCE_MAX_RAW_LIMIT = 100,
};

#define SOURCE_SEARCH_OUTPUT_MAX ((size_t)64U * 1024U * 1024U)
#define SOURCE_SEARCH_SCAN_TIMEOUT_MS ((uint64_t)30000U)

static char *source_strdup(const char *text) {
    if (!text)
        return NULL;
    size_t len = strlen(text);
    char *copy = malloc(len + 1U);
    if (copy)
        memcpy(copy, text, len + 1U);
    return copy;
}

static yyjson_doc *source_args_doc(const char *args) {
    const char *json = args ? args : "{}";
    return yyjson_read(json, strlen(json), 0);
}

static char *source_string_arg(const char *args, const char *name) {
    yyjson_doc *doc = source_args_doc(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    char *result = value && yyjson_is_str(value) ? source_strdup(yyjson_get_str(value)) : NULL;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static char *source_project_arg(const char *args) {
    static const char *const names[] = {"project", "project_name", "project_id", "projectName"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        char *value = source_string_arg(args, names[i]);
        if (value)
            return value;
    }
    return NULL;
}

static int source_int_arg(const char *args, const char *name, int fallback) {
    yyjson_doc *doc = source_args_doc(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    int result = value && yyjson_is_int(value) ? (int)yyjson_get_sint(value) : fallback;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static bool source_bool_arg(const char *args, const char *name, bool fallback) {
    yyjson_doc *doc = source_args_doc(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    bool result = value && yyjson_is_bool(value) ? yyjson_get_bool(value) : fallback;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static char *source_doc_to_str(yyjson_mut_doc *doc) {
    return cbm_operation_json_write(doc);
}

static cbm_operation_result_t source_error(const char *message) {
    return cbm_operation_result_copy(message ? message : "source search failed", true);
}

static cbm_operation_result_t source_project_error(const char *project,
                                                   cbm_store_open_status_t open_status) {
    if (open_status == CBM_STORE_OPEN_CORRUPT) {
        return source_error(CBM_STORE_CORRUPT_ERROR);
    }
    if (!project) {
        return source_error("{\"error\":\"missing required argument: project\",\"hint\":\"Pass the "
                            "project argument. Run projects to see indexed projects.\"}");
    }
    return source_error("{\"error\":\"project not found or not indexed\",\"hint\":\"Run projects "
                        "to see indexed projects.\"}");
}

static cbm_store_t *source_open_store_and_root(const char *project, char **root_path_out,
                                               cbm_store_open_status_t *open_status) {
    *root_path_out = NULL;
    *open_status = CBM_STORE_OPEN_NOT_FOUND;
    if (!project || !project[0])
        return NULL;
    cbm_store_t *store = cbm_store_host_open_query(project, open_status);
    if (!store)
        return NULL;
    cbm_project_t info = {0};
    if (cbm_store_get_project(store, project, &info) != CBM_STORE_OK || !info.root_path ||
        !info.root_path[0]) {
        cbm_project_free_fields(&info);
        cbm_store_close(store);
        return NULL;
    }
    *root_path_out = source_strdup(info.root_path);
    cbm_project_free_fields(&info);
    if (!*root_path_out) {
        cbm_store_close(store);
        return NULL;
    }
    return store;
}

/* Read lines [start, end] byte for byte. Reading by line buffer would split a
 * multi-byte sequence at the buffer boundary; invalid bytes are preserved here
 * and encoded losslessly at the output boundary. */
static char *source_read_file_lines(const char *path, int start, int end) {
    FILE *fp = cbm_fopen(path, "rb");
    if (!fp)
        return NULL;
    cbm_sb_t selected;
    cbm_sb_init(&selected);
    int lineno = 1;
    int byte;
    while ((byte = fgetc(fp)) != EOF) {
        if (lineno >= start && lineno <= end) {
            char ch = (char)byte;
            cbm_sb_append_n(&selected, &ch, 1U);
        }
        if (byte == '\n') {
            if (lineno >= end)
                break;
            lineno++;
        }
    }
    (void)fclose(fp);
    char *result = cbm_sb_finish(&selected);
    if (result && result[0] == '\0') {
        free(result);
        return NULL;
    }
    return result;
}

static bool source_utf8_is_cont(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

/* ── search_code v2: graph-augmented code search ─────────────── */

/* Intermediate grep match */
typedef struct {
    char file[CBM_SZ_512];
    int line;
    char content[CBM_SZ_1K];
    /* Preview window into the original grep line. The line may be longer than
     * `content`; these byte counters let a caller page it exactly. */
    size_t content_start_byte;
    size_t content_returned_bytes;
    size_t content_total_bytes;
    size_t match_start_byte;
    size_t match_end_byte;
    bool match_known;
    bool content_truncated;
} grep_match_t;

/* Deduped result: one per containing graph node */
typedef struct {
    int64_t node_id; /* 0 = raw match (no containing node) */
    char node_name[CBM_SZ_256];
    char qualified_name[CBM_SZ_512];
    char label[CBM_SZ_64];
    char file[CBM_SZ_512];
    int start_line;
    int end_line;
    int in_degree;
    int out_degree;
    int score;
    int match_lines[CBM_SZ_64];
    int match_count; /* retained line numbers (at most CBM_SZ_64) */
    int match_total; /* every hit that landed in this node */
} search_result_t;

typedef struct {
    uint64_t scope_ms;
    uint64_t scan_ms;
    uint64_t enrich_ms;
    uint64_t elapsed_ms;
    bool include_phase_timings;
} search_metrics_t;

/* Score a result for ranking: project source first, vendored last, tests lowest */
enum { SCORE_FUNC = 10, SCORE_ROUTE = 15, SCORE_VENDORED = -50, SCORE_TEST = -5 };
enum { MAX_LINE_SPAN = 999999 };

static int compute_search_score(const search_result_t *r) {
    int score = r->in_degree;
    if (strcmp(r->label, "Function") == 0 || strcmp(r->label, "Method") == 0) {
        score += SCORE_FUNC;
    }
    if (strcmp(r->label, "Route") == 0) {
        score += SCORE_ROUTE;
    }
    if (strstr(r->file, "vendored/") || strstr(r->file, "vendor/") ||
        strstr(r->file, "node_modules/")) {
        score += SCORE_VENDORED;
    }
    /* Penalize test files */
    if (strstr(r->file, "test") || strstr(r->file, "spec") || strstr(r->file, "_test.")) {
        score += SCORE_TEST;
    }
    return score;
}

static int nullable_strcmp(const char *a, const char *b) {
    return strcmp(a ? a : "", b ? b : "");
}

static int search_result_cmp(const void *a, const void *b) {
    const search_result_t *ra = (const search_result_t *)a;
    const search_result_t *rb = (const search_result_t *)b;
    int score_order = rb->score - ra->score; /* descending */
    if (score_order != 0) {
        return score_order;
    }
    /* Equal scores are common; order them by identity so result_offset pages
     * are stable across calls. */
    int qn_order = nullable_strcmp(ra->qualified_name, rb->qualified_name);
    if (qn_order != 0) {
        return qn_order;
    }
    int file_order = nullable_strcmp(ra->file, rb->file);
    if (file_order != 0) {
        return file_order;
    }
    if (ra->start_line != rb->start_line) {
        return ra->start_line < rb->start_line ? -1 : 1;
    }
    if (ra->end_line != rb->end_line) {
        return ra->end_line < rb->end_line ? -1 : 1;
    }
    return 0;
}

/* Moving an arbitrary file_pattern ahead of Select-String is not generally results-preserving:
 * the current Windows path applies PowerShell -like to the full MatchInfo.Path, while POSIX
 * delegates glob semantics to grep --include. Restrict the Windows optimization to plain suffix
 * globs whose meaning cannot depend on path separator normalization or directory components. The
 * original post-scan filter remains in place as a second guard. */
bool cbm_search_code_file_pattern_can_prefilter(const char *file_pattern) {
    if (!file_pattern || file_pattern[0] != '*' || file_pattern[1] != '.' ||
        file_pattern[2] == '\0') {
        return false;
    }
    for (const unsigned char *p = (const unsigned char *)file_pattern + 2; *p; p++) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') ||
              *p == '.' || *p == '_' || *p == '-')) {
            return false;
        }
    }
    return true;
}

bool cbm_search_code_windows_path_matches_prefilter(const char *path, const char *file_pattern) {
    if (!path || !cbm_search_code_file_pattern_can_prefilter(file_pattern)) {
        return false;
    }

    const char *suffix = file_pattern + 1;
    size_t path_len = strlen(path);
    size_t suffix_len = strlen(suffix);
    if (path_len < suffix_len) {
        return false;
    }

    const unsigned char *candidate = (const unsigned char *)path + path_len - suffix_len;
    const unsigned char *expected = (const unsigned char *)suffix;
    for (size_t i = 0; i < suffix_len; i++) {
        unsigned char left = candidate[i];
        unsigned char right = expected[i];
        if (left >= 'A' && left <= 'Z') {
            left = (unsigned char)(left - 'A' + 'a');
        }
        if (right >= 'A' && right <= 'Z') {
            right = (unsigned char)(right - 'A' + 'a');
        }
        if (left != right) {
            return false;
        }
    }
    return true;
}

/* Build the grep/search command string based on scoped vs recursive mode.
 * On Windows, uses PowerShell Select-String with tab-delimited output.
 * On POSIX, uses grep with colon-delimited output. */
/* Windows PowerShell 5.1 encodes stdout for a native-process pipe in the
 * console OEM codepage, so any character the inherited CP cannot carry
 * (Cyrillic under CP437/850, etc.) degrades to '?' before it ever reaches
 * collect_grep_matches — and WHETHER it degrades depends on the console the
 * server happened to inherit. Pin the pipe to UTF-8 inside every generated
 * command so raw search content is codepage-independent. (The read side is
 * already safe: Select-String decodes BOM-less UTF-8 via .NET StreamReader
 * defaults.) */

void cbm_search_code_build_grep_cmd(char *cmd, size_t cmd_sz, bool use_regex, bool scoped,
                                    const char *file_pattern, const char *tmpfile,
                                    const char *filelist, const char *root_path) {
#ifdef _WIN32
    const char *sm = use_regex ? "" : " -SimpleMatch";
    if (scoped) {
        if (file_pattern) {
            snprintf(
                cmd, cmd_sz,
                "powershell -Command \"" SOURCE_PS_UTF8_PRELUDE
                "$pat = Get-Content -Encoding UTF8 -LiteralPath '%s'; "
                "Get-Content -Encoding UTF8 -LiteralPath '%s' | ForEach-Object { Select-String "
                "-LiteralPath $_ -Pattern $pat%s "
                "-ErrorAction Stop }"
                " | Where-Object { $_.Path -like '*%s' }"
                " | ForEach-Object { $_.Path + [char]9 + $_.LineNumber + [char]9 + $_.Line }\"",
                tmpfile, filelist, sm, file_pattern);
        } else {
            snprintf(
                cmd, cmd_sz,
                "powershell -Command \"" SOURCE_PS_UTF8_PRELUDE
                "$pat = Get-Content -Encoding UTF8 -LiteralPath '%s'; "
                "Get-Content -Encoding UTF8 -LiteralPath '%s' | ForEach-Object { Select-String "
                "-LiteralPath $_ -Pattern $pat%s "
                "-ErrorAction Stop }"
                " | ForEach-Object { $_.Path + [char]9 + $_.LineNumber + [char]9 + $_.Line }\"",
                tmpfile, filelist, sm);
        }
    } else {
        if (file_pattern) {
            snprintf(
                cmd, cmd_sz,
                "powershell -Command \"" SOURCE_PS_UTF8_PRELUDE
                "Get-ChildItem -Recurse -Path '%s\\*' -Include '%s' -File "
                "-ErrorAction SilentlyContinue"
                " | Select-String -Pattern (Get-Content -Encoding UTF8 -LiteralPath '%s')%s "
                "-ErrorAction Stop"
                " | ForEach-Object { $_.Path + [char]9 + $_.LineNumber + [char]9 + $_.Line }\"",
                root_path, file_pattern, tmpfile, sm);
        } else {
            snprintf(
                cmd, cmd_sz,
                "powershell -Command \"" SOURCE_PS_UTF8_PRELUDE
                "Get-ChildItem -Recurse -Path '%s\\*' -File -ErrorAction "
                "SilentlyContinue"
                " | Select-String -Pattern (Get-Content -Encoding UTF8 -LiteralPath '%s')%s "
                "-ErrorAction Stop"
                " | ForEach-Object { $_.Path + [char]9 + $_.LineNumber + [char]9 + $_.Line }\"",
                root_path, tmpfile, sm);
        }
    }
#else
    const char *flag = use_regex ? "-E" : "-F";
    if (scoped) {
        /* file_pattern was already applied to the canonical file list in C.
         * Keep this scan compatible with BusyBox grep, which has no GNU
         * --include option (the shipped Alpine/static runtime). Grep's no-match
         * status maps to 0 so every non-zero exit is an incomplete scan. */
        snprintf(cmd, cmd_sz,
                 "xargs -0 sh -c 'pat=$1; shift; if [ \"$#\" -eq 0 ]; then exit 0; fi; "
                 "grep -Hn %s -f \"$pat\" -- \"$@\"; rc=$?; if [ \"$rc\" -eq 1 ]; then "
                 "exit 0; fi; if [ \"$rc\" -ne 0 ]; then exit 255; fi; exit 0' sh '%s' "
                 "< '%s' 2>/dev/null",
                 flag, tmpfile, filelist);
    } else {
        /* Do not pipe discovery directly into sort/xargs: POSIX sh reports only
         * the final pipeline command, which can hide a partial find or failed
         * sort behind a successful grep. Materialize the NUL list in the
         * request-private scratch file and check each producer before scanning.
         * The xargs wrapper's zero-argument guard also makes empty discovery
         * succeed on both GNU (runs once) and BSD (runs zero times) xargs. */
        if (file_pattern) {
            snprintf(cmd, cmd_sz,
                     "fl='%s'; find '%s' -type f -name '%s' -print0 > \"$fl\" 2>/dev/null; "
                     "rc=$?; if [ \"$rc\" -ne 0 ]; then exit \"$rc\"; fi; "
                     "LC_ALL=C sort -z -o \"$fl\" \"$fl\" 2>/dev/null; rc=$?; "
                     "if [ \"$rc\" -ne 0 ]; then exit \"$rc\"; fi; "
                     "xargs -0 sh -c 'pat=$1; shift; if [ \"$#\" -eq 0 ]; then exit 0; fi; "
                     "grep -Hn %s -f \"$pat\" -- \"$@\"; rc=$?; if [ \"$rc\" -eq 1 ]; then "
                     "exit 0; fi; if [ \"$rc\" -ne 0 ]; then exit 255; fi; exit 0' sh '%s' "
                     "< \"$fl\" 2>/dev/null",
                     filelist, root_path, file_pattern, flag, tmpfile);
        } else {
            snprintf(cmd, cmd_sz,
                     "fl='%s'; find '%s' -type f -print0 > \"$fl\" 2>/dev/null; rc=$?; "
                     "if [ \"$rc\" -ne 0 ]; then exit \"$rc\"; fi; "
                     "LC_ALL=C sort -z -o \"$fl\" \"$fl\" 2>/dev/null; rc=$?; "
                     "if [ \"$rc\" -ne 0 ]; then exit \"$rc\"; fi; "
                     "xargs -0 sh -c 'pat=$1; shift; if [ \"$#\" -eq 0 ]; then exit 0; fi; "
                     "grep -Hn %s -f \"$pat\" -- \"$@\"; rc=$?; if [ \"$rc\" -eq 1 ]; then "
                     "exit 0; fi; if [ \"$rc\" -ne 0 ]; then exit 255; fi; exit 0' sh '%s' "
                     "< \"$fl\" 2>/dev/null",
                     filelist, root_path, flag, tmpfile);
        }
    }
#endif
}

/* Build deduplicated file list from the requested result window + the raw
 * rows retained for the requested raw page. */
static yyjson_mut_val *build_dedup_files_array(yyjson_mut_doc *doc, search_result_t *sr,
                                               int result_start, int output_count,
                                               grep_match_t *raw, int raw_count) {
    yyjson_mut_val *files_arr = yyjson_mut_arr(doc);
    size_t seen_capacity = (size_t)output_count + (size_t)raw_count;
    const char **seen_files = seen_capacity > 0 ? calloc(seen_capacity, sizeof(*seen_files)) : NULL;
    if (!files_arr || (seen_capacity > 0 && !seen_files)) {
        free(seen_files);
        return NULL;
    }
    int seen_count = 0;
    for (int fi = 0; fi < output_count; fi++) {
        const char *file = sr[result_start + fi].file;
        bool dup = false;
        for (int j = 0; j < seen_count; j++) {
            if (strcmp(seen_files[j], file) == 0) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            seen_files[seen_count++] = file;
            yyjson_mut_arr_add_strcpy(doc, files_arr, file);
        }
    }
    for (int fi = 0; fi < raw_count; fi++) {
        bool dup = false;
        for (int j = 0; j < seen_count; j++) {
            if (strcmp(seen_files[j], raw[fi].file) == 0) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            seen_files[seen_count++] = raw[fi].file;
            yyjson_mut_arr_add_strcpy(doc, files_arr, raw[fi].file);
        }
    }
    free(seen_files);
    return files_arr;
}

/* Attach source or context lines to a search result JSON item. */
static void attach_result_source(yyjson_mut_doc *doc, yyjson_mut_val *item, search_result_t *r,
                                 int mode, int context_lines, const char *root_path) {
    enum { MODE_FULL = 1 };
    if (r->start_line <= 0 || r->end_line <= 0) {
        return;
    }
    char abs_path[CBM_SZ_1K];
    snprintf(abs_path, sizeof(abs_path), "%s/%s", root_path, r->file);

    /* Containment: a search result whose indexed path resolves outside the
     * project root (a `..` segment, or a symlink/junction that discovery
     * followed) must not be read back into the response. Same guard the
     * snippet path already uses. */
    if (!cbm_path_within_root(root_path, abs_path)) {
        return;
    }

    if (mode == MODE_FULL) {
        /* Cap each hit's source at a match-anchored window: uncapped
         * whole-symbol dumps ran to 5.7KB × N hits (142KB responses). The
         * complete symbol stays one get_code_snippet call away;
         * source_start/source_truncated make the cut explicit. */
        enum { SC_FULL_MAX_LINES = 60, SC_FULL_LEAD = 5 };
        int s = r->start_line;
        int e = r->end_line;
        bool truncated = false;
        if (e - s + 1 > SC_FULL_MAX_LINES) {
            if (r->match_count > 0 && r->match_lines[0] - SC_FULL_LEAD > s) {
                s = r->match_lines[0] - SC_FULL_LEAD;
            }
            e = s + SC_FULL_MAX_LINES - 1;
            if (e > r->end_line) {
                e = r->end_line;
            }
            truncated = true;
        }
        char *source = source_read_file_lines(abs_path, s, e);
        if (source) {
            yyjson_mut_obj_add_strcpy(doc, item, "source", source);
            free(source);
            if (truncated) {
                yyjson_mut_obj_add_int(doc, item, "source_start", s);
                yyjson_mut_obj_add_bool(doc, item, "source_truncated", true);
            }
        }
    } else if (context_lines > 0 && r->match_count > 0) {
        int ctx_start = r->match_lines[0] - context_lines;
        int ctx_end = r->match_lines[r->match_count - SOURCE_SKIP_ONE] + context_lines;
        if (ctx_start < SOURCE_SKIP_ONE) {
            ctx_start = SOURCE_SKIP_ONE;
        }
        char *ctx = source_read_file_lines(abs_path, ctx_start, ctx_end);
        if (ctx) {
            yyjson_mut_obj_add_strcpy(doc, item, "context", ctx);
            yyjson_mut_obj_add_int(doc, item, "context_start", ctx_start);
            free(ctx);
        }
    }
}

/* Build directory distribution object from search results (top-level dir → count). */
/* Aggregate hits by top-level directory. Shared by the JSON object and the
 * TOON table emission. Returns the number of distinct directories. */
static int aggregate_search_dirs(search_result_t *sr, int sr_count, char dir_names[][CBM_SZ_128],
                                 int *dir_counts, int max_dirs) {
    int dir_n = 0;
    for (int di = 0; di < sr_count; di++) {
        char top[CBM_SZ_128] = "";
        const char *slash = strchr(sr[di].file, '/');
        if (slash) {
            size_t dlen = (size_t)(slash - sr[di].file + SOURCE_SKIP_ONE);
            if (dlen >= sizeof(top)) {
                dlen = sizeof(top) - SOURCE_SKIP_ONE;
            }
            memcpy(top, sr[di].file, dlen);
            top[dlen] = '\0';
        } else {
            snprintf(top, sizeof(top), "%s", sr[di].file);
        }
        int found = CBM_NOT_FOUND;
        for (int d = 0; d < dir_n; d++) {
            if (strcmp(dir_names[d], top) == 0) {
                found = d;
                break;
            }
        }
        if (found >= 0) {
            dir_counts[found]++;
        } else if (dir_n < max_dirs) {
            snprintf(dir_names[dir_n], CBM_SZ_128, "%s", top);
            dir_counts[dir_n] = SOURCE_SKIP_ONE;
            dir_n++;
        }
    }
    return dir_n;
}

static yyjson_mut_val *build_dir_distribution(yyjson_mut_doc *doc, search_result_t *sr,
                                              int sr_count) {
    yyjson_mut_val *dirs = yyjson_mut_obj(doc);
    char dir_names[CBM_SZ_64][CBM_SZ_128];
    int dir_counts[CBM_SZ_64];
    int dir_n = aggregate_search_dirs(sr, sr_count, dir_names, dir_counts, CBM_SZ_64);
    for (int d = 0; d < dir_n; d++) {
        yyjson_mut_val *key = yyjson_mut_strcpy(doc, dir_names[d]);
        yyjson_mut_val *val = yyjson_mut_int(doc, dir_counts[d]);
        yyjson_mut_obj_add(dirs, key, val);
    }
    return dirs;
}

static bool raw_content_has_next(const grep_match_t *match) {
    return match->content_start_byte + match->content_returned_bytes < match->content_total_bytes;
}

static bool raw_match_fully_returned(const grep_match_t *match) {
    if (!match->match_known) {
        return false;
    }
    size_t content_end = match->content_start_byte + match->content_returned_bytes;
    return match->content_start_byte <= match->match_start_byte &&
           content_end >= match->match_end_byte;
}

/* The requested result/raw window and the totals around it. */
typedef struct {
    int sr_count;
    int raw_count; /* exact number of unclassified hits */
    int raw_content_truncated;
    int gm_count;
    int result_start;
    int result_limit;
    int output_count;
    int raw_start;
    int raw_limit;
    int raw_output; /* rows retained for the requested raw page */
} search_page_t;

/* Pagination scalars shared by the tree and JSON encodings. */
static void search_page_scalars_tree(cbm_sb_t *sb, const search_page_t *page) {
    bool has_more = page->result_start + page->output_count < page->sr_count;
    cbm_tree_scalar_str(sb, "total_relation", "eq");
    cbm_tree_scalar_int(sb, "result_offset", page->result_start);
    cbm_tree_scalar_int(sb, "results_returned", page->output_count);
    cbm_tree_scalar_bool(sb, "has_more", has_more);
    if (has_more && page->output_count > 0)
        cbm_tree_scalar_int(sb, "next_offset", page->result_start + page->output_count);
    cbm_tree_scalar_int(sb, "raw_offset", page->raw_start);
    cbm_tree_scalar_int(sb, "raw_returned", page->raw_output);
    bool raw_has_more = page->raw_start + page->raw_output < page->raw_count;
    cbm_tree_scalar_bool(sb, "raw_has_more", raw_has_more);
    if (raw_has_more && page->raw_output > 0)
        cbm_tree_scalar_int(sb, "raw_next_offset", page->raw_start + page->raw_output);
    else if (raw_has_more)
        cbm_tree_scalar_bool(sb, "raw_continuation_requires_positive_limit", true);
    if (page->raw_content_truncated > 0)
        cbm_tree_scalar_int(sb, "raw_content_truncated", page->raw_content_truncated);
    cbm_tree_scalar_bool(sb, "truncated", has_more || raw_has_more);
}

static void search_page_scalars_json(yyjson_mut_doc *doc, yyjson_mut_val *root,
                                     const search_page_t *page) {
    bool has_more = page->result_start + page->output_count < page->sr_count;
    yyjson_mut_obj_add_str(doc, root, "total_relation", "eq");
    yyjson_mut_obj_add_int(doc, root, "result_offset", page->result_start);
    yyjson_mut_obj_add_int(doc, root, "results_returned", page->output_count);
    yyjson_mut_obj_add_bool(doc, root, "has_more", has_more);
    if (has_more && page->output_count > 0)
        yyjson_mut_obj_add_int(doc, root, "next_offset", page->result_start + page->output_count);
    yyjson_mut_obj_add_int(doc, root, "raw_offset", page->raw_start);
    yyjson_mut_obj_add_int(doc, root, "raw_returned", page->raw_output);
    bool raw_has_more = page->raw_start + page->raw_output < page->raw_count;
    yyjson_mut_obj_add_bool(doc, root, "raw_has_more", raw_has_more);
    if (raw_has_more && page->raw_output > 0)
        yyjson_mut_obj_add_int(doc, root, "raw_next_offset", page->raw_start + page->raw_output);
    else if (raw_has_more)
        yyjson_mut_obj_add_bool(doc, root, "raw_continuation_requires_positive_limit", true);
    if (page->raw_content_truncated > 0)
        yyjson_mut_obj_add_int(doc, root, "raw_content_truncated", page->raw_content_truncated);
    yyjson_mut_obj_add_bool(doc, root, "truncated", has_more || raw_has_more);
}

static void search_lines_text(const search_result_t *r, char *out, size_t out_size) {
    if (r->start_line > 0)
        snprintf(out, out_size, "%d-%d", r->start_line,
                 r->end_line > r->start_line ? r->end_line : r->start_line);
    else
        out[0] = '\0';
}

/* match line numbers ';'-joined (no comma, so no cell quoting) */
static void search_matches_text(const search_result_t *r, char *out, size_t out_size) {
    size_t used = 0;
    out[0] = '\0';
    for (int j = 0; j < r->match_count && used + 12 < out_size; j++) {
        int n = snprintf(out + used, out_size - used, "%s%d", j > 0 ? ";" : "", r->match_lines[j]);
        if (n < 0)
            break;
        used += (size_t)n;
    }
}

/* TOON emission for compact-mode search results: one row per hit
 * (qn/label/file/lines/matches/degrees — `node` dropped, it duplicates the
 * qn's last segment), a raw[] table for uncorrelated matches, a dirs[]
 * distribution table, and the summary scalars. */
static char *assemble_search_output_toon(search_result_t *sr, grep_match_t *raw,
                                         const search_page_t *page, bool warn_literal_pipe,
                                         const search_metrics_t *metrics) {
    enum { SEARCH_SLOW_MS = 5000, RESULT_COLS = 8, RAW_COLS = 11, RAW_TEXT_FIELDS = 9 };
    cbm_sb_t sb;
    cbm_sb_init(&sb);

    static const char *const cols[] = {"qn",      "label",           "file", "lines",
                                       "matches", "matches_omitted", "in",   "out"};
    typedef struct {
        char lines[CBM_SZ_32];
        char matches[CBM_SZ_256];
        char matches_omitted[CBM_SZ_32];
        char inbound[CBM_SZ_32];
        char outbound[CBM_SZ_32];
    } search_tree_row_t;
    int output_count = page->output_count;
    search_tree_row_t *rendered =
        output_count > 0 ? calloc((size_t)output_count, sizeof(*rendered)) : NULL;
    const char **cells =
        output_count > 0 ? calloc((size_t)output_count * RESULT_COLS, sizeof(*cells)) : NULL;
    if (output_count > 0 && (!rendered || !cells)) {
        free(cells);
        free(rendered);
        cbm_sb_free(&sb);
        return NULL;
    }
    for (int ri = 0; ri < output_count; ri++) {
        search_result_t *r = &sr[page->result_start + ri];
        search_lines_text(r, rendered[ri].lines, sizeof(rendered[ri].lines));
        search_matches_text(r, rendered[ri].matches, sizeof(rendered[ri].matches));
        snprintf(rendered[ri].matches_omitted, sizeof(rendered[ri].matches_omitted), "%d",
                 r->match_total - r->match_count);
        snprintf(rendered[ri].inbound, sizeof(rendered[ri].inbound), "%d", r->in_degree);
        snprintf(rendered[ri].outbound, sizeof(rendered[ri].outbound), "%d", r->out_degree);
        size_t base = (size_t)ri * RESULT_COLS;
        cells[base] = r->qualified_name;
        cells[base + 1] = r->label;
        cells[base + 2] = r->file;
        cells[base + 3] = rendered[ri].lines;
        cells[base + 4] = rendered[ri].matches;
        cells[base + 5] = rendered[ri].matches_omitted;
        cells[base + 6] = rendered[ri].inbound;
        cells[base + 7] = rendered[ri].outbound;
    }
    if (output_count > 0) {
        static const bool string_cols[] = {true, true, true, true, true, false, false, false};
        static const bool prefix_cols[] = {true, false, true, false, false, false, false, false};
        cbm_tree_table_rows_profiled(&sb, "results", output_count, cols, RESULT_COLS, cells,
                                     string_cols, prefix_cols);
    } else {
        cbm_tree_table_header(&sb, "results", 0, cols, RESULT_COLS);
    }
    free(cells);
    free(rendered);

    int raw_output = page->raw_output;
    if (raw_output > 0) {
        static const char *const rcols[] = {"file",
                                            "line",
                                            "content",
                                            "content_start_byte",
                                            "content_returned_bytes",
                                            "content_total_bytes",
                                            "match_start_byte",
                                            "match_end_byte",
                                            "content_has_more",
                                            "content_next_offset",
                                            "match_fully_returned"};
        const char **raw_cells = calloc((size_t)raw_output * RAW_COLS, sizeof(*raw_cells));
        char (*raw_text)[RAW_TEXT_FIELDS][CBM_SZ_32] =
            calloc((size_t)raw_output, sizeof(*raw_text));
        if (!raw_cells || !raw_text) {
            free(raw_text);
            free(raw_cells);
            cbm_sb_free(&sb);
            return NULL;
        }
        for (int ri = 0; ri < raw_output; ri++) {
            grep_match_t *r = &raw[ri];
            snprintf(raw_text[ri][0], sizeof(raw_text[ri][0]), "%d", r->line);
            snprintf(raw_text[ri][1], sizeof(raw_text[ri][1]), "%zu", r->content_start_byte);
            snprintf(raw_text[ri][2], sizeof(raw_text[ri][2]), "%zu", r->content_returned_bytes);
            snprintf(raw_text[ri][3], sizeof(raw_text[ri][3]), "%zu", r->content_total_bytes);
            if (r->match_known) {
                snprintf(raw_text[ri][4], sizeof(raw_text[ri][4]), "%zu", r->match_start_byte);
                snprintf(raw_text[ri][5], sizeof(raw_text[ri][5]), "%zu", r->match_end_byte);
            }
            snprintf(raw_text[ri][6], sizeof(raw_text[ri][6]), "%s",
                     raw_content_has_next(r) ? "true" : "false");
            if (raw_content_has_next(r))
                snprintf(raw_text[ri][7], sizeof(raw_text[ri][7]), "%zu",
                         r->content_start_byte + r->content_returned_bytes);
            snprintf(raw_text[ri][8], sizeof(raw_text[ri][8]), "%s",
                     raw_match_fully_returned(r) ? "true" : "false");
            size_t base = (size_t)ri * RAW_COLS;
            raw_cells[base] = r->file;
            raw_cells[base + 1U] = raw_text[ri][0];
            raw_cells[base + 2U] = r->content;
            for (size_t field = 1; field < RAW_TEXT_FIELDS; field++)
                raw_cells[base + field + 2U] = raw_text[ri][field];
        }
        static const bool raw_string_cols[] = {true,  false, true,  false, false, false,
                                               false, false, false, false, false};
        static const bool raw_prefix_cols[] = {true,  false, false, false, false, false,
                                               false, false, false, false, false};
        cbm_tree_table_rows_profiled(&sb, "raw", raw_output, rcols, RAW_COLS, raw_cells,
                                     raw_string_cols, raw_prefix_cols);
        free(raw_text);
        free(raw_cells);
    }

    char dir_names[CBM_SZ_64][CBM_SZ_128];
    int dir_counts[CBM_SZ_64];
    int dir_n = aggregate_search_dirs(sr, page->sr_count, dir_names, dir_counts, CBM_SZ_64);
    if (dir_n > 0) {
        static const char *const dcols[] = {"dir", "hits"};
        cbm_tree_table_header(&sb, "dirs", dir_n, dcols, 2);
        for (int d = 0; d < dir_n; d++) {
            cbm_tree_row_begin(&sb);
            cbm_tree_cell_str(&sb, dir_names[d], true);
            cbm_tree_cell_int(&sb, dir_counts[d], false);
            cbm_tree_row_end(&sb);
        }
    }

    cbm_tree_scalar_int(&sb, "total_grep_matches", page->gm_count);
    cbm_tree_scalar_int(&sb, "total_results", page->sr_count);
    cbm_tree_scalar_int(&sb, "raw_match_count", page->raw_count);
    search_page_scalars_tree(&sb, page);
    if (metrics->include_phase_timings) {
        cbm_tree_scalar_int(&sb, "scope_ms", (long long)metrics->scope_ms);
        cbm_tree_scalar_int(&sb, "scan_ms", (long long)metrics->scan_ms);
        cbm_tree_scalar_int(&sb, "enrich_ms", (long long)metrics->enrich_ms);
    }
    cbm_tree_scalar_int(&sb, "elapsed_ms", (long long)metrics->elapsed_ms);
    if (warn_literal_pipe) {
        cbm_tree_scalar_str(&sb, "warning",
                            "pattern contains '|' but regex=false, so it is matched literally "
                            "(not as alternation). Pass regex=true for 'foo|bar' to mean "
                            "'foo OR bar'.");
    }
    if (metrics->elapsed_ms >= SEARCH_SLOW_MS) {
        cbm_tree_scalar_str(&sb, "warning_slow",
                            "search was slow; narrow file_pattern/path_filter or use a more "
                            "specific pattern");
    }
    return cbm_sb_finish(&sb);
}

/* Phase 4: assemble JSON output from search results */
static char *assemble_search_output(search_result_t *sr, grep_match_t *raw,
                                    const search_page_t *page, int mode, int context_lines,
                                    const char *root_path, bool warn_literal_pipe,
                                    const search_metrics_t *metrics) {
    enum { MODE_COMPACT = 0, MODE_FULL = 1, MODE_FILES = 2, SEARCH_SLOW_MS = 5000 };

    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root_obj = doc ? yyjson_mut_obj(doc) : NULL;
    if (!root_obj) {
        if (doc)
            yyjson_mut_doc_free(doc);
        return NULL;
    }
    yyjson_mut_doc_set_root(doc, root_obj);

    if (mode == MODE_FILES) {
        yyjson_mut_val *files = build_dedup_files_array(doc, sr, page->result_start,
                                                        page->output_count, raw, page->raw_output);
        if (!files) {
            yyjson_mut_doc_free(doc);
            return NULL;
        }
        yyjson_mut_obj_add_val(doc, root_obj, "files", files);
    } else {
        /* json-stringified tree: cols + column-ordered row arrays. FULL mode
         * appends a per-row object cell with the (guarded, windowed) source;
         * context requests append the corresponding context object. */
        bool attach_context = context_lines > 0 && mode != MODE_FULL;
        yyjson_mut_val *jcols = yyjson_mut_arr(doc);
        static const char *const sc_cols[] = {"qn",      "label",           "file", "lines",
                                              "matches", "matches_omitted", "in",   "out"};
        for (size_t ci = 0; ci < sizeof(sc_cols) / sizeof(sc_cols[0]); ci++) {
            yyjson_mut_arr_add_str(doc, jcols, sc_cols[ci]);
        }
        if (mode == MODE_FULL || attach_context) {
            yyjson_mut_arr_add_str(doc, jcols, mode == MODE_FULL ? "source" : "context");
        }
        yyjson_mut_obj_add_val(doc, root_obj, "cols", jcols);

        yyjson_mut_val *results_arr = yyjson_mut_arr(doc);
        for (int ri = 0; ri < page->output_count; ri++) {
            search_result_t *r = &sr[page->result_start + ri];
            char lines[CBM_SZ_32];
            search_lines_text(r, lines, sizeof(lines));
            yyjson_mut_val *row = yyjson_mut_arr(doc);
            yyjson_mut_arr_add_strcpy(doc, row, r->qualified_name);
            yyjson_mut_arr_add_strcpy(doc, row, r->label);
            yyjson_mut_arr_add_strcpy(doc, row, r->file);
            yyjson_mut_arr_add_strcpy(doc, row, lines);
            yyjson_mut_val *ml = yyjson_mut_arr(doc);
            for (int j = 0; j < r->match_count; j++) {
                yyjson_mut_arr_add_int(doc, ml, r->match_lines[j]);
            }
            yyjson_mut_arr_add_val(row, ml);
            yyjson_mut_arr_add_int(doc, row, r->match_total - r->match_count);
            yyjson_mut_arr_add_int(doc, row, r->in_degree);
            yyjson_mut_arr_add_int(doc, row, r->out_degree);
            if (mode == MODE_FULL || attach_context) {
                yyjson_mut_val *src = yyjson_mut_obj(doc);
                attach_result_source(doc, src, r, mode, context_lines, root_path);
                yyjson_mut_arr_add_val(row, src);
            }
            yyjson_mut_arr_add_val(results_arr, row);
        }
        yyjson_mut_obj_add_val(doc, root_obj, "rows", results_arr);

        yyjson_mut_val *raw_obj = yyjson_mut_obj(doc);
        yyjson_mut_val *rcols = yyjson_mut_arr(doc);
        static const char *const raw_col_names[] = {"file",
                                                    "line",
                                                    "content",
                                                    "content_start_byte",
                                                    "content_returned_bytes",
                                                    "content_total_bytes",
                                                    "match_start_byte",
                                                    "match_end_byte",
                                                    "content_has_more",
                                                    "content_next_offset",
                                                    "match_fully_returned"};
        for (size_t ci = 0; ci < sizeof(raw_col_names) / sizeof(raw_col_names[0]); ci++) {
            yyjson_mut_arr_add_str(doc, rcols, raw_col_names[ci]);
        }
        yyjson_mut_obj_add_val(doc, raw_obj, "cols", rcols);
        yyjson_mut_val *raw_arr = yyjson_mut_arr(doc);
        for (int ri = 0; ri < page->raw_output; ri++) {
            yyjson_mut_val *row = yyjson_mut_arr(doc);
            yyjson_mut_arr_add_strcpy(doc, row, raw[ri].file);
            yyjson_mut_arr_add_int(doc, row, raw[ri].line);
            yyjson_mut_arr_add_strcpy(doc, row, raw[ri].content);
            yyjson_mut_arr_add_uint(doc, row, raw[ri].content_start_byte);
            yyjson_mut_arr_add_uint(doc, row, raw[ri].content_returned_bytes);
            yyjson_mut_arr_add_uint(doc, row, raw[ri].content_total_bytes);
            if (raw[ri].match_known) {
                yyjson_mut_arr_add_uint(doc, row, raw[ri].match_start_byte);
                yyjson_mut_arr_add_uint(doc, row, raw[ri].match_end_byte);
            } else {
                yyjson_mut_arr_add_null(doc, row);
                yyjson_mut_arr_add_null(doc, row);
            }
            yyjson_mut_arr_add_bool(doc, row, raw_content_has_next(&raw[ri]));
            if (raw_content_has_next(&raw[ri]))
                yyjson_mut_arr_add_uint(
                    doc, row, raw[ri].content_start_byte + raw[ri].content_returned_bytes);
            else
                yyjson_mut_arr_add_null(doc, row);
            yyjson_mut_arr_add_bool(doc, row, raw_match_fully_returned(&raw[ri]));
            yyjson_mut_arr_add_val(raw_arr, row);
        }
        yyjson_mut_obj_add_val(doc, raw_obj, "rows", raw_arr);
        yyjson_mut_obj_add_val(doc, root_obj, "raw_matches", raw_obj);
    }

    yyjson_mut_obj_add_val(doc, root_obj, "directories",
                           build_dir_distribution(doc, sr, page->sr_count));

    /* Summary stats */
    yyjson_mut_obj_add_int(doc, root_obj, "total_grep_matches", page->gm_count);
    yyjson_mut_obj_add_int(doc, root_obj, "total_results", page->sr_count);
    yyjson_mut_obj_add_int(doc, root_obj, "raw_match_count", page->raw_count);
    search_page_scalars_json(doc, root_obj, page);
    if (metrics->include_phase_timings) {
        yyjson_mut_obj_add_uint(doc, root_obj, "scope_ms", metrics->scope_ms);
        yyjson_mut_obj_add_uint(doc, root_obj, "scan_ms", metrics->scan_ms);
        yyjson_mut_obj_add_uint(doc, root_obj, "enrich_ms", metrics->enrich_ms);
    }
    yyjson_mut_obj_add_uint(doc, root_obj, "elapsed_ms", metrics->elapsed_ms);
    if (page->sr_count > 0 && page->gm_count > 0) {
        char ratio[CBM_SZ_32];
        snprintf(ratio, sizeof(ratio), "%.1fx",
                 (double)page->gm_count / (double)(page->sr_count + page->raw_count));
        yyjson_mut_obj_add_strcpy(doc, root_obj, "dedup_ratio", ratio);
    }

    /* Warnings: surface common foot-guns instead of leaving them silent. */
    yyjson_mut_val *warnings = yyjson_mut_arr(doc);
    if (warn_literal_pipe) {
        yyjson_mut_arr_add_strcpy(
            doc, warnings,
            "pattern contains '|' but regex=false, so it is matched literally (not as "
            "alternation). Pass regex=true for 'foo|bar' to mean 'foo OR bar'.");
    }
    if (metrics->elapsed_ms >= SEARCH_SLOW_MS) {
        char slow[CBM_SZ_128];
        snprintf(slow, sizeof(slow),
                 "search took %dms (>%ds); narrow file_pattern/path_filter or use a more "
                 "specific pattern",
                 (int)metrics->elapsed_ms, SEARCH_SLOW_MS / 1000);
        yyjson_mut_arr_add_strcpy(doc, warnings, slow);
        char ems[CBM_SZ_32];
        snprintf(ems, sizeof(ems), "%d", (int)metrics->elapsed_ms);
        cbm_log_warn("search.slow", "elapsed_ms", ems); /* visibility in logs */
    }
    if (yyjson_mut_arr_size(warnings) > 0) {
        yyjson_mut_obj_add_val(doc, root_obj, "warnings", warnings);
    }

    char *json = source_doc_to_str(doc);
    yyjson_mut_doc_free(doc);
    return json;
}

/* Read grep output from fp, parse file:line:content format, apply path filter,
 * and return a dynamically-allocated grep_match_t array. */
/* Strip root path prefix from a file path. */
static const char *strip_root_prefix(const char *path, const char *root, size_t root_len) {
    if (strncmp(path, root, root_len) != 0) {
        return path;
    }
    const char *p = path + root_len;
    if (*p == '/') {
        p++;
    }
    return p;
}

static bool source_match_bounds(const char *content, const char *pattern, bool use_regex,
                                const cbm_regex_t *compiled_regex, size_t *start_out,
                                size_t *end_out) {
    if (!content || !pattern || !start_out || !end_out) {
        return false;
    }
    if (!use_regex) {
        const char *match = strstr(content, pattern);
        if (!match) {
            return false;
        }
        *start_out = (size_t)(match - content);
        *end_out = *start_out + strlen(pattern);
        return true;
    }
    if (!compiled_regex) {
        return false;
    }
    cbm_regmatch_t match = {.rm_so = -1, .rm_eo = -1};
    if (cbm_regexec(compiled_regex, content, 1, &match, 0) != CBM_REG_OK || match.rm_so < 0 ||
        match.rm_eo < match.rm_so) {
        return false;
    }
    *start_out = (size_t)match.rm_so;
    *end_out = (size_t)match.rm_eo;
    return true;
}

static size_t source_utf8_sequence_len(const unsigned char *p, const unsigned char *end) {
    size_t remaining = (size_t)(end - p);
    unsigned char c = *p;
    if (c < 0x80) {
        return 1;
    }
    if (c >= 0xC2 && c <= 0xDF && remaining >= 2 && source_utf8_is_cont(p[1])) {
        return 2;
    }
    if (c == 0xE0 && remaining >= 3 && p[1] >= 0xA0 && p[1] <= 0xBF && source_utf8_is_cont(p[2])) {
        return 3;
    }
    if (c >= 0xE1 && c <= 0xEC && remaining >= 3 && source_utf8_is_cont(p[1]) &&
        source_utf8_is_cont(p[2])) {
        return 3;
    }
    if (c == 0xED && remaining >= 3 && p[1] >= 0x80 && p[1] <= 0x9F && source_utf8_is_cont(p[2])) {
        return 3;
    }
    if (c >= 0xEE && c <= 0xEF && remaining >= 3 && source_utf8_is_cont(p[1]) &&
        source_utf8_is_cont(p[2])) {
        return 3;
    }
    if (c == 0xF0 && remaining >= 4 && p[1] >= 0x90 && p[1] <= 0xBF && source_utf8_is_cont(p[2]) &&
        source_utf8_is_cont(p[3])) {
        return 4;
    }
    if (c >= 0xF1 && c <= 0xF3 && remaining >= 4 && source_utf8_is_cont(p[1]) &&
        source_utf8_is_cont(p[2]) && source_utf8_is_cont(p[3])) {
        return 4;
    }
    if (c == 0xF4 && remaining >= 4 && p[1] >= 0x80 && p[1] <= 0x8F && source_utf8_is_cont(p[2]) &&
        source_utf8_is_cont(p[3])) {
        return 4;
    }
    return 0;
}

/* A continuation-shaped byte is not necessarily part of valid UTF-8. Adjust a
 * page boundary only when a complete sequence actually spans it; malformed
 * bytes remain independently addressable original source bytes. */
static size_t source_utf8_page_start(const char *content, size_t total, size_t start) {
    if (start >= total || !source_utf8_is_cont((unsigned char)content[start])) {
        return start;
    }
    size_t earliest = start > 3U ? start - 3U : 0U;
    for (size_t candidate = start; candidate > earliest;) {
        candidate--;
        if (source_utf8_is_cont((unsigned char)content[candidate])) {
            continue;
        }
        size_t sequence = source_utf8_sequence_len((const unsigned char *)content + candidate,
                                                   (const unsigned char *)content + total);
        if (sequence > 1U && candidate + sequence > start) {
            return candidate + sequence;
        }
        break;
    }
    return start;
}

static size_t source_utf8_page_end(const char *content, size_t total, size_t end) {
    if (end >= total || !source_utf8_is_cont((unsigned char)content[end])) {
        return end;
    }
    size_t earliest = end > 3U ? end - 3U : 0U;
    for (size_t candidate = end; candidate > earliest;) {
        candidate--;
        if (source_utf8_is_cont((unsigned char)content[candidate])) {
            continue;
        }
        size_t sequence = source_utf8_sequence_len((const unsigned char *)content + candidate,
                                                   (const unsigned char *)content + total);
        if (sequence > 1U && candidate + sequence > end) {
            return candidate;
        }
        break;
    }
    return end;
}

/* Select one bounded raw-line preview. By default it contains the complete
 * match whenever the match itself fits. An explicit content offset pages the
 * original line independently of the raw-row cursor. Boundaries inside valid
 * UTF-8 code points are adjusted and reported; malformed bytes stay exactly
 * addressable and are reversibly encoded by the shared output boundary. */
static void source_raw_preview(grep_match_t *match, const char *content,
                               bool raw_content_offset_set, size_t raw_content_offset,
                               bool match_known, size_t match_start, size_t match_end) {
    const size_t preview_capacity = sizeof(match->content) - SOURCE_SKIP_ONE;
    size_t total = strlen(content);
    size_t start = 0;
    if (raw_content_offset_set) {
        start = raw_content_offset < total ? raw_content_offset : total;
    } else if (match_known && total > preview_capacity) {
        size_t match_length = match_end - match_start;
        if (match_length >= preview_capacity) {
            start = match_start;
        } else {
            size_t lead = (preview_capacity - match_length) / 2U;
            start = match_start > lead ? match_start - lead : 0;
            if (start + preview_capacity > total) {
                start = total - preview_capacity;
            }
        }
    }
    start = source_utf8_page_start(content, total, start);

    size_t remaining = total - start;
    size_t returned = remaining < preview_capacity ? remaining : preview_capacity;
    size_t end = start + returned;
    if (end < total) {
        end = source_utf8_page_end(content, total, end);
    }
    returned = end - start;
    memcpy(match->content, content + start, returned);
    match->content[returned] = '\0';
    match->content_start_byte = start;
    match->content_returned_bytes = returned;
    match->content_total_bytes = total;
    match->match_start_byte = match_start;
    match->match_end_byte = match_end;
    match->match_known = match_known;
    match->content_truncated = start > 0 || end < total;
}

/* Find the tightest node containing a line in a file. Returns index or -1.
 * Equal spans resolve by qualified name then id so the attribution is stable. */
static int find_tightest_node(cbm_node_t *nodes, int count, int line) {
    int best = CBM_NOT_FOUND;
    int best_span = MAX_LINE_SPAN;
    for (int j = 0; j < count; j++) {
        if (nodes[j].start_line <= line && nodes[j].end_line >= line) {
            int span = nodes[j].end_line - nodes[j].start_line;
            const char *candidate_qn = nodes[j].qualified_name ? nodes[j].qualified_name : "";
            const char *best_qn =
                best >= 0 && nodes[best].qualified_name ? nodes[best].qualified_name : "";
            bool stable_tie_winner =
                span == best_span &&
                (best < 0 || strcmp(candidate_qn, best_qn) < 0 ||
                 (strcmp(candidate_qn, best_qn) == 0 && nodes[j].id < nodes[best].id));
            if (span < best_span || stable_tie_winner) {
                best = j;
                best_span = span;
            }
        }
    }
    return best;
}

/* Add a grep hit to the search result set (merge into existing or create new). */
static bool add_to_search_results(search_result_t **sr, int *sr_count, int *sr_cap, cbm_node_t *n,
                                  int line) {
    for (int j = *sr_count - 1; j >= 0; j--) {
        if ((*sr)[j].node_id == n->id) {
            if ((*sr)[j].match_total < INT_MAX) {
                (*sr)[j].match_total++;
            }
            if ((*sr)[j].match_count < CBM_SZ_64) {
                (*sr)[j].match_lines[(*sr)[j].match_count++] = line;
            }
            return true;
        }
    }
    if (*sr_count >= *sr_cap) {
        int next_capacity = *sr_cap * SOURCE_PAIR_LEN;
        search_result_t *grown = realloc(*sr, (size_t)next_capacity * sizeof(**sr));
        if (!grown) {
            return false;
        }
        memset(grown + *sr_cap, 0, (size_t)(next_capacity - *sr_cap) * sizeof(*grown));
        *sr = grown;
        *sr_cap = next_capacity;
    }
    search_result_t *r = &(*sr)[*sr_count];
    r->node_id = n->id;
    snprintf(r->node_name, sizeof(r->node_name), "%s", n->name ? n->name : "");
    snprintf(r->qualified_name, sizeof(r->qualified_name), "%s",
             n->qualified_name ? n->qualified_name : "");
    snprintf(r->label, sizeof(r->label), "%s", n->label ? n->label : "");
    snprintf(r->file, sizeof(r->file), "%s", n->file_path ? n->file_path : "");
    r->start_line = n->start_line;
    r->end_line = n->end_line;
    r->match_lines[0] = line;
    r->match_count = SOURCE_SKIP_ONE;
    r->match_total = SOURCE_SKIP_ONE;
    (*sr_count)++;
    return true;
}

/* Match a single grep hit to the tightest containing node, then add to sr or
 * raw. Raw hits are counted exactly; only the caller's requested page
 * [raw_offset, raw_offset + raw_limit) is retained. */
static bool classify_grep_hit(const grep_match_t *hit, cbm_node_t *file_nodes, int file_node_count,
                              search_result_t **sr, int *sr_count, int *sr_cap, grep_match_t **raw,
                              int raw_offset, int raw_limit, int *raw_count, int *raw_stored_count,
                              int *raw_cap, int *raw_content_truncated) {
    int best = find_tightest_node(file_nodes, file_node_count, hit->line);
    if (best >= 0) {
        return add_to_search_results(sr, sr_count, sr_cap, &file_nodes[best], hit->line);
    }
    if (*raw_count == INT_MAX) {
        return false;
    }
    int raw_index = (*raw_count)++;
    if (hit->content_truncated) {
        if (*raw_content_truncated == INT_MAX) {
            return false;
        }
        (*raw_content_truncated)++;
    }
    bool retain = raw_limit > 0 && raw_index >= raw_offset && raw_index - raw_offset < raw_limit;
    if (!retain) {
        return true;
    }
    if (*raw_stored_count >= *raw_cap) {
        int next_capacity = (*raw_cap == 0) ? 8 : *raw_cap * SOURCE_PAIR_LEN;
        if (next_capacity > raw_limit) {
            next_capacity = raw_limit;
        }
        grep_match_t *grown = realloc(*raw, (size_t)next_capacity * sizeof(**raw));
        if (!grown) {
            return false;
        }
        *raw = grown;
        *raw_cap = next_capacity;
    }
    (*raw)[(*raw_stored_count)++] = *hit;
    return true;
}

/* Free a file_nodes array returned from cbm_store_find_nodes_by_file. */
static void free_file_nodes(cbm_node_t *nodes, int count) {
    for (int j = 0; j < count; j++) {
        safe_str_free(&nodes[j].project);
        safe_str_free(&nodes[j].label);
        safe_str_free(&nodes[j].name);
        safe_str_free(&nodes[j].qualified_name);
        safe_str_free(&nodes[j].file_path);
        safe_str_free(&nodes[j].properties_json);
    }
    free(nodes);
}

/* Parse and classify the complete grep stream without retaining one object per
 * hit. Each record is read with cbm_getline into a growable buffer, so an
 * over-long record can never split into a fabricated file:line:content
 * fragment. Exact totals are counted while the stream is consumed to EOF. A
 * false return means the stream could not be consumed completely. */
static bool scan_and_classify_grep_matches(
    FILE *fp, const char *root_path, size_t root_len, bool has_path_filter, cbm_regex_t *path_regex,
    const char *pattern, bool use_regex, bool raw_content_offset_set, size_t raw_content_offset,
    cbm_store_t *store, const char *project, search_result_t **sr, int *sr_count, int *sr_cap,
    grep_match_t **raw, int raw_offset, int raw_limit, int *raw_count, int *raw_stored_count,
    int *raw_cap, int *raw_content_truncated, int *grep_count) {
    char *line = NULL;
    size_t line_capacity = 0;
    char current_file[CBM_SZ_512] = "";
    bool have_current_file = false;
    cbm_node_t *file_nodes = NULL;
    int file_node_count = 0;
    cbm_regex_t content_regex;
    bool content_regex_ready = use_regex && pattern &&
                               cbm_regcomp(&content_regex, pattern, CBM_REG_EXTENDED) == CBM_REG_OK;
    bool ok = true;

    for (;;) {
        ssize_t line_length = cbm_getline(&line, &line_capacity, fp);
        if (line_length < 0) {
            if (!feof(fp)) {
                ok = false;
            }
            break;
        }
        size_t len = (size_t)line_length;
        while (len > 0 &&
               (line[len - SOURCE_SKIP_ONE] == '\n' || line[len - SOURCE_SKIP_ONE] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) {
            continue;
        }

        /* PowerShell output uses tab as delimiter (paths may contain colons
         * on Windows, e.g. C:\dir\file). Unix grep uses colon. */
#ifdef _WIN32
        char sep = '\t';
#else
        char sep = ':';
#endif
        char *sep1 = strchr(line, (unsigned char)sep);
        if (!sep1) {
            continue;
        }
        char *sep2 = strchr(sep1 + SOURCE_SKIP_ONE, (unsigned char)sep);
        if (!sep2) {
            continue;
        }
        *sep1 = '\0';
        *sep2 = '\0';
#ifdef _WIN32
        cbm_normalize_path_sep(line);
#endif
        const char *file = strip_root_prefix(line, root_path, root_len);
        if (has_path_filter && cbm_regexec(path_regex, file, 0, NULL, 0) != CBM_REG_OK) {
            continue;
        }
        if (*grep_count == INT_MAX) {
            ok = false;
            break;
        }
        (*grep_count)++;

        if (!have_current_file || strcmp(current_file, file) != 0) {
            free_file_nodes(file_nodes, file_node_count);
            file_nodes = NULL;
            file_node_count = 0;
            snprintf(current_file, sizeof(current_file), "%s", file);
            have_current_file = true;
            if (store) {
                (void)cbm_store_find_nodes_by_file(store, project, file, &file_nodes,
                                                   &file_node_count);
            }
        }

        grep_match_t hit = {0};
        snprintf(hit.file, sizeof(hit.file), "%s", file);
        hit.line = (int)strtol(sep1 + SOURCE_SKIP_ONE, NULL, CBM_DECIMAL_BASE);
        const char *content = sep2 + SOURCE_SKIP_ONE;
        size_t match_start = 0;
        size_t match_end = 0;
        bool match_known = source_match_bounds(content, pattern, use_regex,
                                               content_regex_ready ? &content_regex : NULL,
                                               &match_start, &match_end);
        source_raw_preview(&hit, content, raw_content_offset_set, raw_content_offset, match_known,
                           match_start, match_end);
        if (!classify_grep_hit(&hit, file_nodes, file_node_count, sr, sr_count, sr_cap, raw,
                               raw_offset, raw_limit, raw_count, raw_stored_count, raw_cap,
                               raw_content_truncated)) {
            ok = false;
            break;
        }
    }

    if (content_regex_ready) {
        cbm_regfree(&content_regex);
    }
    free_file_nodes(file_nodes, file_node_count);
    free(line);
    return ok;
}

/* Write indexed file list for scoped grep. Returns true if scoped.
 * When a path_filter is provided, apply it here — before grep — so large
 * indexed projects do not scan files only for collect_grep_matches to discard
 * them later. The predicate is IDENTICAL to the post-grep filter: the same
 * compiled regex run against the same root-relative path (separators
 * normalized on Windows first), so prefiltering can only skip files whose
 * hits would be dropped anyway — results-preserving by construction.
 * *out_written receives the number of records written (0 = the filter
 * excluded every indexed file).
 *
 * `fl` is the caller's already-open binary stream on the descriptor cbm_mkstemp
 * created inside the private scratch directory; this function never opens or
 * closes it, so the list is never reachable through a predictable pathname. */
static bool write_scoped_filelist(cbm_store_t *pre_store, const char *project,
                                  const char *root_path, FILE *fl, const char *file_pattern,
                                  bool has_path_filter, cbm_regex_t *path_regex, int *out_written) {
    *out_written = 0;
    if (!pre_store) {
        return false;
    }
    char **indexed_files = NULL;
    int indexed_count = 0;
    int list_rc = cbm_store_list_files(pre_store, project, &indexed_files, &indexed_count);
    if (list_rc != CBM_STORE_OK || indexed_count == 0) {
        for (int fi = 0; fi < indexed_count; fi++) {
            free(indexed_files[fi]);
        }
        /* An empty successful result still owns the outer allocation. */
        free(indexed_files);
        return false;
    }
    bool ok = false;
    int written = 0;
    if (fl) {
        ok = true;
        for (int fi = 0; fi < indexed_count; fi++) {
            /* A source path never legitimately contains a newline or carriage
             * return. Those bytes are exactly the record separator on the
             * Windows filelist (and would split naive line readers elsewhere),
             * so a crafted indexed path with an embedded newline could inject
             * an extra entry into the scan set. Skip such paths entirely. */
            if (strpbrk(indexed_files[fi], "\r\n") != NULL) {
                continue;
            }
            if (has_path_filter && path_regex) {
#ifdef _WIN32
                cbm_normalize_path_sep(indexed_files[fi]);
#endif
                if (cbm_regexec(path_regex, indexed_files[fi], 0, NULL, 0) != CBM_REG_OK) {
                    continue;
                }
            }
#ifndef _WIN32
            /* GNU grep's --include is unavailable in BusyBox grep. Filter the
             * canonical list before xargs instead, preserving grep's basename
             * glob semantics without making the shipped static binary depend on
             * GNU userland. Windows keeps its PowerShell -like filter. */
            const char *basename = strrchr(indexed_files[fi], '/');
            basename = basename ? basename + SOURCE_SKIP_ONE : indexed_files[fi];
            if (file_pattern && fnmatch(file_pattern, basename, 0) != 0) {
                continue;
            }
#endif
#ifdef _WIN32
            if (cbm_search_code_file_pattern_can_prefilter(file_pattern) &&
                !cbm_search_code_windows_path_matches_prefilter(indexed_files[fi], file_pattern)) {
                continue;
            }
#endif
            size_t root_len = strlen(root_path);
            size_t file_len = strlen(indexed_files[fi]);
            if (root_len > SIZE_MAX - file_len - 2) {
                continue;
            }
            size_t scan_path_len = root_len + 1 + file_len;
            char *scan_path = malloc(scan_path_len + 1);
            if (!scan_path) {
                ok = false;
                break;
            }
            memcpy(scan_path, root_path, root_len);
            scan_path[root_len] = '/';
            memcpy(scan_path + root_len + 1, indexed_files[fi], file_len + 1);

            /* Incremental stores can retain structural directory nodes and
             * briefly stale deleted-file paths. Neither is a content-scan
             * operand. Filter them before spawning so an expected stale entry
             * cannot turn otherwise valid matches into grep status 2. This
             * deliberately does not follow symlinks/reparse points. */
            cbm_path_info_t path_info;
            if (cbm_path_info_utf8(scan_path, &path_info) != 0 || !path_info.is_regular) {
                free(scan_path);
                continue;
            }
            /* Write "<root>/<file>" piece-by-piece (no fixed-size buffer, so an
             * arbitrarily long absolute path cannot overflow). Forward slash join
             * so xargs doesn't treat Windows backslashes as escapes; binary mode
             * (wb) prevents CRLF translation. Record separator differs by platform:
             *   - Unix: NUL, consumed by `xargs -0` — handles spaces in paths (a
             *     newline separator would split plain xargs on the space).
             *   - Windows: newline, consumed by PowerShell `Get-Content |
             *     Select-String -LiteralPath` (NUL bytes break Get-Content). */
            (void)fwrite(scan_path, 1, scan_path_len, fl);
            free(scan_path);
#ifdef _WIN32
            (void)fputc('\n', fl);
#else
            (void)fputc('\0', fl);
#endif
            written++;
        }
        /* The stream stays open — the caller owns it and closes it (flushing
         * these records to disk) before the grep subprocess reads the list. */
    }
    for (int fi = 0; fi < indexed_count; fi++) {
        free(indexed_files[fi]);
    }
    free(indexed_files);
    *out_written = written;
    return ok;
}

/* Parse search mode string (0=compact, 1=full, 2=files). */
static int parse_search_mode(const char *mode_str) {
    if (!mode_str) {
        return 0;
    }
    if (strcmp(mode_str, "full") == 0) {
        return SOURCE_SKIP_ONE;
    }
    if (strcmp(mode_str, "files") == 0) {
        return SOURCE_RETURN_FILES;
    }
    return 0;
}

/* Validate shell-safe arguments for search. */
/* Search/grep paths and globs are ALWAYS single-quoted (POSIX sh) or
 * double-/single-quoted (Windows cmd/PowerShell) on the command line, which
 * neutralises '&' — a very common character in real paths (R&D, "Foo & Bar",
 * OneDrive). Accept '&' here while still rejecting every metacharacter that
 * could break out of the quoting (#272). */
static bool validate_search_path_arg(const char *s) {
    if (!s) {
        return false;
    }
    for (const char *p = s; *p; p++) {
        switch (*p) {
        case '\'':
        case '"':
        case ';':
        case '|':
        case '$':
        case '`':
        case '<':
        case '>':
        case '\n':
        case '\r':
#ifndef _WIN32
        case '\\':
#endif
            return false;
        default:
            break;
        }
    }
    return true;
}

/* These characters retain command-language meaning inside quoted cmd.exe
 * arguments: percent expands environment variables, exclamation can expand
 * delayed variables, and caret changes parsing. Never interpolate them from a
 * stored project root or request branch into the Windows detect_changes payload.
 * /V:OFF is defense in depth for exclamation; validation remains the boundary. */
static __attribute__((unused)) bool validate_windows_cmd_interpolation_arg(const char *s) {
#ifdef _WIN32
    return s && strpbrk(s, "%!^") == NULL;
#else
    return s != NULL;
#endif
}

static bool validate_search_args(const char *root_path, const char *file_pattern) {
    if (!validate_search_path_arg(root_path)) {
        return false;
    }
    if (file_pattern && !validate_search_path_arg(file_pattern)) {
        return false;
    }
    return true;
}

/* Private scratch for one search_code scan: the grep -f pattern file and the
 * scoped file list.
 *
 * Both used to be fixed, guessable paths derived from the pid —
 * "<tmp>/cbm_search_<pid>.pat" and its ".files" companion — opened with a plain
 * fopen. Another local user could pre-plant a symlink at either name and
 * redirect the write; two searches in the same process could also collide on
 * them. Now both live inside a directory created by cbm_mkdtemp (0700 on POSIX,
 * an explicit owner-only DACL on Windows) under an unguessable XXXXXX suffix,
 * and each file is created by cbm_mkstemp — O_CREAT|O_EXCL at mode 0600, so the
 * create fails rather than following anything already at the name. Every write
 * goes through the descriptor cbm_mkstemp returned; neither path is ever
 * reopened by name.
 *
 * Sizing: cbm_mkdtemp copies its expanded result back into `dir`, and its own
 * internal buffer is CBM_SZ_512, so `dir` must be at least that big to receive
 * it. The two file paths are `dir` plus a short basename. */
typedef struct {
    char dir[CBM_SZ_512];
    char pattern_path[CBM_SZ_1K];
    char filelist_path[CBM_SZ_1K];
    FILE *filelist; /* held open for write_scoped_filelist; closed by the caller */
} search_scratch_t;

/* Create <scratch>/<basename>-XXXXXX exclusively and return a stream on the
 * descriptor. On failure `path_out` is emptied so cleanup skips it. */
static FILE *search_scratch_file(const char *dir, const char *basename, char *path_out,
                                 size_t path_sz) {
    path_out[0] = '\0';
    int written = snprintf(path_out, path_sz, "%s/%s-XXXXXX", dir, basename);
    if (written <= 0 || (size_t)written >= path_sz) {
        path_out[0] = '\0';
        return NULL;
    }
    int descriptor = cbm_mkstemp(path_out);
    if (descriptor < 0) {
        path_out[0] = '\0';
        return NULL;
    }
    /* Binary mode: the file list uses an explicit per-platform record separator
     * (NUL for xargs -0, newline for PowerShell) that CRLF translation would
     * corrupt — the same reason the previous code opened it "wb". */
    FILE *stream = source_fdopen(descriptor, "wb");
    if (!stream) {
        (void)source_close(descriptor);
        (void)cbm_unlink(path_out);
        path_out[0] = '\0';
    }
    return stream;
}

/* Anchored cleanup: removes both scratch files and the private directory. Safe
 * to call more than once and on any partially-initialised scratch, so every
 * exit from handle_search_code can call it unconditionally. rmdir succeeding is
 * itself the proof nothing was left inside. */
static void search_scratch_close(search_scratch_t *scratch) {
    if (scratch->filelist) {
        (void)fclose(scratch->filelist);
        scratch->filelist = NULL;
    }
    if (scratch->pattern_path[0] != '\0') {
        (void)cbm_unlink(scratch->pattern_path);
        scratch->pattern_path[0] = '\0';
    }
    if (scratch->filelist_path[0] != '\0') {
        (void)cbm_unlink(scratch->filelist_path);
        scratch->filelist_path[0] = '\0';
    }
    if (scratch->dir[0] != '\0') {
        (void)cbm_rmdir(scratch->dir);
        scratch->dir[0] = '\0';
    }
}

/* Open the scratch directory, write `pattern` to the grep -f file, and leave the
 * file list open for write_scoped_filelist. Returns true on success; on failure
 * everything already created is removed before returning. */
static bool search_scratch_open(search_scratch_t *scratch, const char *pattern) {
    scratch->dir[0] = '\0';
    scratch->pattern_path[0] = '\0';
    scratch->filelist_path[0] = '\0';
    scratch->filelist = NULL;

    int written =
        snprintf(scratch->dir, sizeof(scratch->dir), "%s/cbm-search-XXXXXX", cbm_tmpdir());
    if (written <= 0 || (size_t)written >= sizeof(scratch->dir) || !cbm_mkdtemp(scratch->dir)) {
        scratch->dir[0] = '\0';
        return false;
    }

    FILE *pattern_file = search_scratch_file(scratch->dir, "pat", scratch->pattern_path,
                                             sizeof(scratch->pattern_path));
    if (!pattern_file) {
        search_scratch_close(scratch);
        return false;
    }
    bool ok = fprintf(pattern_file, "%s\n", pattern) >= 0;
    ok = fclose(pattern_file) == 0 && ok;
    if (!ok) {
        search_scratch_close(scratch);
        return false;
    }

    scratch->filelist = search_scratch_file(scratch->dir, "files", scratch->filelist_path,
                                            sizeof(scratch->filelist_path));
    if (!scratch->filelist) {
        search_scratch_close(scratch);
        return false;
    }
    return true;
}

/* Compile a path filter regex. Returns true if compiled successfully. */
static bool compile_path_filter(const char *filter, cbm_regex_t *re) {
    if (!filter || !filter[0]) {
        return false;
    }
    return cbm_regcomp(re, filter, CBM_REG_EXTENDED | CBM_REG_NOSUB) == CBM_REG_OK;
}

static char *search_code_timeout_payload(void) {
    static const char fallback[] = "{\"code\":\"request_timeout\",\"message\":\"search_code scan "
                                   "exceeded its execution deadline\"}";
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root) {
        if (doc)
            yyjson_mut_doc_free(doc);
        return source_strdup(fallback);
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_str(doc, root, "code", "request_timeout");
    yyjson_mut_obj_add_str(doc, root, "message",
                           "search_code scan exceeded its execution deadline");
    char *result = source_doc_to_str(doc);
    yyjson_mut_doc_free(doc);
    return result ? result : source_strdup(fallback);
}

static cbm_operation_result_t search_code_scan_error(
    search_scratch_t *scratch, const char *output_path, bool has_path_filter,
    cbm_regex_t *path_regex, cbm_store_t *store, char *root_path, char *pattern, char *project,
    char *file_pattern, cbm_operation_command_cause_t cause, const char *message) {
    if (output_path && output_path[0]) {
        (void)cbm_unlink(output_path);
    }
    search_scratch_close(scratch);
    if (has_path_filter) {
        cbm_regfree(path_regex);
    }
    if (store)
        cbm_store_close(store);
    free(root_path);
    free(pattern);
    free(project);
    free(file_pattern);
    if (cause == CBM_OPERATION_COMMAND_DEADLINE) {
        char *payload = search_code_timeout_payload();
        return cbm_operation_result_take(payload, true);
    }
    return source_error(message);
}

cbm_operation_result_t cbm_source_search_operation_execute(const char *args,
                                                           const cbm_operation_runtime_t *runtime) {
    char *pattern = source_string_arg(args, "pattern");
    char *project = source_project_arg(args);
    char *file_pattern = source_string_arg(args, "file_pattern");
    char *path_filter = source_string_arg(args, "path_filter");
    char *mode_str = source_string_arg(args, "mode");
    int legacy_limit = source_int_arg(args, "limit", SOURCE_DEFAULT_LIMIT);
    /* #1511: a negative limit flowed straight into the result cap and came back
     * as the reported count ("results: -5"), which reads to an agent as a real
     * answer rather than a rejected argument. The schema now declares
     * minimum:1, but a schema is a request to the client, never a guarantee to
     * the server — clamp here too. */
    if (legacy_limit < 1) {
        legacy_limit = SOURCE_DEFAULT_LIMIT;
    }
    int result_limit = source_int_arg(args, "result_limit", legacy_limit);
    int result_offset = source_int_arg(args, "result_offset", 0);
    if (result_limit < 1) {
        result_limit = SOURCE_DEFAULT_LIMIT;
    } else if (result_limit > SOURCE_MAX_RESULT_LIMIT) {
        result_limit = SOURCE_MAX_RESULT_LIMIT;
    }
    if (result_offset < 0) {
        result_offset = 0;
    }
    int raw_limit = source_int_arg(args, "raw_limit", SOURCE_DEFAULT_RAW_LIMIT);
    if (raw_limit < 0) {
        raw_limit = 0;
    } else if (raw_limit > SOURCE_MAX_RAW_LIMIT) {
        raw_limit = SOURCE_MAX_RAW_LIMIT;
    }
    int raw_offset = source_int_arg(args, "raw_offset", 0);
    if (raw_offset < 0) {
        raw_offset = 0;
    }
    int raw_content_offset_arg = source_int_arg(args, "raw_content_offset", 0);
    if (raw_content_offset_arg < 0) {
        raw_content_offset_arg = 0;
    }
    bool raw_content_offset_set = false;
    {
        yyjson_doc *offset_doc = source_args_doc(args);
        yyjson_val *offset_root = offset_doc ? yyjson_doc_get_root(offset_doc) : NULL;
        yyjson_val *offset_value =
            yyjson_is_obj(offset_root) ? yyjson_obj_get(offset_root, "raw_content_offset") : NULL;
        raw_content_offset_set = offset_value && yyjson_is_int(offset_value);
        if (offset_doc)
            yyjson_doc_free(offset_doc);
    }
    int context_lines = source_int_arg(args, "context", 0);
    bool use_regex = source_bool_arg(args, "regex", false);
    uint64_t search_t0 = cbm_now_ms();
    search_metrics_t metrics = {0};
    metrics.include_phase_timings = source_bool_arg(args, "debug", false);
    /* In literal (non-regex) mode a '|' is matched as a byte, not alternation —
     * a common silent 0-match trap; flagged in the result warnings (#282). */
    bool pat_has_pipe = pattern && strchr(pattern, '|') != NULL;

    int mode = parse_search_mode(mode_str);
    free(mode_str);

    cbm_regex_t path_regex;
    bool has_path_filter = compile_path_filter(path_filter, &path_regex);
    free(path_filter);
    path_filter = NULL;

    if (!pattern) {
        free(project);
        free(file_pattern);
        return source_error("pattern is required");
    }

    /* Project is required */
    if (!project) {
        free(pattern);
        free(file_pattern);
        return source_project_error(NULL, CBM_STORE_OPEN_NOT_FOUND);
    }

    char *root_path = NULL;
    cbm_store_open_status_t open_status = CBM_STORE_OPEN_NOT_FOUND;
    cbm_store_t *store = source_open_store_and_root(project, &root_path, &open_status);
    if (!store) {
        cbm_operation_result_t error = source_project_error(project, open_status);
        free(pattern);
        free(project);
        free(file_pattern);
        return error;
    }

    if (!validate_search_args(root_path, file_pattern)) {
        if (has_path_filter) {
            cbm_regfree(&path_regex);
        }
        cbm_store_close(store);
        free(root_path);
        free(pattern);
        free(project);
        free(file_pattern);
        return source_error("path or file_pattern contains invalid characters");
    }

    /* issue #283: when regex=true, a syntactically invalid pattern (e.g. an
     * unclosed group) makes the underlying grep fail, which the handler would
     * otherwise report as an empty result set — indistinguishable from a
     * legitimate no-match. Validate the user's regex up front and return an
     * explicit error so callers can tell "broken pattern" from "no matches". */
    if (use_regex) {
        cbm_regex_t probe;
        if (cbm_regcomp(&probe, pattern, CBM_REG_EXTENDED | CBM_REG_NOSUB) != CBM_REG_OK) {
            if (has_path_filter) {
                cbm_regfree(&path_regex);
            }
            cbm_store_close(store);
            free(root_path);
            free(pattern);
            free(project);
            free(file_pattern);
            return source_error(
                "invalid regex pattern (regex=true): check for unbalanced (), [], or {}");
        }
        cbm_regfree(&probe);
    }

    /* ── Phase 0.5: Multi-word → regex conversion ───────────── */
    /* If pattern contains whitespace and is not already a regex, convert to a
     * regex that matches all words in order: "foo bar baz" → "foo.*bar.*baz".
     * This avoids requiring the exact phrase as a contiguous substring. */
    if (!use_regex && strchr(pattern, ' ')) {
        size_t plen = strlen(pattern);
        /* Worst case: every char is a space → ".*" between each char */
        char *regex_pat = malloc(plen * 3 + 1);
        if (regex_pat) {
            char *dst = regex_pat;
            const char *src = pattern;
            bool in_space = false;
            while (*src) {
                if (*src == ' ' || *src == '\t') {
                    if (!in_space) {
                        *dst++ = '.';
                        *dst++ = '*';
                        in_space = true;
                    }
                } else {
                    /* Escape regex metacharacters from user input */
                    if (strchr("\\^$.|?*+()[]{}", *src)) {
                        *dst++ = '\\';
                    }
                    *dst++ = *src;
                    in_space = false;
                }
                src++;
            }
            *dst = '\0';
            free(pattern);
            pattern = regex_pat;
            use_regex = true;
        }
    }

    /* ── Phase 1: Grep scan ──────────────────────────────────── */
    uint64_t scan_budget_ms = runtime && runtime->command_timeout_override_set
                                  ? (uint64_t)runtime->command_timeout_override_ms
                                  : SOURCE_SEARCH_SCAN_TIMEOUT_MS;
    uint64_t scan_started_ms = cbm_now_ms();
    uint64_t scan_deadline_ms = UINT64_MAX - scan_started_ms < scan_budget_ms
                                    ? UINT64_MAX
                                    : scan_started_ms + scan_budget_ms;
    bool scan_deadline_latched = false;
    search_scratch_t scratch;
    if (!search_scratch_open(&scratch, pattern)) {
        bool scan_cancelled = cbm_operation_runtime_cancelled(runtime);
        bool scan_timed_out = cbm_now_ms() >= scan_deadline_ms;
        char errmsg[CBM_SZ_256];
        snprintf(errmsg, sizeof(errmsg), "search failed: cannot create temp file (%s)",
                 strerror(errno));
        cbm_store_close(store);
        free(root_path);
        free(pattern);
        free(project);
        free(file_pattern);
        if (scan_cancelled) {
            return source_error("search_code cancelled for this request");
        }
        if (scan_timed_out) {
            char *payload = search_code_timeout_payload();
            return cbm_operation_result_take(payload, true);
        }
        return source_error(errmsg);
    }
    scan_deadline_latched = cbm_now_ms() >= scan_deadline_ms;
    bool scan_cancellation_latched = cbm_operation_runtime_cancelled(runtime);
    const char *tmpfile = scratch.pattern_path;
    const char *filelist = scratch.filelist_path;

    /* No grep-level match limit: the whole stream is consumed and every hit is
     * counted, so totals are exact and no hit is lost to a cap. Only the
     * requested pages are retained. */
    /* Scope grep to indexed files only — avoids scanning vendored/generated code.
     * Query the graph for distinct file paths, write them to a temp file,
     * then use xargs to pass them to grep. Falls back to recursive grep if
     * no indexed files found (project not fully indexed). */
    bool scoped = false;
    int scoped_written = 0;

    uint64_t scope_t0 = metrics.include_phase_timings ? cbm_now_ms() : 0;
    if (!scan_cancellation_latched && !scan_deadline_latched) {
        scoped = write_scoped_filelist(store, project, root_path, scratch.filelist, file_pattern,
                                       has_path_filter, has_path_filter ? &path_regex : NULL,
                                       &scoped_written);
    }
    /* Close before grep runs: this is what flushes the records the helper wrote
     * through the descriptor. Clearing the field hands ownership to
     * search_scratch_close, which still unlinks the file itself. */
    (void)fclose(scratch.filelist);
    scratch.filelist = NULL;
    scan_cancellation_latched =
        scan_cancellation_latched || cbm_operation_runtime_cancelled(runtime);
    scan_deadline_latched = scan_deadline_latched || cbm_now_ms() >= scan_deadline_ms;
    if (metrics.include_phase_timings) {
        metrics.scope_ms = cbm_now_ms() - scope_t0;
    }

    /* Consume and classify the complete stream. Only graph identities, bounded
     * per-result line evidence, and the requested raw page are retained. */
    int sr_cap = CBM_SZ_32;
    int sr_count = 0;
    search_result_t *sr = calloc((size_t)sr_cap, sizeof(*sr));
    int raw_cap = 0;
    int raw_count = 0;
    int raw_stored_count = 0;
    int raw_content_truncated = 0;
    grep_match_t *raw = NULL;
    int gm_count = 0;
    bool scan_ok = sr != NULL;
    uint64_t scan_t0 = metrics.include_phase_timings ? cbm_now_ms() : 0;
    if (scoped && scoped_written == 0 && !scan_cancellation_latched && !scan_deadline_latched) {
        /* The path_filter (or POSIX file_pattern) excluded every indexed file —
         * nothing to scan. Skip the grep subprocess: xargs on an empty filelist
         * is platform-dependent (GNU execs grep once with no operands, BSD
         * skips), and the post-grep filter would drop every hit anyway. */
        search_scratch_close(&scratch);
    } else if (scan_ok) {
        char cmd[CBM_SZ_4K];
        cbm_search_code_build_grep_cmd(cmd, sizeof(cmd), use_regex, scoped, file_pattern, tmpfile,
                                       filelist, root_path);

        char output_path[CBM_SZ_2K] = {0};
        cbm_proc_result_t scan_result = {0};
        size_t scan_output_limit = runtime && runtime->command_output_limit_override
                                       ? runtime->command_output_limit_override
                                       : SOURCE_SEARCH_OUTPUT_MAX;
        const char *scan_command =
            runtime && runtime->command_override ? runtime->command_override : cmd;
        /* Both POSIX commands wrap grep and map its no-match status to 0, so any
         * non-zero exit (a failed find/sort, an unreadable operand, a broken
         * grep) is an incomplete scan and fails closed. */
        cbm_operation_command_cause_t scan_cause = cbm_operation_run_shell_command_bounded(
            runtime, scan_command, output_path, scan_output_limit, scan_deadline_ms, true,
            scan_deadline_latched, false, &scan_result);
        const char *scan_message = NULL; /* COMMAND_DEADLINE renders its own text */
        char limit_message[CBM_SZ_128];
        FILE *fp = NULL;
        if (scan_cause == CBM_OPERATION_COMMAND_SUPERVISION_FAILURE) {
            scan_message = "search failed: process supervision could not quiesce";
        } else if (scan_cause == CBM_OPERATION_COMMAND_CANCELLED) {
            scan_message = "search_code cancelled for this request";
        } else if (scan_cause == CBM_OPERATION_COMMAND_OUTPUT_LIMIT) {
            snprintf(limit_message, sizeof(limit_message),
                     "search failed: output exceeded the %zu-byte safety limit", scan_output_limit);
            scan_message = limit_message;
        } else if (scan_cause == CBM_OPERATION_COMMAND_FAILURE ||
                   scan_cause == CBM_OPERATION_COMMAND_CONTAINED_FAILURE) {
            scan_message = "search failed before the complete result set was scanned: the "
                           "contained command could not complete";
        } else if (scan_cause == CBM_OPERATION_COMMAND_SUCCESS) {
            fp = cbm_fopen(output_path, "rb");
            if (!fp) {
                scan_cause = CBM_OPERATION_COMMAND_FAILURE;
                scan_message = "search failed: contained output could not be read";
            }
        }
        if (scan_cause != CBM_OPERATION_COMMAND_SUCCESS) {
            free(sr);
            return search_code_scan_error(&scratch, output_path, has_path_filter, &path_regex,
                                          store, root_path, pattern, project, file_pattern,
                                          scan_cause, scan_message);
        }
        scan_ok = scan_and_classify_grep_matches(
            fp, root_path, strlen(root_path), has_path_filter, &path_regex, pattern, use_regex,
            raw_content_offset_set, (size_t)raw_content_offset_arg, store, project, &sr, &sr_count,
            &sr_cap, &raw, raw_offset, raw_limit, &raw_count, &raw_stored_count, &raw_cap,
            &raw_content_truncated, &gm_count);
        (void)fclose(fp);
        (void)cbm_unlink(output_path);
        /* Both scratch files and the private directory go here — unlike the old
         * code, the file list is removed even when the scan was not scoped. */
        search_scratch_close(&scratch);
    }
    if (metrics.include_phase_timings) {
        metrics.scan_ms = cbm_now_ms() - scan_t0;
    }
    if (!scan_ok) {
        search_scratch_close(&scratch);
        free(sr);
        free(raw);
        free(root_path);
        free(pattern);
        free(project);
        free(file_pattern);
        if (has_path_filter) {
            cbm_regfree(&path_regex);
        }
        cbm_store_close(store);
        return source_error("search failed before the complete result set was scanned");
    }

    /* ── Phase 2+3: degree expansion + graph ranking ─────────── */
    uint64_t enrich_t0 = metrics.include_phase_timings ? cbm_now_ms() : 0;

    /* Phase 3: batch degree query — ONE query for all results instead of 2×N */
    if (store && sr_count > 0) {
        int64_t *ids = malloc(sr_count * sizeof(int64_t));
        int *in_degs = malloc(sr_count * sizeof(int));
        int *out_degs = malloc(sr_count * sizeof(int));
        if (!ids || !in_degs || !out_degs) {
            free(ids);
            free(in_degs);
            free(out_degs);
            free(sr);
            free(raw);
            free(root_path);
            free(pattern);
            free(project);
            free(file_pattern);
            if (has_path_filter) {
                cbm_regfree(&path_regex);
            }
            cbm_store_close(store);
            return source_error("out of memory");
        }
        for (int j = 0; j < sr_count; j++) {
            ids[j] = sr[j].node_id;
        }
        if (cbm_store_batch_count_degrees(store, ids, sr_count, "CALLS", in_degs, out_degs) ==
            CBM_STORE_OK) {
            for (int j = 0; j < sr_count; j++) {
                sr[j].in_degree = in_degs[j];
                sr[j].out_degree = out_degs[j];
            }
        }
        free(ids);
        free(in_degs);
        free(out_degs);
    }

    /* Compute scores and sort */
    for (int j = 0; j < sr_count; j++) {
        sr[j].score = compute_search_score(&sr[j]);
    }
    if (sr_count > SOURCE_SKIP_ONE) {
        qsort(sr, sr_count, sizeof(search_result_t), search_result_cmp);
    }
    if (metrics.include_phase_timings) {
        metrics.enrich_ms = cbm_now_ms() - enrich_t0;
    }
    metrics.elapsed_ms = cbm_now_ms() - search_t0;

    /* ── Phase 4: Context assembly (extracted helper) ─────────── */

    /* compact mode (default) emits tree tables; format:"json" emits the
     * same model as structured JSON ({cols, rows}; full adds a per-row
     * source cell; files is a plain list). */
    char *sc_format = source_string_arg(args, "format");
    bool sc_legacy_json = sc_format && strcmp(sc_format, "json") == 0;
    free(sc_format);

    search_page_t page = {.sr_count = sr_count,
                          .raw_count = raw_count,
                          .raw_content_truncated = raw_content_truncated,
                          .gm_count = gm_count,
                          .result_start = result_offset < sr_count ? result_offset : sr_count,
                          .result_limit = result_limit,
                          .raw_start = raw_offset < raw_count ? raw_offset : raw_count,
                          .raw_limit = raw_limit,
                          .raw_output = raw_stored_count};
    page.output_count = sr_count - page.result_start;
    if (page.output_count > result_limit) {
        page.output_count = result_limit;
    }

    char *result = NULL;
    bool result_error = false;
    if (mode == 0 && !sc_legacy_json) {
        result = assemble_search_output_toon(sr, raw, &page, pat_has_pipe && !use_regex, &metrics);
    } else {
        result = assemble_search_output(sr, raw, &page, mode, context_lines, root_path,
                                        pat_has_pipe && !use_regex, &metrics);
    }
    result_error = result == NULL;
    if (!result)
        result = source_strdup("out of memory");
    free(sr);
    free(raw);
    free(root_path);
    free(pattern);
    free(project);
    free(file_pattern);
    if (has_path_filter) {
        cbm_regfree(&path_regex);
    }
    cbm_store_close(store);
    return cbm_operation_result_take(result, result_error || result == NULL);
}

/* ── detect_changes ───────────────────────────────────────────── */
