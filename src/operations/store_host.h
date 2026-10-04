#ifndef CBM_OPERATIONS_STORE_HOST_H
#define CBM_OPERATIONS_STORE_HOST_H

#include <stdbool.h>
#include <stddef.h>
#include "operations/operation.h"
#include "store/store.h"

typedef struct cbm_store_host cbm_store_host_t;
typedef bool (*cbm_store_host_mutation_begin_fn)(void *context, const char *project);
typedef bool (*cbm_store_host_mutation_try_begin_fn)(void *context, const char *project);
typedef void (*cbm_store_host_mutation_end_fn)(void *context, const char *project);
typedef bool (*cbm_store_host_quarantine_step_fn)(void *context, const char *step);

cbm_store_host_t *cbm_store_host_new(const char *store_path);
cbm_store_host_t *cbm_store_host_new_deferred(void);
void cbm_store_host_free(cbm_store_host_t *host);

cbm_store_t *cbm_store_host_store(cbm_store_host_t *host);
const char *cbm_store_host_current_project(const cbm_store_host_t *host);
void cbm_store_host_set_project(cbm_store_host_t *host, const char *project);
void cbm_store_host_detach_project(cbm_store_host_t *host, const char *project);
void cbm_store_host_evict_idle(cbm_store_host_t *host, int timeout_s);
bool cbm_store_host_has_cached_store(cbm_store_host_t *host);
bool cbm_store_host_release_pristine_memory_store(cbm_store_host_t *host);

void cbm_store_host_set_mutation_guard(cbm_store_host_t *host,
                                       cbm_store_host_mutation_begin_fn begin,
                                       cbm_store_host_mutation_try_begin_fn try_begin,
                                       cbm_store_host_mutation_end_fn end, void *context);
void cbm_store_host_set_quarantine_step_hook(cbm_store_host_t *host,
                                             cbm_store_host_quarantine_step_fn hook, void *context);

cbm_store_t *cbm_store_host_resolve(cbm_store_host_t *host, const char *project,
                                    bool mutation_already_held, bool nonblocking_recovery,
                                    cbm_operation_store_recovery_status_t *recovery_status);
void cbm_store_host_invalidate(cbm_store_host_t *host);
char *cbm_store_host_error(const char *project);

/* Query-only store open for read operations. Strictly non-mutating: no mutation
 * lease, no quarantine, no rebuild, no file creation. A database that fails the
 * shallow integrity check is classified (corrupt vs transient) and left in place;
 * rebuilding is reserved for write-side opens (index, manage_adr writes). */
typedef enum cbm_store_open_status {
    CBM_STORE_OPEN_OK = 0,
    CBM_STORE_OPEN_NOT_FOUND, /* no database, or it could not be opened */
    CBM_STORE_OPEN_CORRUPT,   /* confirmed corrupt; left untouched */
    CBM_STORE_OPEN_BUSY,      /* integrity check was inconclusive (lock/IO) */
} cbm_store_open_status_t;

/* Message and hint for CBM_STORE_OPEN_CORRUPT. The combined form is for
 * single-string error paths. */
#define CBM_STORE_CORRUPT_MESSAGE "project store is corrupt (left untouched)"
#define CBM_STORE_CORRUPT_HINT "Run 'codebase-memory-cli index .' to rebuild it."
#define CBM_STORE_CORRUPT_ERROR \
    "project store is corrupt (left untouched); run 'codebase-memory-cli index .' to rebuild it"

/* status may be NULL. Returns NULL unless status is CBM_STORE_OPEN_OK. */
cbm_store_t *cbm_store_host_open_query(const char *project, cbm_store_open_status_t *status);
cbm_store_t *cbm_store_host_open_query_path(const char *db_path, cbm_store_open_status_t *status);

bool cbm_store_host_db_internal_project_name(const char *full_path, char *name_out, size_t name_sz,
                                             cbm_store_t **out_store);
bool cbm_store_host_is_project_db_file(const char *name, size_t len);

#endif
