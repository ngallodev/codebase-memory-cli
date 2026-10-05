#!/usr/bin/env bash
# Real CLI/daemon regression: outer --json preserves payload JSON bytes.
set -euo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
binary="${CBM_TEST_BINARY:-$repo_root/build/c/codebase-memory-cli}"
source "$repo_root/scripts/test-runtime.sh"
cbm_test_runtime_init
trap 'cbm_test_runtime_cleanup "$binary"' EXIT
mkdir "$CBM_TEST_RUNTIME_ROOT/project"
for n in {1..24}; do
  printf 'def marker%s():\n    return %s\n' "$n" "$n" >> "$CBM_TEST_RUNTIME_ROOT/project/main.py"
done
export CBM_ALLOWED_ROOT="$CBM_TEST_RUNTIME_ROOT/project"
cd "$CBM_TEST_RUNTIME_ROOT/project"
"$binary" index . --mode fast --persistence false --json > "$CBM_TEST_RUNTIME_ROOT/index.json"
"$binary" projects > "$CBM_TEST_RUNTIME_ROOT/projects.tree"
"$binary" projects --json > "$CBM_TEST_RUNTIME_ROOT/projects.json"
"$binary" search --name-pattern marker --limit 24 > "$CBM_TEST_RUNTIME_ROOT/search.tree"
"$binary" search --name-pattern marker --limit 24 --json > "$CBM_TEST_RUNTIME_ROOT/search.json"
"$binary" search --name-pattern marker --limit 24 --max-output-tokens 128 > "$CBM_TEST_RUNTIME_ROOT/budget.tree"
"$binary" search --name-pattern marker --limit 24 --max-output-tokens 128 --json > "$CBM_TEST_RUNTIME_ROOT/budget.json"
python3 - "$CBM_TEST_RUNTIME_ROOT" <<'PY'
import json, pathlib, sys
root = pathlib.Path(sys.argv[1])
projects = (root/'projects.tree').read_text()
search = (root/'search.tree').read_text()
budget = (root/'budget.tree').read_text()
assert 'projects:' in projects
assert 'results:' in search and 'marker24' in search
assert 'output_budget' in budget and len(budget.encode()) <= 512
assert len(json.loads((root/'projects.json').read_text())['projects']) == 1
assert json.loads((root/'search.json').read_text())['count'] == 24
assert len((root/'budget.json').read_bytes().rstrip(b'\n')) <= 512
assert json.loads((root/'budget.json').read_text())['truncation_reason'] == 'output_budget'
print('CLI/daemon smoke: tree, JSON, and 128-token budget passed')
PY
