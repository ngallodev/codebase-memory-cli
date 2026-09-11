#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: run-linux-benchmark-corpus.sh --binary PATH [options]

Options:
  --manifest PATH              Corpus manifest (default docs/qualification/BENCHMARK_CORPUS.json)
  --workspace-root PATH        Repo checkout root (default /tmp/cbm-benchmark/repos)
  --results-root PATH          Results root (default /tmp/cbm-benchmark/results)
  --codebase-memory-ref REF    Ref used to freeze codebase-memory-cli when manifest is unpinned
  --frozen-manifest-out PATH   Output manifest when --initialize is used
  --repeats N                  Measured repetitions per repeated operation (default 5)
  --initialize                 Clone/fetch/check out initialization refs and freeze exact SHAs
  -h, --help                   Show this help
USAGE
}

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd -P)
MANIFEST="$REPO_ROOT/docs/qualification/BENCHMARK_CORPUS.json"
WORKSPACE_ROOT="/tmp/cbm-benchmark/repos"
RESULTS_ROOT="/tmp/cbm-benchmark/results"
CODEBASE_MEMORY_REF=""
FROZEN_MANIFEST_OUT=""
REPEATS=5
INITIALIZE=0
BINARY=""

while (($#)); do
  case "$1" in
    --binary) BINARY="${2:?missing value}"; shift 2 ;;
    --manifest) MANIFEST="${2:?missing value}"; shift 2 ;;
    --workspace-root) WORKSPACE_ROOT="${2:?missing value}"; shift 2 ;;
    --results-root) RESULTS_ROOT="${2:?missing value}"; shift 2 ;;
    --codebase-memory-ref) CODEBASE_MEMORY_REF="${2:?missing value}"; shift 2 ;;
    --frozen-manifest-out) FROZEN_MANIFEST_OUT="${2:?missing value}"; shift 2 ;;
    --repeats) REPEATS="${2:?missing value}"; shift 2 ;;
    --initialize) INITIALIZE=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "error: unknown argument: $1" >&2; usage >&2; exit 2 ;;
  esac
done

[[ -n "$BINARY" ]] || { echo "error: --binary is required" >&2; exit 2; }
command -v git >/dev/null || { echo "error: git is required" >&2; exit 2; }
command -v python3 >/dev/null || { echo "error: python3 is required" >&2; exit 2; }
[[ "$REPEATS" =~ ^[1-9][0-9]*$ ]] || { echo "error: --repeats must be >= 1" >&2; exit 2; }

BINARY=$(cd "$(dirname "$BINARY")" && pwd -P)/$(basename "$BINARY")
[[ -x "$BINARY" ]] || { echo "error: binary is not executable: $BINARY" >&2; exit 2; }
MANIFEST=$(cd "$(dirname "$MANIFEST")" && pwd -P)/$(basename "$MANIFEST")
[[ -f "$MANIFEST" ]] || { echo "error: manifest not found: $MANIFEST" >&2; exit 2; }
mkdir -p "$WORKSPACE_ROOT" "$RESULTS_ROOT"
WORKSPACE_ROOT=$(cd "$WORKSPACE_ROOT" && pwd -P)
RESULTS_ROOT=$(cd "$RESULTS_ROOT" && pwd -P)
if (( INITIALIZE )) && [[ -z "$FROZEN_MANIFEST_OUT" ]]; then
  FROZEN_MANIFEST_OUT="$RESULTS_ROOT/BENCHMARK_CORPUS.frozen.json"
fi

# Emit tab-separated manifest rows safely without requiring jq.
mapfile -t ROWS < <(python3 - "$MANIFEST" <<'PY'
import json,sys
m=json.load(open(sys.argv[1], encoding='utf-8'))
for r in m['repositories']:
    w=r.get('workload') or {}
    fields=[r['id'],r['remote'],str(r['commit']),str(r.get('initialization_ref','')),
            ','.join(r.get('operations') or []),str(w.get('query','daemon')),str(w.get('symbol','main')),
            str(w.get('file_path','src/main.c')),str(w.get('base_branch','main'))]
    if any('\t' in f or '\n' in f for f in fields): raise SystemExit('manifest field contains tab/newline')
    print('\t'.join(fields))
PY
)

# Build a mutable frozen manifest in a temporary file when initializing.
TMP_MANIFEST=""
if (( INITIALIZE )); then
  TMP_MANIFEST=$(mktemp)
  cp "$MANIFEST" "$TMP_MANIFEST"
fi

for row in "${ROWS[@]}"; do
  IFS=$'\t' read -r id remote commit init_ref operations query symbol file_path base_branch <<< "$row"
  path="$WORKSPACE_ROOT/$id"
  if [[ ! -e "$path" ]]; then
    git clone --no-checkout "$remote" "$path"
  fi
  git -C "$path" rev-parse --git-dir >/dev/null 2>&1 || { echo "error: not a git checkout: $path" >&2; exit 3; }
  git -C "$path" remote set-url origin "$remote"
  git -C "$path" fetch --tags --prune origin

  ref="$commit"
  if [[ "$ref" == "TO_BE_PINNED_FROM_FROZEN_CHECKOUT" ]]; then
    if [[ "$id" == "codebase-memory-cli" && -n "$CODEBASE_MEMORY_REF" ]]; then ref="$CODEBASE_MEMORY_REF"
    elif [[ -n "$init_ref" ]]; then ref="$init_ref"
    else echo "error: no initialization ref for $id" >&2; exit 3
    fi
  fi
  checkout_ref="$ref"
  if git -C "$path" show-ref --verify --quiet "refs/remotes/origin/$ref"; then checkout_ref="refs/remotes/origin/$ref"; fi
  resolved=$(git -C "$path" rev-parse "$checkout_ref^{commit}" 2>/dev/null) || { echo "error: cannot resolve $id ref=$checkout_ref" >&2; exit 3; }
  git -C "$path" checkout --detach "$resolved"
  git -C "$path" reset --hard "$resolved" >/dev/null
  git -C "$path" clean -ffd >/dev/null
  [[ -z "$(git -C "$path" status --porcelain)" ]] || { echo "error: checkout is dirty after reset/clean: $id" >&2; exit 3; }

  if (( INITIALIZE )); then
    python3 - "$TMP_MANIFEST" "$id" "$resolved" "$path" <<'PY'
