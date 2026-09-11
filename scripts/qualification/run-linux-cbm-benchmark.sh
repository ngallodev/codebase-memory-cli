#!/usr/bin/env bash
# One-command Linux benchmark-corpus execution for this checkout.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)"
binary="$root/build/c/codebase-memory-cli"
workspace_root="${CBM_LINUX_BENCHMARK_WORKSPACE:-/var/tmp/cbm-linux-benchmark/repos}"
results_root="${CBM_LINUX_BENCHMARK_RESULTS:-/var/tmp/cbm-linux-benchmark/results}"
repeats="${CBM_LINUX_BENCHMARK_REPEATS:-5}"

case "${1:-}" in
  -h|--help)
    cat <<'EOF'
Usage: scripts/qualification/run-linux-cbm-benchmark.sh

Runs the frozen Linux benchmark corpus for this checkout's exact HEAD.
Build first with scripts/build-dev.sh if build/c/codebase-memory-cli is absent.

Environment overrides:
  CBM_LINUX_BENCHMARK_WORKSPACE  Repository checkout root
  CBM_LINUX_BENCHMARK_RESULTS    Results root
  CBM_LINUX_BENCHMARK_REPEATS    Measured repetitions (default: 5)
EOF
    exit 0
    ;;
esac

if [[ "$#" -ne 0 ]]; then
  echo "error: no positional arguments; use --help" >&2
  exit 2
fi
if [[ ! -x "$binary" ]]; then
  echo "error: missing executable $binary; run scripts/build-dev.sh first" >&2
  exit 2
fi

exec "$root/scripts/qualification/run-linux-benchmark-corpus.sh" \
  --binary "$binary" \
  --workspace-root "$workspace_root" \
  --results-root "$results_root" \
  --codebase-memory-ref "$(git -C "$root" rev-parse HEAD)" \
  --initialize \
  --repeats "$repeats"
