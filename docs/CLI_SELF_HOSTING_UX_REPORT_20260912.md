# CLI self-hosting usability report — 2026-09-12

## Scope

Exercised the locally built `build/c/codebase-memory-cli` against this checkout.
The trial used a disposable directory outside the repository for both cache and
runtime state, with `--persistence false`; it created no new repository index
state.

```bash
trial_dir=$(mktemp -d)
mkdir -m 700 "$trial_dir/cache" "$trial_dir/run"
export CBM_CACHE_DIR="$trial_dir/cache"
export CBM_RUNTIME_DIR="$trial_dir/run"
build/c/codebase-memory-cli index . --mode moderate \
  --name codebase-memory-cli-ux --persistence false --json
```

## Result summary

| Area | Result | Notes |
|---|---|---|
| `--help` and command help | works | Clear command list and detailed option descriptions. |
| `doctor --deep --json` | works | Healthy after secure directories were pre-created; useful structured diagnosis. |
| Index | works | Moderate index completed; project appeared in `projects --json`. |
| Search | works | BM25 search returned ranked symbols with source location. |
| Architecture | works | Overview reported 20,646 nodes, 130,806 edges, a ten-language mix, and package rollups. |
| Outline, schema, snippet | works | JSON output is machine-friendly; ambiguous short names provide useful suggestions. |
| Cypher query | works | Simple node-count query returned expected rows. |
| Changes | works, with caveat | Correctly reports uncommitted files, but that can mask the previous-commit comparison. |
| Semantic search | accepts input; relevance needs work | The documented JSON-array form worked but `secure`/`endpoint` ranked unrelated cgroup tests. |

## First-run issue and recovery

The initial index attempt failed immediately with:

```text
codebase-memory-cli: secure CLI coordination could not be created (endpoint)
```

`doctor --deep --json` also showed an absent, insecure cache directory and an
unreachable daemon. The cause was the trial configuration: the runtime and
cache parent directories did not yet exist. Creating both beforehand with mode
`0700` resolved the condition; a subsequent deep doctor was `healthy` and the
index completed.

This is safe behavior, but the setup friction is high: a first-time user can
reasonably expect the CLI to create an owner-only directory or print the exact
recovery command in the failure itself. The current short `(endpoint)` label
does not identify the missing secure parent.

## Discovery observations

The core discovery loop is practical once a project name is known:

```bash
build/c/codebase-memory-cli search --project codebase-memory-cli-ux \
  --query 'secure endpoint' --format json --limit 5
build/c/codebase-memory-cli architecture --project codebase-memory-cli-ux --aspects overview
build/c/codebase-memory-cli outline --project codebase-memory-cli-ux --file-path src/main.c
build/c/codebase-memory-cli snippet --project codebase-memory-cli-ux \
  --qualified-name codebase-memory-cli-ux.src.main.main --include-neighbors true
```

`search`, `architecture`, and `outline` are easy to discover from help and
give compact, actionable output. `snippet main` deliberately refuses to guess
and supplies qualified-name choices; that is a good safety/usability tradeoff.
The `--project` flag is marked required in help even though its introductory
text says inference may be possible; consistently showing the current project
or accepting a session default would make the normal path clearer.

`query` is powerful but necessarily specialist-facing: it requires Cypher and
the separate `schema` lookup. This is appropriate, but it is not the easiest
entry point for ordinary source discovery.

`changes --since HEAD~1` included untracked workspace artifacts as changed
files and consequently found no indexed seed symbols. This is accurate for a
working-tree view but unexpected for a user who reads `--since` as a pure Git
commit comparison. The command should make the working-tree inclusion explicit
in its result/help, or offer a committed-only switch.

## Follow-up implemented

The immediate setup and discovery clarity fixes are now in the working tree:

1. A missing `CBM_RUNTIME_DIR` override is created with the existing secure,
   no-follow owner-private path. Existing directories remain fail-closed and
   are never re-owned or permission-modified.
2. Command help now consistently says that `--project` selects the indexed
   project, matching its required flag.
3. `changes` help and JSON output explicitly say that the working tree is
   included (`working_tree_included: true`).

Semantic ranking remains a separate relevance-quality task; no ranking
heuristic was changed based on one exploratory query.

No product changes were made by this exercise. The disposable cache/runtime
directory can be removed after the session.
