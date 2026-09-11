#!/usr/bin/env bash
set -euo pipefail

# Reproducible Linux baseline for agent-relevant Codebase Memory CLI workflows.
# Usage: benchmark-agent-workflows.sh <binary> <repo> <results-dir> [secondary-repo]
# Optional environment: CBM_BENCH_QUERY, CBM_BENCH_SYMBOL, CBM_BENCH_FILE,
# CBM_BENCH_BASE_BRANCH, CBM_BENCH_REPEATS (default 5), CBM_BENCH_OPERATIONS
# (comma-separated; default index,search,architecture,snippet,outline,changes,status).
# Each invocation is one independent trial and must use a new/empty results directory.

BINARY="${1:?Usage: $0 <binary> <repo> <results-dir> [secondary-repo]}"
REPO="${2:?}"
RESULTS_DIR="${3:?}"
SECONDARY_REPO="${4:-}"
REPEATS="${CBM_BENCH_REPEATS:-5}"
QUERY="${CBM_BENCH_QUERY:-daemon}"
SYMBOL="${CBM_BENCH_SYMBOL:-main}"
FILE_PATH="${CBM_BENCH_FILE:-src/main.c}"
BASE_BRANCH="${CBM_BENCH_BASE_BRANCH:-main}"
OPERATIONS_CSV="${CBM_BENCH_OPERATIONS:-index,search,architecture,snippet,outline,changes,status}"
IFS=',' read -r -a OPERATIONS <<< "$OPERATIONS_CSV"

if ! [[ "$REPEATS" =~ ^[1-9][0-9]*$ ]]; then
  echo "error: CBM_BENCH_REPEATS must be >= 1" >&2
  exit 2
fi

REPO=$(cd "$REPO" && pwd -P)
if [[ -e "$RESULTS_DIR/timings.tsv" || -e "$RESULTS_DIR/cache" || -e "$RESULTS_DIR/RESULT" ]]; then
  echo "error: results directory already contains benchmark state; use a new empty directory: $RESULTS_DIR" >&2
  exit 2
fi
mkdir -p "$RESULTS_DIR"
RESULTS_DIR=$(cd "$RESULTS_DIR" && pwd -P)
export CBM_CACHE_DIR="$RESULTS_DIR/cache"
mkdir -p "$CBM_CACHE_DIR"

if [[ ! -x "$BINARY" ]]; then
  echo "error: binary is not executable: $BINARY" >&2
  exit 2
fi
BINARY=$(cd "$(dirname "$BINARY")" && pwd -P)/$(basename "$BINARY")

if [[ ! -f "$REPO/$FILE_PATH" ]]; then
  echo "error: benchmark workload file does not exist: $REPO/$FILE_PATH" >&2
  exit 2
fi

printf 'case\trun\telapsed_ms\texit_code\n' > "$RESULTS_DIR/timings.tsv"
FAILURES_FILE="$RESULTS_DIR/FAILURES.txt"
: > "$FAILURES_FILE"

now_ms() { python3 -c 'import time; print(time.monotonic_ns() // 1000000)'; }
wants() {
  local wanted="$1" op
  for op in "${OPERATIONS[@]}"; do [[ "$op" == "$wanted" ]] && return 0; done
  return 1
}
record_failure() { printf '%s\n' "$1" >> "$FAILURES_FILE"; }

run_once() {
  local label="$1" run="$2"; shift 2
  local out="$RESULTS_DIR/${label}.${run}.json"
  local err="$RESULTS_DIR/${label}.${run}.stderr"
  local start end rc
  start=$(now_ms)
  set +e
  "$@" >"$out" 2>"$err"
  rc=$?
  set -e
  end=$(now_ms)
  printf '%s\t%s\t%s\t%s\n' "$label" "$run" "$((end-start))" "$rc" >> "$RESULTS_DIR/timings.tsv"
  if (( rc != 0 )); then record_failure "$label run $run exit=$rc"; fi
  return "$rc"
}

repeat_case() {
  local label="$1"; shift
  local warm_rc
  set +e
  "$@" >/dev/null 2>/dev/null
  warm_rc=$?
  set -e
  if (( warm_rc != 0 )); then
    record_failure "$label warmup exit=$warm_rc"
    return 0
  fi
  local i
  for ((i=1; i<=REPEATS; i++)); do run_once "$label" "$i" "$@" || true; done
}

cleanup() { "$BINARY" daemon stop >/dev/null 2>&1 || true; }
trap cleanup EXIT

# Record daemon startup separately; steady-state action timings begin afterward.
"$BINARY" daemon stop >/dev/null 2>&1 || true
run_once startup 1 "$BINARY" daemon start || true
if ! awk -F '\t' 'NR>1 && $1=="startup" && $4==0 {ok=1} END{exit ok?0:1}' "$RESULTS_DIR/timings.tsv"; then
  echo "error: daemon start failed before benchmark workload" >&2
  exit 3
fi

if ! wants index; then
  echo "error: benchmark operations must include index" >&2
  exit 2
fi
run_once index 1 "$BINARY" index "$REPO" --mode full --json || true
if ! awk -F '\t' 'NR>1 && $1=="index" && $4==0 {ok=1} END{exit ok?0:1}' "$RESULTS_DIR/timings.tsv"; then
  echo "error: index failed" >&2
  exit 3
fi
PROJECT=$(python3 - "$RESULTS_DIR/index.1.json" <<'PY'
import json,sys
try:
    obj=json.load(open(sys.argv[1], encoding='utf-8'))
    print(obj.get('project') or '')
except Exception:
    print('')
PY
)
if [[ -z "$PROJECT" ]]; then
  record_failure "index did not return a project name"
  echo "error: index did not return a project name" >&2
  exit 3