import json,sys
p,id_,sha,path=sys.argv[1:]
m=json.load(open(p, encoding='utf-8'))
for r in m['repositories']:
    if r['id']==id_:
        r['commit']=sha; r['local_path']=path; break
else: raise SystemExit(f'unknown repo id {id_}')
with open(p,'w',encoding='utf-8',newline='\n') as f: json.dump(m,f,indent=2); f.write('\n')
PY
  fi
done

EFFECTIVE_MANIFEST="$MANIFEST"
if (( INITIALIZE )); then
  mkdir -p "$(dirname "$FROZEN_MANIFEST_OUT")"
  mv "$TMP_MANIFEST" "$FROZEN_MANIFEST_OUT"
  EFFECTIVE_MANIFEST=$(cd "$(dirname "$FROZEN_MANIFEST_OUT")" && pwd -P)/$(basename "$FROZEN_MANIFEST_OUT")
fi
python3 "$SCRIPT_DIR/validate-corpus-checkouts.py" "$EFFECTIVE_MANIFEST"

STAMP=$(date -u +%Y%m%dT%H%M%SZ)
RUN_ROOT="$RESULTS_ROOT/$STAMP"
mkdir -p "$RUN_ROOT"
CORPUS_RESULTS="$RUN_ROOT/corpus-results.json"
python3 - "$EFFECTIVE_MANIFEST" "$CORPUS_RESULTS" <<'PY'
import json,sys
m=json.load(open(sys.argv[1],encoding='utf-8'))
json.dump({'generation':m.get('generation'),'repositories':[]},open(sys.argv[2],'w',encoding='utf-8'),indent=2)
PY

mapfile -t RUN_ROWS < <(python3 - "$EFFECTIVE_MANIFEST" <<'PY'
import json,sys
m=json.load(open(sys.argv[1],encoding='utf-8'))
for r in m['repositories']:
 w=r.get('workload') or {}
 fields=[r['id'],r['local_path'],r['commit'],','.join(r.get('operations') or []),str(w.get('query','daemon')),str(w.get('symbol','main')),str(w.get('file_path','src/main.c')),str(w.get('base_branch','main'))]
 print('\t'.join(fields))
PY
)

for row in "${RUN_ROWS[@]}"; do
  IFS=$'\t' read -r id path commit operations query symbol file_path base_branch <<< "$row"
  secondary=""
  if [[ ",$operations," == *",cross_repo,"* ]]; then
    secondary=$(python3 - "$EFFECTIVE_MANIFEST" "$id" <<'PY'
import json,sys
m=json.load(open(sys.argv[1],encoding='utf-8')); id_=sys.argv[2]
repos={r['id']:r for r in m['repositories']}
choice=repos['codebase-memory-cli']['local_path']
if repos[id_]['local_path']==choice: choice=repos['agent-workflow']['local_path']
print(choice)
PY
)
  fi
  out="$RUN_ROOT/$id"
  CBM_BENCH_REPEATS="$REPEATS" \
  CBM_BENCH_OPERATIONS="$operations" \
  CBM_BENCH_QUERY="$query" \
  CBM_BENCH_SYMBOL="$symbol" \
  CBM_BENCH_FILE="$file_path" \
  CBM_BENCH_BASE_BRANCH="$base_branch" \
    "$REPO_ROOT/scripts/benchmark-agent-workflows.sh" "$BINARY" "$path" "$out" "$secondary"
  [[ -f "$out/RESULT" && "$(cat "$out/RESULT")" == "PASS" ]] || { echo "error: benchmark did not produce PASS: $id" >&2; exit 4; }
  python3 - "$CORPUS_RESULTS" "$id" "$commit" "$out" <<'PY'
import json,sys
p,id_,sha,out=sys.argv[1:]
d=json.load(open(p,encoding='utf-8'))
d['repositories'].append({'id':id_,'commit':sha,'result':'PASS','results':out})
with open(p,'w',encoding='utf-8',newline='\n') as f: json.dump(d,f,indent=2); f.write('\n')
PY
done

python3 - "$EFFECTIVE_MANIFEST" "$CORPUS_RESULTS" <<'PY'
import json,sys
m=json.load(open(sys.argv[1],encoding='utf-8')); r=json.load(open(sys.argv[2],encoding='utf-8'))
expected=[x['id'] for x in m['repositories']]; got=[x['id'] for x in r['repositories'] if x.get('result')=='PASS']
if got!=expected: raise SystemExit(f'corpus incomplete: expected {expected}, got {got}')
PY
printf 'PASS\n' > "$RUN_ROOT/RESULT"
printf 'Benchmark corpus complete: %s\nFrozen manifest: %s\n' "$RUN_ROOT" "$EFFECTIVE_MANIFEST"
