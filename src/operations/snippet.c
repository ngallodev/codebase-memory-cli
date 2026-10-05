#include "operations/json_args.h"
#include "operations/output_budget.h"
#include "operations/result_wire.h"
#include "operations/operation.h"
#include "operations/store_host.h"

#include "foundation/compat_fs.h"
#include "foundation/workspace.h"
#include "store/store.h"
#include "yyjson/yyjson.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    SNIPPET_NEIGHBOR_LIMIT = 20,
    /* Context-bomb guard: a structural node (Module/File) spans its whole file. */
    SNIPPET_MAX_LINES = 500,
    SNIPPET_DEFAULT_LINES = 50,
    SNIPPET_OUTLINE_MIN_SPAN = 200,
    SNIPPET_MEMBER_DEFAULT_LIMIT = 50,
    SNIPPET_MEMBER_MAX_LIMIT = 500,
    SNIPPET_RES_RANK_CALLABLE = 2,
    SNIPPET_RES_RANK_OTHER = 1,
    SNIPPET_RES_RANK_MODULE = 0,
    SNIPPET_RES_LABEL_WEIGHT = 1000000,
};

static char *dup_text(const char *text) {
    if (!text)
        return NULL;
    return cbm_mem_strdup(CBM_MEM_CLASS_OPERATION_ARG, text);
}

static char *string_arg(const char *args, const char *name) {
    yyjson_doc *doc = args ? yyjson_read(args, strlen(args), 0) : NULL;
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    char *copy = value && yyjson_is_str(value) ? dup_text(yyjson_get_str(value)) : NULL;
    if (doc)
        yyjson_doc_free(doc);
    return copy;
}

static int int_arg(const char *args, const char *name, int fallback) {
    return cbm_json_int_arg(args, name, fallback);
}

static bool bool_arg(const char *args, const char *name) {
    yyjson_doc *doc = args ? yyjson_read(args, strlen(args), 0) : NULL;
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    bool result = value && yyjson_is_bool(value) && yyjson_get_bool(value);
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static cbm_operation_result_t json_result(yyjson_mut_doc *doc, bool error) {
    if (!doc)
        return cbm_operation_result_copy("{\"error\":\"result allocation failed\"}", true);
    char *json = cbm_operation_json_write(doc);
    yyjson_mut_doc_free(doc);
    return json ? cbm_operation_result_take(json, error)
                : cbm_operation_result_copy("{\"error\":\"result encoding failed\"}", true);
}

static cbm_operation_result_t error_result(const char *message, const char *hint) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root) {
        if (doc)
            yyjson_mut_doc_free(doc);
        return cbm_operation_result_copy(message ? message : "snippet failed", true);
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "error", message ? message : "snippet failed");
    if (hint)
        yyjson_mut_obj_add_strcpy(doc, root, "hint", hint);
    return json_result(doc, true);
}

static long resolution_score(const cbm_node_t *node) {
    long rank = SNIPPET_RES_RANK_MODULE;
    if (node->label) {
        if (strcmp(node->label, "Function") == 0 || strcmp(node->label, "Method") == 0) {
            rank = SNIPPET_RES_RANK_CALLABLE;
        } else if (strcmp(node->label, "Module") != 0 && strcmp(node->label, "File") != 0) {
            rank = SNIPPET_RES_RANK_OTHER;
        }
    }
    long span = (long)node->end_line - (long)node->start_line;
    if (span < 0)
        span = 0;
    return rank * SNIPPET_RES_LABEL_WEIGHT + span;
}

static bool real_callable(const cbm_node_t *node) {
    return node->label &&
           (strcmp(node->label, "Function") == 0 || strcmp(node->label, "Method") == 0) &&
           node->end_line > node->start_line;
}

static int resolved_node(const cbm_node_t *nodes, int count, bool *ambiguous) {
    *ambiguous = false;
    if (count <= 1)
        return 0;
    int best = 0;
    long score = resolution_score(&nodes[0]);
    for (int i = 1; i < count; ++i) {
        long candidate = resolution_score(&nodes[i]);
        if (candidate > score) {
            score = candidate;
            best = i;
        }
    }
    int top_count = 0;
    int real_count = 0;
    for (int i = 0; i < count; ++i) {
        if (resolution_score(&nodes[i]) == score)
            ++top_count;
        if (real_callable(&nodes[i]))
            ++real_count;
    }
    *ambiguous = top_count > 1 || real_count > 1;
    return best;
}

