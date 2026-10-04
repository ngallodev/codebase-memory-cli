#include "operations/result_wire.h"
#include "operations/schema.h"
#include "operations/store_host.h"

#include "store/store.h"
#include "yyjson/yyjson.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *copy_string(const char *text) {
    if (!text)
        return NULL;
    size_t len = strlen(text);
    char *copy = malloc(len + 1U);
    if (copy)
        memcpy(copy, text, len + 1U);
    return copy;
}

static char *project_arg(const char *args_json) {
    yyjson_doc *doc =
        yyjson_read(args_json ? args_json : "{}", strlen(args_json ? args_json : "{}"), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    static const char *const names[] = {"project", "project_name", "project_id", "projectName"};
    char *result = NULL;
    for (size_t i = 0; yyjson_is_obj(root) && i < sizeof(names) / sizeof(names[0]); ++i) {
        yyjson_val *value = yyjson_obj_get(root, names[i]);
        if (yyjson_is_str(value)) {
            result = copy_string(yyjson_get_str(value));
            break;
        }
    }
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static bool blocked_property(const char *name) {
    return name && (strcmp(name, "fp") == 0 || strcmp(name, "sp") == 0 || strcmp(name, "bt") == 0);
}

static bool project_has_adr(cbm_store_t *store, const char *project, const char *root_path) {
    cbm_adr_t adr = {0};
    if (store && project && cbm_store_adr_get(store, project, &adr) == CBM_STORE_OK) {
        cbm_store_adr_free(&adr);
        return true;
    }
    if (!root_path)
        return false;
    char path[4096];
    if (snprintf(path, sizeof(path), "%s/.codebase-memory/adr.md", root_path) >= (int)sizeof(path))
        return false;
    struct stat st;
    return stat(path, &st) == 0;
}

static int int_arg(const char *args_json, const char *name, int fallback) {
    yyjson_doc *doc =
        yyjson_read(args_json ? args_json : "{}", strlen(args_json ? args_json : "{}"), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, name) : NULL;
    int result = value && yyjson_is_int(value) ? (int)yyjson_get_sint(value) : fallback;
    if (doc)
        yyjson_doc_free(doc);
    return result;
}

static bool diagnostics_full(const char *args_json) {
    yyjson_doc *doc =
        yyjson_read(args_json ? args_json : "{}", strlen(args_json ? args_json : "{}"), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;
    yyjson_val *value = yyjson_is_obj(root) ? yyjson_obj_get(root, "diagnostics") : NULL;
    bool full = yyjson_is_str(value) && strcmp(yyjson_get_str(value), "full") == 0;
    if (doc)
        yyjson_doc_free(doc);
    return full;
}

typedef struct {
    bool node;
    int index;
    int count;
    const char *name;
} schema_record_t;

/* Labels first, then edge types; busiest first, name as the tie-break so pages
 * are stable across calls. */
static int schema_record_compare(const void *left, const void *right) {
    const schema_record_t *a = left;
    const schema_record_t *b = right;
    if (a->node != b->node)
        return a->node ? -1 : 1;
    if (a->count != b->count)
        return a->count > b->count ? -1 : 1;
    return strcmp(a->name ? a->name : "", b->name ? b->name : "");
}

cbm_operation_result_t cbm_schema_operation_execute(const char *args_json) {
    char *project = project_arg(args_json);
    if (!project || !project[0]) {
        free(project);
        return cbm_operation_result_copy("project is required", true);
    }
    cbm_store_open_status_t open_status = CBM_STORE_OPEN_OK;
    cbm_store_t *store = cbm_store_host_open_query(project, &open_status);
    if (!store) {
        free(project);
        return cbm_operation_result_copy(open_status == CBM_STORE_OPEN_CORRUPT
                                             ? CBM_STORE_CORRUPT_ERROR
                                             : "project not found or not indexed",
                                         true);
    }
    if (cbm_store_count_nodes(store, project) <= 0) {
        cbm_store_close(store);
        free(project);
        return cbm_operation_result_copy("project not indexed or index is empty", true);
    }

    /* Property listings dominate the payload; they are opt-in. */
    bool include_properties = diagnostics_full(args_json);
    cbm_schema_info_t schema = {0};
    if (include_properties)
        cbm_store_get_schema(store, project, &schema);
    else
        cbm_store_get_schema_counts(store, project, &schema);
    int limit = int_arg(args_json, "limit", 50);
    int offset = int_arg(args_json, "offset", 0);
    if (limit < 1)
        limit = 1;
    else if (limit > 500)
        limit = 500;
    if (offset < 0)
        offset = 0;
    int total = schema.node_label_count + schema.edge_type_count;
    schema_record_t *records = calloc(total > 0 ? (size_t)total : 1U, sizeof(*records));
    if (!records) {
        cbm_store_schema_free(&schema);
        cbm_store_close(store);
        free(project);
        return cbm_operation_result_copy("out of memory while rendering schema", true);
    }
    int record_count = 0;
    for (int i = 0; i < schema.node_label_count; ++i)
        records[record_count++] =
            (schema_record_t){true, i, schema.node_labels[i].count, schema.node_labels[i].label};
    for (int i = 0; i < schema.edge_type_count; ++i)
        records[record_count++] =
            (schema_record_t){false, i, schema.edge_types[i].count, schema.edge_types[i].type};
    qsort(records, (size_t)record_count, sizeof(*records), schema_record_compare);
    int start = offset < total ? offset : total;
    int end = start + limit < total ? start + limit : total;
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = doc ? yyjson_mut_obj(doc) : NULL;
    if (!doc || !root) {
        if (doc)
            yyjson_mut_doc_free(doc);
        free(records);
        cbm_store_schema_free(&schema);
        cbm_store_close(store);
        free(project);
        return cbm_operation_result_copy("result allocation failed", true);
    }
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_val *labels = yyjson_mut_arr(doc);
    for (int page = start; page < end; ++page) {
        if (!records[page].node)
            continue;
        int i = records[page].index;
        yyjson_mut_val *label = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, label, "label", schema.node_labels[i].label);
        yyjson_mut_obj_add_int(doc, label, "count", schema.node_labels[i].count);
        if (include_properties) {
            yyjson_mut_val *properties = yyjson_mut_arr(doc);
            for (int j = 0; j < schema.node_labels[i].property_count; ++j) {
                if (!blocked_property(schema.node_labels[i].properties[j]))
                    yyjson_mut_arr_add_str(doc, properties, schema.node_labels[i].properties[j]);
            }
            yyjson_mut_obj_add_val(doc, label, "properties", properties);
        }
        yyjson_mut_arr_add_val(labels, label);
    }
    yyjson_mut_obj_add_val(doc, root, "node_labels", labels);

    yyjson_mut_val *types = yyjson_mut_arr(doc);
    for (int page = start; page < end; ++page) {
        if (records[page].node)
            continue;
        int i = records[page].index;
        yyjson_mut_val *type = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, type, "type", schema.edge_types[i].type);
        yyjson_mut_obj_add_int(doc, type, "count", schema.edge_types[i].count);
        if (include_properties) {
            yyjson_mut_val *properties = yyjson_mut_arr(doc);
            for (int j = 0; j < schema.edge_types[i].property_count; ++j)
                yyjson_mut_arr_add_str(doc, properties, schema.edge_types[i].properties[j]);
            yyjson_mut_obj_add_val(doc, type, "properties", properties);
        }
        yyjson_mut_arr_add_val(types, type);
    }
    yyjson_mut_obj_add_val(doc, root, "edge_types", types);
    yyjson_mut_obj_add_int(doc, root, "total", total);
    yyjson_mut_obj_add_int(doc, root, "returned", end - start);
    yyjson_mut_obj_add_bool(doc, root, "has_more", end < total);
    if (end < total)
        yyjson_mut_obj_add_int(doc, root, "next_offset", end);

    cbm_project_t info = {0};
    if (cbm_store_get_project(store, project, &info) == CBM_STORE_OK && info.root_path) {
        bool adr_present = project_has_adr(store, project, info.root_path);
        yyjson_mut_obj_add_bool(doc, root, "adr_present", adr_present);
        if (!adr_present && include_properties) {
            yyjson_mut_obj_add_str(
                doc, root, "adr_hint",
                "No ADR found. Use manage_adr(mode='update') to persist architectural decisions "
                "across sessions. Run get_architecture(aspects=['all']) first.");
        }
        cbm_project_free_fields(&info);
    }

    char *payload = cbm_operation_json_write(doc);
    yyjson_mut_doc_free(doc);
    free(records);
    cbm_store_schema_free(&schema);
    cbm_store_close(store);
    free(project);
    return payload ? cbm_operation_result_take(payload, false)
                   : cbm_operation_result_copy("result encoding failed", true);
}
