#include "operations/watch_status.h"

#include "foundation/constants.h"
#include "foundation/compat.h"

#include <stdint.h>
#include <string.h>
#include <time.h>

void cbm_op_add_watch_status_json(yyjson_mut_doc *doc, yyjson_mut_val *root,
                                  const cbm_operation_runtime_t *runtime, const char *project) {
    cbm_operation_watch_status_t status = {0};
    if (runtime && runtime->watch_status) {
        runtime->watch_status(runtime->watch_status_context, project, &status);
    } else {
        status.reason = "no_watcher";
    }
    yyjson_mut_val *watch = yyjson_mut_obj(doc);
    if (!watch) {
        return;
    }
    yyjson_mut_obj_add_bool(doc, watch, "watched", status.watched);
    if (status.watched) {
        yyjson_mut_obj_add_str(doc, watch, "strategy", status.strategy ? status.strategy : "");
        yyjson_mut_obj_add_int(doc, watch, "poll_interval_ms", status.poll_interval_ms);
        if (status.last_scan_unix_s > 0) {
            char when[CBM_SZ_32];
            time_t t = (time_t)status.last_scan_unix_s;
            struct tm tm;
            cbm_gmtime_r(&t, &tm);
            if (strftime(when, sizeof(when), "%Y-%m-%dT%H:%M:%SZ", &tm) > 0) {
                yyjson_mut_obj_add_strcpy(doc, watch, "last_scan_at", when);
            }
        }
    } else {
        yyjson_mut_obj_add_str(doc, watch, "reason", status.reason ? status.reason : "unknown");
        yyjson_mut_obj_add_str(doc, watch, "hint",
                               "Auto-sync watches only the active session's project; "
                               "re-run 'codebase-memory-cli index .' to refresh this index.");
    }
    yyjson_mut_obj_add_val(doc, root, "watch", watch);
}