/* Read lines [start_line, end_line] byte for byte. A fixed line buffer would
 * split a long line and count it twice; invalid UTF-8 is preserved here and
 * encoded losslessly at the output boundary. */
static char *read_lines(const char *path, int start_line, int end_line) {
    FILE *file = cbm_fopen(path, "rb");
    if (!file)
        return NULL;
    size_t capacity = 4096U;
    size_t length = 0U;
    char *buffer = malloc(capacity);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    buffer[0] = '\0';
    int line_number = 1;
    int byte;
    while ((byte = fgetc(file)) != EOF) {
        if (line_number >= start_line && line_number <= end_line) {
            if (length + 2U > capacity) {
                size_t next = capacity * 2U;
                char *grown = realloc(buffer, next);
                if (!grown) {
                    free(buffer);
                    fclose(file);
                    return NULL;
                }
                buffer = grown;
                capacity = next;
            }
            buffer[length++] = (char)byte;
            buffer[length] = '\0';
        }
        if (byte == '\n') {
            if (line_number >= end_line)
                break;
            ++line_number;
        }
    }
    fclose(file);
    if (length == 0U) {
        free(buffer);
        return NULL;
    }
    return buffer;
}

/* Stable member order for outline pages: source order, then identity. */
static int snippet_member_cmp(const void *left, const void *right) {
    const cbm_node_t *a = left;
    const cbm_node_t *b = right;
    if (a->start_line != b->start_line)
        return a->start_line < b->start_line ? -1 : 1;
    if (a->end_line != b->end_line)
        return a->end_line < b->end_line ? -1 : 1;
    int qn_cmp = strcmp(a->qualified_name ? a->qualified_name : "",
                        b->qualified_name ? b->qualified_name : "");
    if (qn_cmp != 0)
        return qn_cmp;
    return a->id < b->id ? -1 : a->id > b->id ? 1 : 0;
}

static cbm_operation_result_t ambiguous_result(const char *input, const cbm_node_t *nodes,
                                               int count) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    yyjson_mut_val *suggestions = doc ? yyjson_mut_arr(doc) : NULL;
    if (!doc || !root || !suggestions) {
        if (doc)
            yyjson_mut_doc_free(doc);
        return error_result("result allocation failed", NULL);
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_str(doc, root, "status", "ambiguous");
    yyjson_mut_obj_add_strcpy(doc, root, "input", input ? input : "");
    for (int i = 0; i < count; ++i) {
        yyjson_mut_val *item = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_strcpy(doc, item, "qualified_name",
                                  nodes[i].qualified_name ? nodes[i].qualified_name : "");
        yyjson_mut_obj_add_strcpy(doc, item, "name", nodes[i].name ? nodes[i].name : "");
        yyjson_mut_obj_add_strcpy(doc, item, "label", nodes[i].label ? nodes[i].label : "");
        yyjson_mut_obj_add_strcpy(doc, item, "file_path",
                                  nodes[i].file_path ? nodes[i].file_path : "");
        yyjson_mut_arr_add_val(suggestions, item);
    }
    yyjson_mut_obj_add_val(doc, root, "suggestions", suggestions);
    yyjson_mut_obj_add_str(
        doc, root, "hint",
        "Choose an exact qualified_name from suggestions, or narrow with search.");
    return json_result(doc, false);
}

/* Serialized size of a candidate response without mutating the live document:
 * cbm_operation_json_write normalizes non-UTF-8 strings in place, so probes run
 * on a deep copy and the real document is serialized exactly once. */
static size_t snippet_probe_len(yyjson_mut_val *root) {
    yyjson_mut_doc *probe = yyjson_mut_doc_new(NULL);
    if (!probe) {
        return (size_t)-1;
    }
    yyjson_mut_val *copy = yyjson_mut_val_mut_copy(probe, root);
    if (!copy) {
        yyjson_mut_doc_free(probe);
        return (size_t)-1;
    }
    yyjson_mut_doc_set_root(probe, copy);
    char *json = cbm_operation_json_write(probe);
    yyjson_mut_doc_free(probe);
    if (!json) {
        return (size_t)-1;
    }
    size_t length = strlen(json);
    free(json);
    return length;
}

/* Whole lines of `source` (NUL-terminated), never splitting a line. */
static int snippet_source_line_count(const char *source) {
    if (!source || !source[0]) {
        return 0;
    }
    int lines = 1;
    for (const char *p = source; *p; p++) {
        if (*p == '\n' && p[1] != '\0') {
            lines++;
        }
    }
    return lines;
}

