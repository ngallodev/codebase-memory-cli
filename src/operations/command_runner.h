#ifndef CBM_OPERATIONS_COMMAND_RUNNER_H
#define CBM_OPERATIONS_COMMAND_RUNNER_H

#include "operations/operation.h"
#include "foundation/constants.h"
#include "foundation/subprocess.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum cbm_operation_command_cause {
    CBM_OPERATION_COMMAND_SUCCESS = 0,
    CBM_OPERATION_COMMAND_FAILURE,
    CBM_OPERATION_COMMAND_CONTAINED_FAILURE,
    CBM_OPERATION_COMMAND_OUTPUT_LIMIT,
    CBM_OPERATION_COMMAND_DEADLINE,
    CBM_OPERATION_COMMAND_CANCELLED,
    CBM_OPERATION_COMMAND_SUPERVISION_FAILURE,
} cbm_operation_command_cause_t;

bool cbm_operation_runtime_cancelled(const cbm_operation_runtime_t *runtime);

cbm_operation_command_cause_t cbm_operation_run_shell_command_bounded(
    const cbm_operation_runtime_t *runtime, const char *command, char output_path[CBM_SZ_2K],
    size_t output_limit, uint64_t deadline_ms, bool deadline_enabled, bool deadline_latched,
    bool exit_one_is_no_match, cbm_proc_result_t *result_out);

#ifndef _WIN32
/* POSIX-only: supervised run of one argv stage (no shell). Same latch and
 * containment contract as the shell runner. The measured stdout_capture file's
 * size plus output_base counts toward output_limit so chained stages share one
 * budget. tolerate_exit_one maps a plain exit-1 (grep's no-match) to SUCCESS. */
cbm_operation_command_cause_t cbm_operation_run_argv_stage_bounded(
    const cbm_operation_runtime_t *runtime, const char *bin, const char *const *argv,
    const char *const *env_overrides, const char *stdout_capture, uint64_t output_base,
    size_t output_limit, uint64_t deadline_ms, bool deadline_enabled, bool tolerate_exit_one,
    bool *cancellation_latched, bool *deadline_latched, bool *output_limit_latched,
    cbm_proc_result_t *result_out);
#endif

/* Compatibility shape used by detect-changes style callers: any contained
 * terminal process tree is considered wrapper success and the caller inspects
 * result_out for exit/cancellation classification. */
int cbm_operation_run_shell_command(const cbm_operation_runtime_t *runtime, const char *command,
                                    char output_path[CBM_SZ_2K], cbm_proc_result_t *result_out);

#endif
