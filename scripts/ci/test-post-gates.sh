#!/usr/bin/env bash
# The production-only gates that follow the shared sharded test runner.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
BUILD_DIR="${1:-build/c}"
# shellcheck source=../env.sh
source "$ROOT/scripts/env.sh"

if [ "${CBM_RUN_HANG_TEST:-0}" = "1" ]; then
    echo "=== Step 4: C++ index-hang regression (#410) ==="
    bash "$ROOT/tests/test_cpp_index_hang.sh"
fi

echo "=== Step 5: prepare worker-watchdog test binary ==="
if [ -n "${CBM_TEST_SEAM_BINARY:-}" ]; then
    WATCHDOG_BINARY="$CBM_TEST_SEAM_BINARY"
    test -x "$WATCHDOG_BINARY" || {
        echo "missing prebuilt seam binary: $WATCHDOG_BINARY" >&2
        exit 1
    }
    echo "Using prebuilt seam binary: $WATCHDOG_BINARY"
else
    make -j"$NPROC" -f Makefile.cbm cbm TEST_SEAMS=1 BUILD_DIR="$BUILD_DIR"
    WATCHDOG_BINARY="$ROOT/$BUILD_DIR/codebase-memory-cli"
fi

echo "=== Step 5b: worker-mode watchdog regression (#845) ==="
CBM_TEST_BINARY="$WATCHDOG_BINARY" bash "$ROOT/tests/test_worker_watchdog.sh"

echo "=== Step 5c: worker error-response transport regression ==="
CBM_TEST_BINARY="$WATCHDOG_BINARY" bash "$ROOT/tests/test_worker_error_response.sh"

echo "=== Step 5e: watcher_enabled kill-switch regression (#335) ==="
CBM_TEST_BINARY="$WATCHDOG_BINARY" bash "$ROOT/tests/test_watcher_disabled.sh"

echo "=== Step 6: security-strings allow-list regression ==="
bash "$ROOT/tests/test_security_strings_allowlist.sh"
bash "$ROOT/tests/test_destructive_ordering_contract.sh"

echo "=== Post-test gates passed ==="