/* Replace the whole response shape before measuring a source-prefix candidate. */
static void snippet_set_source_prefix(yyjson_mut_doc *doc, yyjson_mut_val *root, const char *source,
                                      int lines, int start_line, int original_end) {
    size_t keep = 0;
    int line = 0;
    for (const char *q = source; lines > 0 && *q; q++) {
        keep++;
        if (*q == '\n' && ++line >= lines)
            break;
    }
    static const char *const keys[] = {"source",          "source_truncated",  "source_clipped",
                                       "next_start_line", "original_end_line", "end_line"};
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        (void)yyjson_mut_obj_remove_key(root, keys[i]);
    yyjson_mut_obj_add_val(doc, root, "source", yyjson_mut_strncpy(doc, source, keep));
    yyjson_mut_obj_add_int(doc, root, "end_line", lines > 0 ? start_line + lines - 1 : start_line);
    yyjson_mut_obj_add_bool(doc, root, "source_truncated", true);
    yyjson_mut_obj_add_bool(doc, root, "source_clipped", true);
    yyjson_mut_obj_add_int(doc, root, "next_start_line", start_line + lines);
    yyjson_mut_obj_add_int(doc, root, "original_end_line", original_end);
}

/* The floor is metadata only: no partial line or identifier is ever emitted. */
static char *snippet_budget_floor(int max_output_tokens) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root) {
        if (doc) {
            yyjson_mut_doc_free(doc);
        }
        return NULL;
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_str(doc, root, "source_mode", "omitted");
    yyjson_mut_obj_add_bool(doc, root, "truncated", true);
    yyjson_mut_obj_add_str(doc, root, "truncation_reason", "output_budget");
    yyjson_mut_obj_add_int(doc, root, "max_output_tokens", max_output_tokens);
    yyjson_mut_obj_add_bool(doc, root, "continuation_requires_higher_budget", true);
    yyjson_mut_obj_add_str(doc, root, "continuation",
                           "raise max_output_tokens; no partial line or identifier was emitted");
    char *json = cbm_operation_json_write(doc);
    yyjson_mut_doc_free(doc);
    return json;
}

