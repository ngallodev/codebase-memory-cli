#!/usr/bin/env bash
# benchmark-search-graph.sh — Time canonical CLI graph-search cases against a
# codebase-memory-cli binary to measure regex / BM25 search performance.
#
# Usage:
#   scripts/benchmark-search-graph.sh <binary-path> <project-name>

set -euo pipefail

BINARY="${1:?Usage: $0 <binary-path> <repo-path>}"
REPO="${2:?Usage: $0 <binary-path> <repo-path>}"
REPO=$(cd "$REPO" && pwd -P)

# shellcheck source=test-runtime.sh
source "$(dirname "${BASH_SOURCE[0]}")/test-runtime.sh"
cbm_test_runtime_init
BENCH_TMP=""
trap 'cbm_test_runtime_cleanup "$BINARY"; [ -z "$BENCH_TMP" ] || rm -rf -- "$BENCH_TMP"' EXIT
BENCH_TMP=$(mktemp -d)
INDEX_ERR="$BENCH_TMP/index-stderr.log"
DAEMON_START_LOG="$BENCH_TMP/daemon-start.log"

# Its output is kept for the failure path: the product names why a daemon did
# not start, and that is the part an operator needs.
if ! "$BINARY" daemon start >"$DAEMON_START_LOG" 2>&1; then
    echo "private daemon did not start" >&2
    if [ -s "$DAEMON_START_LOG" ]; then
        echo "--- daemon start output ---" >&2
        cat "$DAEMON_START_LOG" >&2
    fi
    exit 1
fi
# One pre-escaped spelling of the path, the way the soak harness builds its
# own: a repository path may legitimately contain a quote or a backslash, and
# hand-built JSON turns that into a parse error.
REPO_JSON=$(python3 -c 'import json,sys; print(json.dumps(sys.argv[1]))' "$REPO")
# Index and parse keep their stderr instead of discarding it: without it the
# failure below names only its symptom, and the cause — an unreadable
# repository, a refused daemon, a malformed envelope — is unrecoverable.
INDEX_JSON=$("$BINARY" cli index_repository "{\"repo_path\":$REPO_JSON,\"mode\":\"full\"}" \
    2>"$INDEX_ERR" || echo '{}')
PROJECT=$(echo "$INDEX_JSON" | python3 -c "
import json, sys
d = json.load(sys.stdin)
if 'content' in d:
    d = json.loads(d['content'][0]['text'])
print(d.get('project', ''))
" 2>>"$INDEX_ERR" || echo "")
if [ -z "$PROJECT" ]; then
    echo "index of $REPO did not report a project" >&2
    if [ -s "$INDEX_ERR" ]; then
        echo "--- index/parse stderr ---" >&2
        cat "$INDEX_ERR" >&2
    fi
    if [ -n "$INDEX_JSON" ]; then
        echo "--- index response (first 500 bytes) ---" >&2
        printf '%.500s\n' "$INDEX_JSON" >&2
    fi
    exit 1
fi

echo "Binary:  $BINARY"
echo "Project: $PROJECT (indexed from $REPO)"
echo ""

run_case() {
    local label="$1"; shift
    local start end elapsed_ms result count

    start=$(date +%s%3N)
    result=$("$BINARY" search --project "$PROJECT" --json "$@" 2>/dev/null || true)
    end=$(date +%s%3N)
    elapsed_ms=$(( end - start ))

    count=$(printf '%s' "$result" | python3 -c '
import json, sys
try:
    obj = json.load(sys.stdin)
    print(obj.get("total", obj.get("count", "?")))
except Exception:
    print("?")
' 2>/dev/null || echo "?")

    printf "  %-55s %5dms  (total=%s)\n" "$label" "$elapsed_ms" "$count"
}

echo "=== search name-pattern benchmarks ==="
run_case "name_pattern=.*Controller.*"         --name-pattern '.*Controller.*' --limit 20
run_case "name_pattern=.*Service.*"            --name-pattern '.*Service.*' --limit 20
run_case "name_pattern=.*Repository.*"         --name-pattern '.*Repository.*' --limit 20
run_case "name_pattern=specificFunctionName"   --name-pattern 'specificFunctionName' --limit 20
run_case "label=Method + name_pattern=.*get.*" --label Method --name-pattern '.*get.*' --limit 20

echo ""
echo "=== search query benchmarks (BM25 path) ==="
run_case "query=controller service handler"                  --query 'controller service handler' --limit 20
run_case "query=user authentication permission role"         --query 'user authentication permission role' --limit 20
run_case "query=create update delete manage list view admin" --query 'create update delete manage list view admin' --limit 20