fi

wants search && repeat_case search "$BINARY" search --project "$PROJECT" --query "$QUERY" --limit 20 --json
wants architecture && repeat_case architecture "$BINARY" architecture --project "$PROJECT" --json
wants snippet && repeat_case snippet "$BINARY" snippet --project "$PROJECT" --qualified-name "$SYMBOL" --json
wants outline && repeat_case outline "$BINARY" outline --project "$PROJECT" --file-path "$FILE_PATH" --limit 100 --json
wants changes && repeat_case changes "$BINARY" changes --project "$PROJECT" --scope files --base-branch "$BASE_BRANCH" --json
wants status && repeat_case status "$BINARY" status --project "$PROJECT" --json

if wants cross_repo; then
  if [[ -z "$SECONDARY_REPO" ]]; then
    record_failure "cross_repo requested but no secondary repository was supplied"
  else
    SECONDARY_REPO=$(cd "$SECONDARY_REPO" && pwd -P)
    run_once secondary_index 1 "$BINARY" index "$SECONDARY_REPO" --mode full --json || true
    SECONDARY_PROJECT=$(python3 - "$RESULTS_DIR/secondary_index.1.json" <<'PY'
import json,sys
try:
    obj=json.load(open(sys.argv[1], encoding='utf-8'))
    print(obj.get('project') or '')
except Exception:
    print('')
PY
)
    if [[ -z "$SECONDARY_PROJECT" ]]; then
      record_failure "secondary index did not return a project name"
    else
      run_once cross_repo 1 "$BINARY" index "$REPO" --mode cross-repo-intelligence \
        --target-projects "$SECONDARY_PROJECT" --json || true
    fi
  fi
fi

python3 - "$RESULTS_DIR/timings.tsv" > "$RESULTS_DIR/summary.tsv" <<'PY'
import csv, statistics, sys
from collections import defaultdict
rows=defaultdict(list)
with open(sys.argv[1], newline='', encoding='utf-8') as f:
    for r in csv.DictReader(f, delimiter='\t'):
        if r['exit_code']=='0': rows[r['case']].append(int(r['elapsed_ms']))
print('case\truns\tmin_ms\tmedian_ms\tmax_ms')
for case in sorted(rows):
    vals=rows[case]
    print(f"{case}\t{len(vals)}\t{min(vals)}\t{statistics.median(vals):g}\t{max(vals)}")
PY

{
  echo "captured_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "binary=$BINARY"
  "$BINARY" --version 2>/dev/null | sed 's/^/version=/' || true
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$BINARY" | awk '{print "binary_sha256=" $1}'
    sha256sum "$0" | awk '{print "harness_sha256=" $1}'
  fi
  echo "repo=$REPO"
  if git -C "$REPO" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    git -C "$REPO" rev-parse HEAD | sed 's/^/repo_commit=/'
    git -C "$REPO" status --porcelain=v1 | python3 -c 'import sys; print("repo_dirty=" + ("yes" if sys.stdin.read().strip() else "no"))'
    git -C "$REPO" remote get-url origin 2>/dev/null | sed 's/^/repo_origin=/' || true
  fi
  echo "project=$PROJECT"
  echo "repeats=$REPEATS"
  echo "operations=$OPERATIONS_CSV"
  echo "query=$QUERY"
  echo "symbol=$SYMBOL"
  echo "file=$FILE_PATH"
  echo "base_branch=$BASE_BRANCH"
  [[ -n "$SECONDARY_REPO" ]] && echo "secondary_repo=$SECONDARY_REPO"
  uname -a | sed 's/^/uname=/'
  if command -v lscpu >/dev/null 2>&1; then lscpu | sed 's/^/lscpu=/'; fi
  if [[ -r /proc/meminfo ]]; then grep -E '^(MemTotal|SwapTotal):' /proc/meminfo | sed 's/^/meminfo=/'; fi
  df -T "$REPO" "$RESULTS_DIR" 2>/dev/null | sed 's/^/filesystem=/' || true
  env | grep '^CBM_' | sort | sed 's/^/env=/' || true
} > "$RESULTS_DIR/environment.txt"

# Fail closed: every required repeated case must have exactly REPEATS successful measured runs.
for op in "${OPERATIONS[@]}"; do
  case "$op" in
    search|architecture|snippet|outline|changes|status)
      count=$(awk -F '\t' -v op="$op" 'NR>1 && $1==op && $4==0 {n++} END{print n+0}' "$RESULTS_DIR/timings.tsv")
      if [[ "$count" != "$REPEATS" ]]; then record_failure "$op successful measured runs=$count expected=$REPEATS"; fi
      ;;
    cross_repo)
      count=$(awk -F '\t' 'NR>1 && $1=="cross_repo" && $4==0 {n++} END{print n+0}' "$RESULTS_DIR/timings.tsv")
      if [[ "$count" != "1" ]]; then record_failure "cross_repo successful measured runs=$count expected=1"; fi
      ;;
  esac
done

if [[ -s "$FAILURES_FILE" ]]; then
  echo "error: benchmark workload failed; see $FAILURES_FILE" >&2
  cat "$FAILURES_FILE" >&2
  exit 4
fi
rm -f "$FAILURES_FILE"
printf 'PASS\n' > "$RESULTS_DIR/RESULT"
cat "$RESULTS_DIR/summary.tsv"