static cbm_operation_result_t node_result(cbm_store_t *store, const char *project,
                                          const cbm_node_t *node, const char *match,
                                          bool include_neighbors, const char *args) {
    cbm_project_t project_info = {0};
    if (cbm_store_get_project(store, project, &project_info) != CBM_STORE_OK ||
        !project_info.root_path || !node->file_path) {
        cbm_project_free_fields(&project_info);
        return error_result("indexed source location is unavailable", NULL);
    }
    char abs_path[4096];
    int n = snprintf(abs_path, sizeof(abs_path), "%s%s%s", project_info.root_path,
                     project_info.root_path[strlen(project_info.root_path) - 1U] == '/' ? "" : "/",
                     node->file_path);
    if (n < 0 || (size_t)n >= sizeof(abs_path) ||
        !cbm_path_within_root(project_info.root_path, abs_path)) {
        cbm_project_free_fields(&project_info);
        return error_result("indexed source path escapes project root", NULL);
    }
    int original_start = node->start_line > 0 ? node->start_line : 1;
    /* A one-line symbol legitimately has end == start. Only missing or inverted
     * end metadata is unknown; expanding a valid one-line node would waste output
     * and lie about the continuation range. */
    int original_end =
        node->end_line >= original_start ? node->end_line : original_start + SNIPPET_DEFAULT_LINES;
    char *source_mode = string_arg(args, "source_mode");
    bool explicit_full = source_mode && strcmp(source_mode, "full") == 0;
    bool explicit_outline = source_mode && strcmp(source_mode, "outline") == 0;
    cbm_operation_arg_free(source_mode);
    /* A large container (file/module/class) answers with its member outline by
     * default; the whole source stays one source_mode=full call away. */
    bool container =
        node->label && (strcmp(node->label, "File") == 0 || strcmp(node->label, "Module") == 0 ||
                        strcmp(node->label, "Class") == 0 || strcmp(node->label, "Interface") == 0);
    bool outline = explicit_outline || (!explicit_full && container &&
                                        original_end - original_start >= SNIPPET_OUTLINE_MIN_SPAN);
    int start_line = outline ? original_start : int_arg(args, "start_line", original_start);
    if (start_line < original_start || start_line > original_end)
        start_line = original_start;
    int end_line = original_end;
    int max_lines = int_arg(args, "max_lines", 0);
    if (max_lines > SNIPPET_MAX_LINES)
        max_lines = SNIPPET_MAX_LINES;
    if (!outline && max_lines > 0 && start_line + max_lines - 1 < end_line)
        end_line = start_line + max_lines - 1;
    bool snippet_clipped = false;
    if (!outline && end_line - start_line + 1 > SNIPPET_MAX_LINES) {
        end_line = start_line + SNIPPET_MAX_LINES - 1;
    }
    if (!outline && end_line < original_end)
        snippet_clipped = true;
    char *source = outline ? NULL : read_lines(abs_path, start_line, end_line);
    if (!outline && !source) {
        cbm_project_free_fields(&project_info);
        return error_result("source file could not be read",
                            "The index may be stale. Re-index or inspect the file directly.");
    }
    int max_output_tokens = cbm_output_budget_tokens(args, 0);
    size_t byte_budget = cbm_output_budget_bytes(max_output_tokens);

    /* Outline page state, hoisted so the byte ceiling can drop whole member
     * rows in place and still report an exact continuation. */
    yyjson_mut_val *member_rows = NULL;
    int member_offset_used = 0;
    int member_total = 0;
    int members_returned = 0;

    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root) {
        if (doc)
            yyjson_mut_doc_free(doc);
        free(source);
        cbm_project_free_fields(&project_info);
        return error_result("result allocation failed", NULL);
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "project", project);
    yyjson_mut_obj_add_strcpy(doc, root, "qualified_name",
                              node->qualified_name ? node->qualified_name : "");
    yyjson_mut_obj_add_strcpy(doc, root, "name", node->name ? node->name : "");
    yyjson_mut_obj_add_strcpy(doc, root, "label", node->label ? node->label : "");
    yyjson_mut_obj_add_strcpy(doc, root, "file_path", node->file_path ? node->file_path : "");
    yyjson_mut_obj_add_int(doc, root, "start_line", start_line);
    yyjson_mut_obj_add_int(doc, root, "end_line", end_line);
    if (snippet_clipped) {
        yyjson_mut_obj_add_bool(doc, root, "source_truncated", true);
        yyjson_mut_obj_add_bool(doc, root, "source_clipped", true); /* compatibility */
        yyjson_mut_obj_add_int(doc, root, "next_start_line", end_line + 1);
        yyjson_mut_obj_add_int(doc, root, "original_end_line", original_end);
    }
    if (outline) {
        cbm_node_t *members = NULL;
        int member_count = 0;
        (void)cbm_store_find_nodes_by_file(store, project, node->file_path, &members,
                                           &member_count);
        if (member_count > 1)
            qsort(members, (size_t)member_count, sizeof(*members), snippet_member_cmp);
        int member_limit = int_arg(args, "member_limit", SNIPPET_MEMBER_DEFAULT_LIMIT);
        int member_offset = int_arg(args, "member_offset", 0);
        if (member_limit < 1)
            member_limit = 1;
        else if (member_limit > SNIPPET_MEMBER_MAX_LIMIT)
            member_limit = SNIPPET_MEMBER_MAX_LIMIT;
        if (member_offset < 0)
            member_offset = 0;
        int eligible = 0;
        for (int i = 0; i < member_count; ++i)
            if (members[i].id != node->id && members[i].start_line >= original_start &&
                members[i].end_line <= original_end)
                ++eligible;
        member_rows = yyjson_mut_arr(doc);
        int seen = 0;
        int emitted = 0;
        for (int i = 0; i < member_count && emitted < member_limit; ++i) {
            if (members[i].id == node->id || members[i].start_line < original_start ||
                members[i].end_line > original_end)
                continue;
            if (seen++ < member_offset)
                continue;
            yyjson_mut_val *member = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_strcpy(doc, member, "qualified_name",
                                      members[i].qualified_name ? members[i].qualified_name : "");
            yyjson_mut_obj_add_strcpy(doc, member, "label",
                                      members[i].label ? members[i].label : "");
            yyjson_mut_obj_add_int(doc, member, "start_line", members[i].start_line);
            yyjson_mut_obj_add_int(doc, member, "end_line", members[i].end_line);
            yyjson_mut_arr_add_val(member_rows, member);
            ++emitted;
        }
        member_offset_used = member_offset;
        member_total = eligible;
        members_returned = emitted;
        yyjson_mut_obj_add_str(doc, root, "source_mode", "outline");
        yyjson_mut_obj_add_val(doc, root, "members", member_rows);
        yyjson_mut_obj_add_int(doc, root, "members_total", eligible);
        yyjson_mut_obj_add_int(doc, root, "members_returned", emitted);
        yyjson_mut_obj_add_bool(doc, root, "members_has_more", member_offset + emitted < eligible);
        if (member_offset + emitted < eligible)
            yyjson_mut_obj_add_int(doc, root, "next_member_offset", member_offset + emitted);
        yyjson_mut_obj_add_bool(doc, root, "full_source_available", true);
        cbm_store_free_nodes(members, member_count);
    } else {
        yyjson_mut_obj_add_str(doc, root, "source_mode", "full");
        yyjson_mut_obj_add_strcpy(doc, root, "source", source);
    }
    if (match)
        yyjson_mut_obj_add_strcpy(doc, root, "match", match);

    /* Preserve the coverage warning promised by the public snippet contract.
     * The graph may contain a callable from a file whose parse included
     * ERROR/MISSING regions; callers need that signal before treating the
     * snippet as complete evidence. */
    cbm_coverage_row_t *coverage_rows = NULL;
    int coverage_count = 0;
    if (cbm_store_coverage_get_path(store, project, node->file_path, &coverage_rows,
                                    &coverage_count) == CBM_STORE_OK) {
        for (int i = 0; i < coverage_count; ++i) {
            if (!coverage_rows[i].rel_path || !node->file_path ||
                strcmp(coverage_rows[i].rel_path, node->file_path) != 0 || !coverage_rows[i].kind)
                continue;
            if (strcmp(coverage_rows[i].kind, "parse_unusable") == 0) {
                yyjson_mut_obj_add_str(
                    doc, root, "coverage_note",
                    "The parse of this file failed across nearly the whole of it, so most "
                    "constructs are missing from the graph and naming line ranges would not "
                    "help. Read the source directly; the source above is ground truth. "
                    "(best-effort signal)");
                break;
            }
            if (strcmp(coverage_rows[i].kind, "parse_partial") == 0) {
                yyjson_mut_obj_add_str(
                    doc, root, "coverage_note",
                    "This file was only PARTIALLY indexed; read the source directly when graph "
                    "coverage matters for completeness.");
                break;
            }
        }
    }
    cbm_store_free_coverage(coverage_rows, coverage_count);

    if (include_neighbors) {
        char **callers = NULL;
        char **callees = NULL;
        int caller_count = 0;
        int callee_count = 0;
        if (cbm_store_node_neighbor_names(store, node->id, SNIPPET_NEIGHBOR_LIMIT, &callers,
                                          &caller_count, &callees, &callee_count) == CBM_STORE_OK) {
            yyjson_mut_val *caller_values = yyjson_mut_arr(doc);
            yyjson_mut_val *callee_values = yyjson_mut_arr(doc);
            for (int i = 0; i < caller_count; ++i) {
                yyjson_mut_arr_add_strcpy(doc, caller_values, callers[i] ? callers[i] : "");
                free(callers[i]);
            }
            for (int i = 0; i < callee_count; ++i) {
                yyjson_mut_arr_add_strcpy(doc, callee_values, callees[i] ? callees[i] : "");
                free(callees[i]);
            }
            free(callers);
            free(callees);
            yyjson_mut_obj_add_val(doc, root, "callers", caller_values);
            yyjson_mut_obj_add_val(doc, root, "callees", callee_values);
        }
    }

    if (byte_budget > 0 && snippet_probe_len(root) > byte_budget) {
        yyjson_mut_obj_add_str(doc, root, "truncation_reason", "output_budget");
        if (outline && member_rows && members_returned > 0) {
            /* An outline is already the quality-preserving alternative; page
             * whole member rows before falling back to the floor. */
            while (snippet_probe_len(root) > byte_budget && members_returned > 0) {
                members_returned--;
                (void)yyjson_mut_arr_remove(member_rows, (size_t)members_returned);
                (void)yyjson_mut_obj_remove_key(root, "members_returned");
                yyjson_mut_obj_add_int(doc, root, "members_returned", members_returned);
                (void)yyjson_mut_obj_remove_key(root, "members_has_more");
                yyjson_mut_obj_add_bool(doc, root, "members_has_more",
                                        member_offset_used + members_returned < member_total);
                (void)yyjson_mut_obj_remove_key(root, "next_member_offset");
                if (member_offset_used + members_returned < member_total) {
                    yyjson_mut_obj_add_int(doc, root, "next_member_offset",
                                           member_offset_used + members_returned);
                }
            }
        } else if (!outline && source) {
            /* Binary-search the largest whole-line source prefix that fits. */
            int available = snippet_source_line_count(source);
            int low = 0;
            int high = available;
            int best_lines = -1;
            while (low <= high) {
                int middle = low + (high - low) / 2;
                snippet_set_source_prefix(doc, root, source, middle, start_line, original_end);
                if (snippet_probe_len(root) <= byte_budget) {
                    best_lines = middle;
                    low = middle + 1;
                } else {
                    high = middle - 1;
                }
            }
            if (best_lines >= 0) {
                snippet_set_source_prefix(doc, root, source, best_lines, start_line, original_end);
            }
        }
        if (snippet_probe_len(root) > byte_budget) {
            free(source);
            cbm_project_free_fields(&project_info);
            yyjson_mut_doc_free(doc);
            char *floor = snippet_budget_floor(max_output_tokens);
            return floor ? cbm_operation_result_take(floor, false)
                         : error_result("out of memory", NULL);
        }
    }
    {
        char *json = cbm_operation_json_write(doc);
        yyjson_mut_doc_free(doc);
        free(source);
        cbm_project_free_fields(&project_info);
        return json ? cbm_operation_result_take(json, false)
                    : error_result("result encoding failed", NULL);
    }
}

