#include "operations/output_budget.h"
#include "operations/result_wire.h"
#include "operations/trace.h"
#include "operations/store_host.h"

#include "foundation/limits.h"
#include "store/store.h"
#include "yyjson/yyjson.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    TRACE_DEFAULT_DEPTH = 3,
    TRACE_DEFAULT_LIMIT = 100,
    TRACE_MAX_LIMIT = 5000,
    TRACE_MAX_EDGE_TYPES = 16,
    TRACE_RES_CALLABLE = 2,
    TRACE_RES_OTHER = 1,
    TRACE_RES_MODULE = 0,
    TRACE_RES_WEIGHT = 1000000,
};

typedef struct trace_cursor {
    char leg;
    char generation[96];
    uint64_t qhash;
    int hop;
    int64_t node_id;
} trace_cursor_t;

static char *copy_text(const char *text) {
    if (!text)
        return NULL;
    size_t len = strlen(text);
    char *copy = malloc(len + 1U);
    if (copy)
        memcpy(copy, text, len + 1U);
    return copy;
}

static yyjson_doc *args_doc(const char *args) {
    return args ? yyjson_read(args, strlen(args), 0) : NULL;
}

static char *string_arg(const char *args, const char *name) {
    yyjson_doc *doc = args_doc(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    char *result = value && yyjson_is_str(value) ? copy_text(yyjson_get_str(value)) : NULL;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static int int_arg(const char *args, const char *name, int fallback) {
    yyjson_doc *doc = args_doc(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    int result = value && yyjson_is_int(value) ? (int)yyjson_get_sint(value) : fallback;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static bool bool_arg(const char *args, const char *name) {
    yyjson_doc *doc = args_doc(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    bool result = value && yyjson_is_bool(value) && yyjson_get_bool(value);
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static char *doc_to_str(yyjson_mut_doc *doc) {
    if (!doc) {
        return NULL;
    }
    char *json = cbm_operation_json_write(doc);
    yyjson_mut_doc_free(doc);
    return json;
}

/* The optional evidence/args columns are presentation detail: they yield to the
 * byte ceiling before any graph row does, and the response says so. */
static void emit_omitted_optional_fields(yyjson_mut_doc *doc, yyjson_mut_val *root,
                                         bool risk_labels, bool data_flow, bool include_evidence) {
    yyjson_mut_val *fields = yyjson_mut_arr(doc);
    if (risk_labels) {
        yyjson_mut_arr_add_str(doc, fields, "risk_labels");
    }
    if (data_flow) {
        yyjson_mut_arr_add_str(doc, fields, "data_flow");
    }
    if (include_evidence) {
        yyjson_mut_arr_add_str(doc, fields, "include_evidence");
    }
    yyjson_mut_obj_add_bool(doc, root, "optional_fields_omitted", true);
    yyjson_mut_obj_add_val(doc, root, "omitted_optional_fields", fields);
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
        return cbm_operation_result_copy(message ? message : "trace failed", true);
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "error", message ? message : "trace failed");
    if (hint)
        yyjson_mut_obj_add_strcpy(doc, root, "hint", hint);
    return json_result(doc, true);
}

static bool is_test_file(const char *path) {
    if (!path)
        return false;
    return strstr(path, "/test") != NULL || strstr(path, "test_") != NULL ||
           strstr(path, "_test.") != NULL || strstr(path, "/tests/") != NULL ||
           strstr(path, "/spec/") != NULL || strstr(path, ".test.") != NULL ||
           strncmp(path, "tests/", 6U) == 0 || strncmp(path, "test/", 5U) == 0 ||
           strncmp(path, "spec/", 5U) == 0 || strncmp(path, "__tests__/", 10U) == 0;
}

static long resolution_score(const cbm_node_t *node) {
    long rank = TRACE_RES_MODULE;
    if (node->label) {
        if (strcmp(node->label, "Function") == 0 || strcmp(node->label, "Method") == 0) {
            rank = TRACE_RES_CALLABLE;
        } else if (strcmp(node->label, "Module") != 0 && strcmp(node->label, "File") != 0) {
            rank = TRACE_RES_OTHER;
        }
    }
    long span = (long)node->end_line - (long)node->start_line;
    if (span < 0)
        span = 0;
    return rank * TRACE_RES_WEIGHT + span;
}

static bool real_callable(const cbm_node_t *node) {
    return node->label &&
           (strcmp(node->label, "Function") == 0 || strcmp(node->label, "Method") == 0) &&
           node->end_line > node->start_line;
}

static bool nodes_ambiguous(const cbm_node_t *nodes, int count) {
    if (count <= 1)
        return false;
    long best = resolution_score(&nodes[0]);
    for (int i = 1; i < count; ++i) {
        long score = resolution_score(&nodes[i]);
        if (score > best)
            best = score;
    }
    int top = 0;
    int real = 0;
    for (int i = 0; i < count; ++i) {
        if (resolution_score(&nodes[i]) == best)
            ++top;
        if (real_callable(&nodes[i]))
            ++real;
    }
    return top > 1 || real > 1;
}

static cbm_operation_result_t ambiguous_result(const char *input, const cbm_node_t *nodes,
                                               int count) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    yyjson_mut_val *items = doc ? yyjson_mut_arr(doc) : NULL;
    if (!doc || !root || !items) {
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
        yyjson_mut_obj_add_strcpy(doc, item, "label", nodes[i].label ? nodes[i].label : "");
        yyjson_mut_obj_add_strcpy(doc, item, "file_path",
                                  nodes[i].file_path ? nodes[i].file_path : "");
        yyjson_mut_arr_add_val(items, item);
    }
    yyjson_mut_obj_add_val(doc, root, "suggestions", items);
    yyjson_mut_obj_add_str(
        doc, root, "hint",
        "Choose an exact qualified_name from suggestions, or narrow with search.");
    return json_result(doc, false);
}

static int hop_id_cmp(const void *left, const void *right) {
    const cbm_node_hop_t *a = left;
    const cbm_node_hop_t *b = right;
    if (a->hop != b->hop)
        return a->hop < b->hop ? -1 : 1;
    if (a->node.id != b->node.id)
        return a->node.id < b->node.id ? -1 : 1;
    return 0;
}

static bool grow_visited(cbm_traverse_result_t *out, int *capacity) {
    if (out->visited_count < *capacity)
        return true;
    int next = *capacity ? *capacity * 2 : 8;
    cbm_node_hop_t *grown = realloc(out->visited, (size_t)next * sizeof(*grown));
    if (!grown)
        return false;
    out->visited = grown;
    *capacity = next;
    return true;
}

static bool grow_edges(cbm_traverse_result_t *out, int *capacity) {
    if (out->edge_count < *capacity)
        return true;
    int next = *capacity ? *capacity * 2 : 8;
    cbm_edge_info_t *grown = realloc(out->edges, (size_t)next * sizeof(*grown));
    if (!grown)
        return false;
    out->edges = grown;
    *capacity = next;
    return true;
}

/* Union the traversals of every same-name seed. Edge properties are skipped for
 * lean traces (edge_limit 0) or merged under an explicit edge_limit for
 * data-flow/evidence output; saturation of either bound is reported through
 * out->truncated / out->edges_truncated, never silently. cbm_store_bfs_multi()
 * returns only nodes, so the individual traversals are unioned here. */
static bool bfs_union(cbm_store_t *store, const cbm_node_t *seeds, int seed_count,
                      const char *direction, const char **edge_types, int edge_type_count,
                      int depth, int limit, int edge_limit, cbm_traverse_result_t *out) {
    memset(out, 0, sizeof(*out));
    int vcap = 0;
    int ecap = 0;
    for (int seed = 0; seed < seed_count; ++seed) {
        cbm_traverse_result_t current = {0};
        if (cbm_store_bfs_with_edge_limit(store, seeds[seed].id, direction, edge_types,
                                          edge_type_count, depth, limit, edge_limit,
                                          &current) != CBM_STORE_OK) {
            cbm_store_traverse_free(&current);
            cbm_store_traverse_free(out);
            return false;
        }
        out->truncated = out->truncated || current.truncated;
        out->edges_truncated = out->edges_truncated || current.edges_truncated;
        for (int i = 0; i < current.visited_count; ++i) {
            int existing = -1;
            for (int j = 0; j < out->visited_count; ++j) {
                if (out->visited[j].node.id == current.visited[i].node.id) {
                    existing = j;
                    break;
                }
            }
            if (existing >= 0) {
                if (current.visited[i].hop < out->visited[existing].hop)
                    out->visited[existing].hop = current.visited[i].hop;
                continue;
            }
            if (out->visited_count >= limit) {
                out->truncated = true;
                continue;
            }
            if (!grow_visited(out, &vcap)) {
                cbm_store_traverse_free(&current);
                cbm_store_traverse_free(out);
                return false;
            }
            out->visited[out->visited_count++] = current.visited[i];
            memset(&current.visited[i], 0, sizeof(current.visited[i]));
        }
        for (int i = 0; i < current.edge_count; ++i) {
            bool duplicate = false;
            for (int j = 0; j < out->edge_count; ++j) {
                const char *a = out->edges[j].type;
                const char *b = current.edges[i].type;
                if (out->edges[j].source_id == current.edges[i].source_id &&
                    out->edges[j].target_id == current.edges[i].target_id &&
                    ((!a && !b) || (a && b && strcmp(a, b) == 0))) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate)
                continue;
            if (edge_limit > 0 && out->edge_count >= edge_limit) {
                out->edges_truncated = true;
                continue;
            }
            if (!grow_edges(out, &ecap)) {
                cbm_store_traverse_free(&current);
                cbm_store_traverse_free(out);
                return false;
            }
            out->edges[out->edge_count++] = current.edges[i];
            memset(&current.edges[i], 0, sizeof(current.edges[i]));
        }
        cbm_store_traverse_free(&current);
    }
    if (out->visited_count > 1)
        qsort(out->visited, (size_t)out->visited_count, sizeof(*out->visited), hop_id_cmp);
    return true;
}

/* Filtering belongs before page-window calculation: hidden test rows must not
 * consume the caller's visible limit or become cursor watermarks. Compact the
 * owned traversal array in place, preserving canonical (hop,id) order. */
static void filter_test_rows(cbm_traverse_result_t *result) {
    int write_index = 0;
    for (int read_index = 0; read_index < result->visited_count; ++read_index) {
        if (is_test_file(result->visited[read_index].node.file_path)) {
            cbm_node_free_fields(&result->visited[read_index].node);
            continue;
        }
        if (write_index != read_index) {
            result->visited[write_index] = result->visited[read_index];
            memset(&result->visited[read_index], 0, sizeof(result->visited[read_index]));
        }
        ++write_index;
    }
    result->visited_count = write_index;
}

static yyjson_doc *resolve_edge_types(const char *args, const char *mode, const char **types,
                                      int *count) {
    static const char *calls[] = {"CALLS"};
    static const char *data_flow[] = {"CALLS", "DATA_FLOWS"};
    static const char *cross_service[] = {
        "HTTP_CALLS",          "ASYNC_CALLS",       "DATA_FLOWS",    "CALLS",
        "CROSS_HTTP_CALLS",    "CROSS_ASYNC_CALLS", "CROSS_CHANNEL", "CROSS_GRPC_CALLS",
        "CROSS_GRAPHQL_CALLS", "CROSS_TRPC_CALLS"};
    *count = 0;
    yyjson_doc *doc = args_doc(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *array = yyjson_is_obj(root) ? yyjson_obj_get(root, "edge_types") : NULL;
    if (yyjson_is_arr(array)) {
        size_t index, max;
        yyjson_val *value;
        yyjson_arr_foreach(array, index, max, value) {
            if (yyjson_is_str(value) && *count < TRACE_MAX_EDGE_TYPES)
                types[(*count)++] = yyjson_get_str(value);
        }
    }
    if (*count > 0)
        return doc;
    if (doc)
        yyjson_doc_free(doc);
    const char **defaults = calls;
    int n = 1;
    if (mode && strcmp(mode, "data_flow") == 0) {
        defaults = data_flow;
        n = 2;
    } else if (mode && strcmp(mode, "cross_service") == 0) {
        defaults = cross_service;
        n = (int)(sizeof(cross_service) / sizeof(cross_service[0]));
    }
    for (int i = 0; i < n; ++i)
        types[i] = defaults[i];
    *count = n;
    return NULL;
}

static uint64_t fnv1a(const char *text, uint64_t hash) {
    while (text && *text) {
        hash ^= (uint64_t)(unsigned char)*text++;
        hash *= UINT64_C(0x100000001b3);
    }
    return hash;
}

static uint64_t params_hash(const char *project, const char *function, const char *direction,
                            const char *mode, const char *parameter_name, int depth,
                            bool include_tests, bool risk_labels, bool include_evidence, int limit,
                            const char *args) {
    uint64_t hash = UINT64_C(0xcbf29ce484222325);
    hash = fnv1a(project ? project : "", hash);
    hash = fnv1a("|", hash);
    hash = fnv1a(function ? function : "", hash);
    hash = fnv1a("|", hash);
    hash = fnv1a(direction ? direction : "", hash);
    hash = fnv1a("|", hash);
    hash = fnv1a(mode ? mode : "", hash);
    hash = fnv1a("|", hash);
    hash = fnv1a(parameter_name ? parameter_name : "", hash);
    char numbers[64];
    (void)snprintf(numbers, sizeof(numbers), "|%d|%d|%d|%d|%d", depth, include_tests ? 1 : 0,
                   risk_labels ? 1 : 0, include_evidence ? 1 : 0, limit);
    hash = fnv1a(numbers, hash);
    /* Explicit edge types define the traversed graph. Omitting them would let a
     * token minted for CALLS resume an IMPORTS traversal at an unrelated
     * watermark. Caller order is preserved: cursors require unchanged args. */
    yyjson_doc *doc = args_doc(args);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *types = yyjson_is_obj(root) ? yyjson_obj_get(root, "edge_types") : NULL;
    hash = fnv1a("|edge_types=", hash);
    if (yyjson_is_arr(types)) {
        size_t index, max;
        yyjson_val *value;
        yyjson_arr_foreach(types, index, max, value) {
            hash = fnv1a(yyjson_is_str(value) ? yyjson_get_str(value) : "<non-string>", hash);
            hash = fnv1a(";", hash);
        }
    }
    if (doc)
        yyjson_doc_free(doc);
    return hash;
}

static void cursor_encode(const trace_cursor_t *cursor, char *buffer, size_t size) {
    (void)snprintf(buffer, size, "c1.%c.%s.%016llx.%d.%lld", cursor->leg, cursor->generation,
                   (unsigned long long)cursor->qhash, cursor->hop, (long long)cursor->node_id);
}

static const char *cursor_decode(const char *token, const char *generation, uint64_t expected,
                                 trace_cursor_t *out) {
    memset(out, 0, sizeof(*out));
    if (!token || strncmp(token, "c1.", 3U) != 0)
        return "invalid_cursor";
    const char *p = token + 3;
    if ((*p != 'o' && *p != 'i') || p[1] != '.')
        return "invalid_cursor";
    out->leg = *p;
    p += 2;
    const char *end = strchr(p, '.');
    if (!end || end == p || (size_t)(end - p) >= sizeof(out->generation))
        return "invalid_cursor";
    memcpy(out->generation, p, (size_t)(end - p));
    out->generation[end - p] = '\0';
    const char *hash_start = end + 1;
    const char *hash_end = strchr(hash_start, '.');
    if (!hash_end || hash_end - hash_start != 16)
        return "invalid_cursor";
    for (const char *digit = hash_start; digit < hash_end; ++digit)
        if (!isxdigit((unsigned char)*digit))
            return "invalid_cursor";
    errno = 0;
    char *parsed_end = NULL;
    unsigned long long hash = strtoull(hash_start, &parsed_end, 16);
    if (errno == ERANGE || parsed_end != hash_end)
        return "invalid_cursor";
    const char *hop_start = hash_end + 1;
    const char *hop_end = strchr(hop_start, '.');
    errno = 0;
    long parsed_hop = hop_end ? strtol(hop_start, &parsed_end, 10) : -1;
    if (!hop_end || hop_end == hop_start || errno == ERANGE || parsed_end != hop_end ||
        parsed_hop < 1 || parsed_hop > INT_MAX)
        return "invalid_cursor";
    const char *node_start = hop_end + 1;
    errno = 0;
    long long node = strtoll(node_start, &parsed_end, 10);
    if (node_start == parsed_end || errno == ERANGE || *parsed_end != '\0' || node <= 0)
        return "invalid_cursor";
    out->hop = (int)parsed_hop;
    out->qhash = (uint64_t)hash;
    out->node_id = (int64_t)node;
    if (out->qhash != expected)
        return "cursor_params_mismatch";
    if (strcmp(out->generation, generation) != 0)
        return "stale_cursor";
    return NULL;
}

/* Validate an exactly-issued watermark and return the first row after it.
 * Treating an arbitrary (hop,id) as an insertion point would let a syntactically
 * valid but edited cursor skip rows. The watermarked row must still exist in the
 * selected, test-filtered leg for this generation; -1 means it does not. */
static int watermark_next_index(const cbm_traverse_result_t *result, int hop, int64_t node_id) {
    for (int i = 0; i < result->visited_count; ++i) {
        if (result->visited[i].hop == hop && result->visited[i].node.id == node_id)
            return i + 1;
        if (result->visited[i].hop > hop ||
            (result->visited[i].hop == hop && result->visited[i].node.id > node_id))
            break;
    }
    return -1;
}

/* Edge collection returns the whole induced subgraph, not a predecessor tree,
 * so an arbitrary incident edge can point sideways or away from the root. The
 * evidence and args for a row must come from a deterministic edge that can
 * actually precede it on a shortest path: it has the traversal direction and
 * connects hop h to hop h-1 (or a resolved root for hop 1). */
typedef struct {
    const cbm_traverse_result_t *full;
    const cbm_node_t *roots;
    int root_count;
    bool inbound;
} trace_edge_context_t;

static bool root_contains(const trace_edge_context_t *ctx, int64_t node_id) {
    for (int i = 0; ctx && i < ctx->root_count; ++i)
        if (ctx->roots[i].id == node_id)
            return true;
    return false;
}

static int hop_for_node(const trace_edge_context_t *ctx, int64_t node_id) {
    if (!ctx || !ctx->full)
        return -1;
    for (int i = 0; i < ctx->full->visited_count; ++i)
        if (ctx->full->visited[i].node.id == node_id)
            return ctx->full->visited[i].hop;
    return -1;
}

static const cbm_edge_info_t *predecessor_edge(const trace_edge_context_t *ctx,
                                               const cbm_node_hop_t *hop_node) {
    if (!ctx || !ctx->full || !hop_node || hop_node->hop <= 0)
        return NULL;
    const cbm_edge_info_t *best = NULL;
    int64_t best_predecessor = INT64_MAX;
    for (int e = 0; e < ctx->full->edge_count; ++e) {
        const cbm_edge_info_t *edge = &ctx->full->edges[e];
        if ((ctx->inbound && edge->source_id != hop_node->node.id) ||
            (!ctx->inbound && edge->target_id != hop_node->node.id))
            continue;
        int64_t predecessor = ctx->inbound ? edge->target_id : edge->source_id;
        bool previous_hop = hop_node->hop == 1
                                ? root_contains(ctx, predecessor)
                                : hop_for_node(ctx, predecessor) == hop_node->hop - 1;
        if (!previous_hop)
            continue;
        const char *edge_type = edge->type ? edge->type : "";
        const char *best_type = best && best->type ? best->type : "";
        if (!best || predecessor < best_predecessor ||
            (predecessor == best_predecessor && strcmp(edge_type, best_type) < 0)) {
            best = edge;
            best_predecessor = predecessor;
        }
    }
    return best;
}

/* Serialized argument-expression array from the selected predecessor edge. The
 * returned slice is borrowed from properties_json. */
static const char *edge_args(const cbm_edge_info_t *edge, size_t *length) {
    const char *properties = edge ? edge->properties_json : NULL;
    const char *key = properties ? strstr(properties, "\"args\"") : NULL;
    const char *open = key ? strchr(key, '[') : NULL;
    if (!open)
        return NULL;
    int depth = 0;
    const char *p = open;
    for (; *p; ++p) {
        if (*p == '[')
            ++depth;
        else if (*p == ']' && --depth == 0) {
            ++p;
            break;
        }
    }
    if (depth != 0)
        return NULL;
    *length = (size_t)(p - open);
    return open;
}

static const char *strategy_class(const char *strategy) {
    if (!strategy || !strategy[0])
        return NULL;
    if (strcmp(strategy, "lsp_unresolved") == 0 || strcmp(strategy, "unknown") == 0)
        return "unresolved";
    if (strncmp(strategy, "lsp_", 4U) == 0)
        return "lsp";
    if (strncmp(strategy, "php_", 4U) == 0 || strncmp(strategy, "perl_", 5U) == 0)
        return "language_rule";
    return "heuristic";
}

static bool edge_evidence(const cbm_edge_info_t *edge, char class_buffer[32], double *confidence) {
    const char *properties = edge ? edge->properties_json : NULL;
    const char *key = properties ? strstr(properties, "\"strategy\"") : NULL;
    const char *open = key ? strchr(key + 10, '"') : NULL;
    if (!open)
        return false;
    ++open;
    const char *close = strchr(open, '"');
    if (!close || close == open)
        return false;
    char raw[64];
    size_t length = (size_t)(close - open);
    if (length >= sizeof(raw))
        length = sizeof(raw) - 1U;
    memcpy(raw, open, length);
    raw[length] = '\0';
    const char *classification = strategy_class(raw);
    if (!classification)
        return false;
    (void)snprintf(class_buffer, 32U, "%s", classification);
    *confidence = -1.0;
    const char *conf = strstr(properties, "\"confidence\"");
    const char *colon = conf ? strchr(conf, ':') : NULL;
    if (colon) {
        /* strtod answers 0.0 for text it cannot read, and callers publish any value >= 0
         * as a recorded confidence. Keep the -1 when nothing was parsed. */
        char *end = NULL;
        double parsed = strtod(colon + 1, &end);
        if (end != colon + 1)
            *confidence = parsed;
    }
    return true;
}

static size_t qn_prefix_length(const char *qualified_name) {
    const char *last = qualified_name ? strrchr(qualified_name, '.') : NULL;
    return last ? (size_t)(last - qualified_name) : 0U;
}

static yyjson_mut_val *leg_json(yyjson_mut_doc *doc, const cbm_traverse_result_t *result,
                                bool risk_labels, bool include_tests, bool data_flow,
                                bool include_evidence, const trace_edge_context_t *edge_ctx) {
    yyjson_mut_val *leg = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_str(doc, leg, "qn_rule",
                           "qn = qn_prefix == \"\" ? name : qn_prefix + \".\" + name");
    yyjson_mut_val *columns = yyjson_mut_arr(doc);
    yyjson_mut_arr_add_str(doc, columns, "name");
    yyjson_mut_arr_add_str(doc, columns, "hop");
    if (risk_labels)
        yyjson_mut_arr_add_str(doc, columns, "risk");
    if (include_tests)
        yyjson_mut_arr_add_str(doc, columns, "test");
    /* Declaration order matches row emission: strategy, confidence, then args. */
    if (include_evidence) {
        yyjson_mut_arr_add_str(doc, columns, "strategy");
        yyjson_mut_arr_add_str(doc, columns, "confidence");
    }
    if (data_flow)
        yyjson_mut_arr_add_str(doc, columns, "args");
    yyjson_mut_obj_add_val(doc, leg, "cols", columns);
    yyjson_mut_val *groups = yyjson_mut_arr(doc);
    yyjson_mut_val *rows = NULL;
    char group_name[1024] = "";
    bool have_group = false;
    for (int i = 0; i < result->visited_count; ++i) {
        const cbm_node_hop_t *hop = &result->visited[i];
        const char *qn = hop->node.qualified_name ? hop->node.qualified_name : "";
        size_t prefix = qn_prefix_length(qn);
        if (prefix >= sizeof(group_name))
            prefix = 0U;
        if (!have_group || strlen(group_name) != prefix || strncmp(group_name, qn, prefix) != 0) {
            (void)snprintf(group_name, sizeof(group_name), "%.*s", (int)prefix, qn);
            have_group = true;
            yyjson_mut_val *group = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_strcpy(doc, group, "qn_prefix", group_name);
            rows = yyjson_mut_arr(doc);
            yyjson_mut_obj_add_val(doc, group, "rows", rows);
            yyjson_mut_arr_add_val(groups, group);
        }
        yyjson_mut_val *row = yyjson_mut_arr(doc);
        yyjson_mut_arr_add_strcpy(doc, row, prefix ? qn + prefix + 1U : qn);
        yyjson_mut_arr_add_int(doc, row, hop->hop);
        if (risk_labels)
            yyjson_mut_arr_add_str(doc, row, cbm_risk_label(cbm_hop_to_risk(hop->hop)));
        if (include_tests)
            yyjson_mut_arr_add_bool(doc, row, is_test_file(hop->node.file_path));
        const cbm_edge_info_t *predecessor =
            (data_flow || include_evidence) ? predecessor_edge(edge_ctx, hop) : NULL;
        if (include_evidence) {
            char classification[32];
            double confidence = -1.0;
            if (edge_evidence(predecessor, classification, &confidence)) {
                yyjson_mut_arr_add_strcpy(doc, row, classification);
                if (confidence >= 0.0)
                    yyjson_mut_arr_add_real(doc, row, confidence);
                else
                    yyjson_mut_arr_add_null(doc, row);
            } else {
                yyjson_mut_arr_add_null(doc, row);
                yyjson_mut_arr_add_null(doc, row);
            }
        }
        if (data_flow) {
            size_t length = 0U;
            const char *raw = edge_args(predecessor, &length);
            yyjson_mut_val *value = raw && length ? yyjson_mut_rawn(doc, raw, length) : NULL;
            if (value)
                yyjson_mut_arr_add_val(row, value);
            else
                yyjson_mut_arr_add_str(doc, row, "");
        }
        yyjson_mut_arr_add_val(rows, row);
    }
    yyjson_mut_obj_add_val(doc, leg, "groups", groups);
    return leg;
}

cbm_operation_result_t cbm_trace_operation_execute(const char *args) {
    char *function = string_arg(args, "function_name");
    char *project = string_arg(args, "project");
    char *direction = string_arg(args, "direction");
    char *mode = string_arg(args, "mode");
    char *parameter_name = string_arg(args, "parameter_name");
    char *cursor_text = string_arg(args, "cursor");
    bool risk_labels = bool_arg(args, "risk_labels");
    bool include_tests = bool_arg(args, "include_tests");
    bool include_evidence = bool_arg(args, "include_evidence");
    int requested_depth = int_arg(args, "depth", TRACE_DEFAULT_DEPTH);
    int requested_limit = int_arg(args, "limit", TRACE_DEFAULT_LIMIT);
    int depth = requested_depth < 1 ? 1 : requested_depth;
    int max_depth = cbm_operation_max_depth();
    if (depth > max_depth)
        depth = max_depth;
    int limit = requested_limit < 1 ? 1 : requested_limit;
    if (limit > TRACE_MAX_LIMIT)
        limit = TRACE_MAX_LIMIT;
    if (!direction)
        direction = copy_text("both");

    cbm_operation_result_t result = {0};
    cbm_store_t *store = NULL;
    cbm_store_open_status_t open_status = CBM_STORE_OPEN_OK;
    cbm_node_t *nodes = NULL;
    int node_count = 0;
    yyjson_doc *edge_doc = NULL;
    cbm_traverse_result_t outbound = {0};
    cbm_traverse_result_t inbound = {0};

    if (!function || !function[0]) {
        result =
            error_result("function_name is required", "Use search first to discover a symbol.");
        goto done;
    }
    if (!project || !project[0]) {
        result = error_result("project is required", "Run the command from an indexed repository.");
        goto done;
    }
    if (strcmp(direction, "inbound") != 0 && strcmp(direction, "outbound") != 0 &&
        strcmp(direction, "both") != 0) {
        result = error_result("invalid direction", "Use inbound, outbound, or both.");
        goto done;
    }
    store = cbm_store_host_open_query(project, &open_status);
    if (!store) {
        result =
            open_status == CBM_STORE_OPEN_CORRUPT
                ? error_result(CBM_STORE_CORRUPT_MESSAGE, CBM_STORE_CORRUPT_HINT)
                : error_result("project not indexed", "Run 'codebase-memory-cli index .' first.");
        goto done;
    }

    (void)cbm_store_find_nodes_by_name(store, project, function, &nodes, &node_count);
    if (node_count == 0) {
        cbm_node_t exact = {0};
        if (cbm_store_find_node_by_qn(store, project, function, &exact) == CBM_STORE_OK) {
            nodes = malloc(sizeof(*nodes));
            if (!nodes) {
                cbm_node_free_fields(&exact);
                result = error_result("out of memory", NULL);
                goto done;
            }
            nodes[0] = exact;
            node_count = 1;
        }
    }
    if (node_count == 0) {
        result = error_result(
            "function not found",
            "Use 'codebase-memory-cli search <term>' to discover the exact qualified name.");
        goto done;
    }
    if (nodes_ambiguous(nodes, node_count)) {
        result = ambiguous_result(function, nodes, node_count);
        goto done;
    }

    char generation[96] = "legacy";
    (void)cbm_store_generation(store, generation, sizeof(generation));
    bool legacy_generation = strcmp(generation, "legacy") == 0;
    trace_cursor_t cursor = {0};
    bool have_cursor = cursor_text && cursor_text[0];
    uint64_t hash =
        params_hash(project, function, direction, mode, parameter_name, requested_depth,
                    include_tests, risk_labels, include_evidence, requested_limit, args);
    if (have_cursor) {
        if (legacy_generation) {
            result = error_result(
                "cursor_unsupported",
                "Re-run with a higher limit or re-index to enable generation-aware cursors.");
            goto done;
        }
        const char *cursor_error = cursor_decode(cursor_text, generation, hash, &cursor);
        if (cursor_error) {
            result = error_result(
                cursor_error,
                "Re-run without cursor, or pass it back with all other arguments unchanged.");
            goto done;
        }
        if ((cursor.leg == 'o' && strcmp(direction, "inbound") == 0) ||
            (cursor.leg == 'i' && strcmp(direction, "outbound") == 0)) {
            result = error_result("invalid_cursor",
                                  "The cursor leg is incompatible with the requested direction; "
                                  "re-run the original query without cursor.");
            goto done;
        }
    }

    const char *edge_types[TRACE_MAX_EDGE_TYPES];
    int edge_type_count = 0;
    edge_doc = resolve_edge_types(args, mode, edge_types, &edge_type_count);
    bool do_outbound = strcmp(direction, "outbound") == 0 || strcmp(direction, "both") == 0;
    bool do_inbound = strcmp(direction, "inbound") == 0 || strcmp(direction, "both") == 0;
    bool data_flow = mode && strcmp(mode, "data_flow") == 0;
    /* Edge properties are only needed for data-flow args and evidence columns. */
    int edge_data_limit = data_flow || include_evidence ? TRACE_MAX_LIMIT : 0;
    /* Traverse with the safety ceiling, not the page size: the traversal
     * enumerates the whole depth-bounded reachable set regardless, so exact
     * totals and the rows every later page needs come for free. */
    if (do_outbound && !bfs_union(store, nodes, node_count, "outbound", edge_types, edge_type_count,
                                  depth, TRACE_MAX_LIMIT, edge_data_limit, &outbound)) {
        result = error_result("trace traversal failed", "Inspect index status and retry.");
        goto done;
    }
    if (do_inbound && !bfs_union(store, nodes, node_count, "inbound", edge_types, edge_type_count,
                                 depth, TRACE_MAX_LIMIT, edge_data_limit, &inbound)) {
        result = error_result("trace traversal failed", "Inspect index status and retry.");
        goto done;
    }
    if (!include_tests) {
        filter_test_rows(&outbound);
        filter_test_rows(&inbound);
    }

    int out_start = 0;
    int in_start = 0;
    if (have_cursor) {
        int watermark_next;
        if (cursor.leg == 'o') {
            watermark_next = watermark_next_index(&outbound, cursor.hop, cursor.node_id);
            out_start = watermark_next;
        } else {
            out_start = outbound.visited_count; /* callees leg already drained */
            watermark_next = watermark_next_index(&inbound, cursor.hop, cursor.node_id);
            in_start = watermark_next;
        }
        if (watermark_next < 0) {
            result = error_result("invalid_cursor",
                                  "The watermark is not present in the selected trace leg; "
                                  "re-run the original query without cursor.");
            goto done;
        }
    }
    int budget = limit;
    int requested_out_len = 0;
    int requested_in_len = 0;
    if (do_outbound) {
        requested_out_len = outbound.visited_count - out_start;
        if (requested_out_len > budget)
            requested_out_len = budget;
        budget -= requested_out_len;
    }
    if (do_inbound) {
        requested_in_len = inbound.visited_count - in_start;
        if (requested_in_len > budget)
            requested_in_len = budget;
    }
    /* The requested page is fixed by `limit`; the byte ceiling may only shrink
     * the emitted whole-row window, never slice an identifier. Cursors are
     * recomputed on every render so `next_cursor` always follows the last row
     * that was actually emitted. */
    const int requested_page_rows = requested_out_len + requested_in_len;
    int row_target = requested_page_rows;
    int budget_search_low = 0;
    int budget_search_high = requested_page_rows;
    int budget_search_best = 0;
    bool budget_search_active = false;
    bool output_budget_hit = false;
    bool emit_optional_fields = risk_labels || data_flow || include_evidence;
    bool optional_fields_omitted = false;
    size_t output_budget_bytes = cbm_output_budget_bytes(cbm_output_budget_tokens(args, 0));
    char *json = NULL;

render_trace_output:;
    int rows_left = row_target;
    int out_len = requested_out_len < rows_left ? requested_out_len : rows_left;
    rows_left -= out_len;
    int in_len = requested_in_len < rows_left ? requested_in_len : rows_left;
    bool out_more = do_outbound && out_start + out_len < outbound.visited_count;
    bool in_more = do_inbound && in_start + in_len < inbound.visited_count;
    bool more = out_more || in_more;
    bool engine_saturated = outbound.truncated || inbound.truncated;
    bool edge_data_saturated = outbound.edges_truncated || inbound.edges_truncated;
    char next[192] = "";
    if (more && !legacy_generation) {
        trace_cursor_t next_cursor = {0};
        (void)snprintf(next_cursor.generation, sizeof(next_cursor.generation), "%s", generation);
        next_cursor.qhash = hash;
        if (out_more && out_len > 0) {
            next_cursor.leg = 'o';
            next_cursor.hop = outbound.visited[out_start + out_len - 1].hop;
            next_cursor.node_id = outbound.visited[out_start + out_len - 1].node.id;
        } else if (in_len > 0) {
            next_cursor.leg = 'i';
            next_cursor.hop = inbound.visited[in_start + in_len - 1].hop;
            next_cursor.node_id = inbound.visited[in_start + in_len - 1].node.id;
        }
        if (next_cursor.leg)
            cursor_encode(&next_cursor, next, sizeof(next));
    }

    cbm_traverse_result_t out_view = outbound;
    out_view.visited = outbound.visited ? outbound.visited + out_start : NULL;
    out_view.visited_count = out_len;
    cbm_traverse_result_t in_view = inbound;
    in_view.visited = inbound.visited ? inbound.visited + in_start : NULL;
    in_view.visited_count = in_len;

    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root) {
        if (doc)
            yyjson_mut_doc_free(doc);
        result = error_result("result allocation failed", NULL);
        goto done;
    }
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "project", project);
    yyjson_mut_obj_add_strcpy(doc, root, "function", function);
    yyjson_mut_obj_add_strcpy(doc, root, "direction", direction);
    if (mode)
        yyjson_mut_obj_add_strcpy(doc, root, "mode", mode);
    if (requested_depth != depth) {
        yyjson_mut_obj_add_int(doc, root, "requested_depth", requested_depth);
        yyjson_mut_obj_add_int(doc, root, "depth", depth);
        yyjson_mut_obj_add_str(
            doc, root, "depth_note",
            "requested depth was capped by the configured traversal safety limit");
    }
    /* Totals count exactly what the caller can enumerate: test rows were
     * filtered before windowing when include_tests is false. "gte" flags the
     * traversal safety ceiling. */
    trace_edge_context_t out_edge_ctx = {
        .full = &outbound, .roots = nodes, .root_count = node_count, .inbound = false};
    trace_edge_context_t in_edge_ctx = {
        .full = &inbound, .roots = nodes, .root_count = node_count, .inbound = true};
    if (do_outbound) {
        yyjson_mut_obj_add_int(doc, root, "callees_total", outbound.visited_count);
        yyjson_mut_obj_add_str(doc, root, "callees_total_relation",
                               outbound.truncated ? "gte" : "eq");
        yyjson_mut_obj_add_val(doc, root, "callees",
                               leg_json(doc, &out_view, risk_labels && emit_optional_fields,
                                        include_tests, data_flow && emit_optional_fields,
                                        include_evidence && emit_optional_fields, &out_edge_ctx));
    }
    if (do_inbound) {
        yyjson_mut_obj_add_int(doc, root, "callers_total", inbound.visited_count);
        yyjson_mut_obj_add_str(doc, root, "callers_total_relation",
                               inbound.truncated ? "gte" : "eq");
        yyjson_mut_obj_add_val(doc, root, "callers",
                               leg_json(doc, &in_view, risk_labels && emit_optional_fields,
                                        include_tests, data_flow && emit_optional_fields,
                                        include_evidence && emit_optional_fields, &in_edge_ctx));
    }
    if (more || engine_saturated || edge_data_saturated || output_budget_hit) {
        yyjson_mut_obj_add_bool(doc, root, "truncated", true);
        yyjson_mut_obj_add_bool(doc, root, "has_more", more);
        yyjson_mut_obj_add_str(doc, root, "truncation_reason",
                               output_budget_hit  ? "output_budget"
                               : more             ? "page_limit"
                               : engine_saturated ? "engine_limit"
                                                  : "edge_data_limit");
        if (output_budget_hit)
            yyjson_mut_obj_add_uint(doc, root, "max_output_bytes", output_budget_bytes);
        if (optional_fields_omitted)
            emit_omitted_optional_fields(doc, root, risk_labels, data_flow, include_evidence);
        if (engine_saturated)
            yyjson_mut_obj_add_bool(doc, root, "engine_saturated", true);
        if (edge_data_saturated)
            yyjson_mut_obj_add_bool(doc, root, "edge_data_saturated", true);
        if (next[0]) {
            yyjson_mut_obj_add_strcpy(doc, root, "next_cursor", next);
        } else if (more) {
            yyjson_mut_obj_add_str(doc, root, "hint",
                                   "More rows exist; raise limit because this legacy index cannot "
                                   "mint a safe cursor.");
        } else if (engine_saturated) {
            yyjson_mut_obj_add_str(doc, root, "hint",
                                   "Traversal reached the 5000-node safety ceiling; narrow "
                                   "depth/edge_types or use query for a differently bounded "
                                   "traversal.");
        } else {
            yyjson_mut_obj_add_str(doc, root, "hint",
                                   "Optional edge evidence reached its 5000-edge ceiling; narrow "
                                   "depth/edge_types or disable data_flow/include_evidence.");
        }
    }
    json = doc_to_str(doc);
    doc = NULL;

    if (output_budget_bytes > 0 && json && strlen(json) > output_budget_bytes) {
        output_budget_hit = true;
        free(json);
        json = NULL;
        if (emit_optional_fields) {
            emit_optional_fields = false;
            optional_fields_omitted = true;
            goto render_trace_output;
        }
        if (row_target <= 1 || requested_page_rows == 0) {
            /* Even one graph row cannot fit: report the exact totals and the
             * continuation without emitting a fabricated cursor. */
            yyjson_mut_doc *floor_doc = yyjson_mut_doc_new(NULL);
            yyjson_mut_val *floor = floor_doc ? yyjson_mut_obj(floor_doc) : NULL;
            if (!floor_doc || !floor) {
                if (floor_doc)
                    yyjson_mut_doc_free(floor_doc);
                result = error_result("result allocation failed", NULL);
                goto done;
            }
            yyjson_mut_doc_set_root(floor_doc, floor);
            if (do_outbound) {
                yyjson_mut_obj_add_int(floor_doc, floor, "callees_total", outbound.visited_count);
                yyjson_mut_obj_add_str(floor_doc, floor, "callees_total_relation",
                                       outbound.truncated ? "gte" : "eq");
            }
            if (do_inbound) {
                yyjson_mut_obj_add_int(floor_doc, floor, "callers_total", inbound.visited_count);
                yyjson_mut_obj_add_str(floor_doc, floor, "callers_total_relation",
                                       inbound.truncated ? "gte" : "eq");
            }
            bool floor_has_more = requested_page_rows > 0;
            yyjson_mut_obj_add_bool(floor_doc, floor, "has_more", floor_has_more);
            if (floor_has_more)
                yyjson_mut_obj_add_bool(floor_doc, floor, "continuation_requires_higher_budget",
                                        true);
            yyjson_mut_obj_add_bool(floor_doc, floor, "truncated", true);
            yyjson_mut_obj_add_str(floor_doc, floor, "truncation_reason", "output_budget");
            yyjson_mut_obj_add_bool(floor_doc, floor, "output_budget_floor_exceeded", true);
            yyjson_mut_obj_add_uint(floor_doc, floor, "max_output_bytes", output_budget_bytes);
            if (optional_fields_omitted)
                emit_omitted_optional_fields(floor_doc, floor, risk_labels, data_flow,
                                             include_evidence);
            yyjson_mut_obj_add_str(floor_doc, floor, "hint",
                                   "raise max_output_tokens; no identifier was sliced");
            json = doc_to_str(floor_doc);
            goto trace_output_ready;
        }
        if (!budget_search_active) {
            budget_search_active = true;
            budget_search_low = 1;
            budget_search_high = row_target - 1;
        } else {
            budget_search_high = row_target - 1;
        }
        if (budget_search_low > budget_search_high) {
            row_target = budget_search_best < 1 ? 1 : budget_search_best;
            budget_search_active = false;
            goto render_trace_output;
        }
        row_target = budget_search_low + (budget_search_high - budget_search_low) / 2;
        goto render_trace_output;
    }
    if (budget_search_active && json && strlen(json) <= output_budget_bytes) {
        budget_search_best = row_target;
        budget_search_low = row_target + 1;
        if (budget_search_low <= budget_search_high) {
            free(json);
            json = NULL;
            row_target = budget_search_low + (budget_search_high - budget_search_low + 1) / 2;
            goto render_trace_output;
        }
    }

trace_output_ready:
    result = json ? cbm_operation_result_take(json, false) : error_result("out of memory", NULL);
    json = NULL;

done:
    if (edge_doc)
        yyjson_doc_free(edge_doc);
    cbm_store_traverse_free(&outbound);
    cbm_store_traverse_free(&inbound);
    if (nodes)
        cbm_store_free_nodes(nodes, node_count);
    if (store)
        cbm_store_close(store);
    free(function);
    free(project);
    free(direction);
    free(mode);
    free(parameter_name);
    free(cursor_text);
    return result;
}