cbm_operation_result_t cbm_snippet_operation_execute(const char *args) {
    char *project = string_arg(args, "project");
    char *qualified_name = string_arg(args, "qualified_name");
    bool include_neighbors = bool_arg(args, "include_neighbors");
    if (!project || !project[0]) {
        cbm_operation_arg_free(project);
        cbm_operation_arg_free(qualified_name);
        return error_result("project is required", "Run the command from an indexed repository.");
    }
    if (!qualified_name || !qualified_name[0]) {
        cbm_operation_arg_free(project);
        cbm_operation_arg_free(qualified_name);
        return error_result("qualified_name is required", "Use search first to discover a symbol.");
    }
    cbm_store_open_status_t open_status = CBM_STORE_OPEN_OK;
    cbm_store_t *store = cbm_store_host_open_query(project, &open_status);
    if (!store) {
        cbm_operation_arg_free(project);
        cbm_operation_arg_free(qualified_name);
        if (open_status == CBM_STORE_OPEN_CORRUPT)
            return error_result(CBM_STORE_CORRUPT_MESSAGE, CBM_STORE_CORRUPT_HINT);
        return error_result("project not indexed", "Run 'codebase-memory-cli index .' first.");
    }

    cbm_node_t exact = {0};
    if (cbm_store_find_node_by_qn(store, project, qualified_name, &exact) == CBM_STORE_OK) {
        cbm_operation_result_t result =
            node_result(store, project, &exact, NULL, include_neighbors, args);
        cbm_node_free_fields(&exact);
        cbm_store_close(store);
        cbm_operation_arg_free(project);
        cbm_operation_arg_free(qualified_name);
        return result;
    }

    cbm_node_t *matches = NULL;
    int count = 0;
    const char *method = "base";
    cbm_store_find_nodes_by_qn_base(store, project, qualified_name, false, &matches, &count);
    if (count == 0) {
        cbm_store_free_nodes(matches, count);
        matches = NULL;
        method = "suffix";
        cbm_store_find_nodes_by_qn_suffix(store, project, qualified_name, &matches, &count);
    }
    if (count == 0) {
        cbm_store_free_nodes(matches, count);
        matches = NULL;
        method = "base_suffix";
        cbm_store_find_nodes_by_qn_base(store, project, qualified_name, true, &matches, &count);
    }
    if (count > 0) {
        bool ambiguous = false;
        int selected = resolved_node(matches, count, &ambiguous);
        cbm_operation_result_t result = ambiguous ? ambiguous_result(qualified_name, matches, count)
                                                  : node_result(store, project, &matches[selected],
                                                                method, include_neighbors, args);
        cbm_store_free_nodes(matches, count);
        cbm_store_close(store);
        cbm_operation_arg_free(project);
        cbm_operation_arg_free(qualified_name);
        return result;
    }
    cbm_store_free_nodes(matches, count);
    cbm_store_close(store);
    cbm_operation_arg_free(project);
    cbm_operation_arg_free(qualified_name);
    return error_result(
        "symbol not found",
        "Use 'codebase-memory-cli search <term>' first, then pass an exact qualified_name.");
}
