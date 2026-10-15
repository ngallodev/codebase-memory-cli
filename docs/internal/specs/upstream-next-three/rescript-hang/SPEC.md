# Bound ReScript indexing and reject binary resource files

> Spec `cbm-rescript-hang` · version `0.1.0` · snapshot `SNAP-001` · current · draft

## Snapshot

- **Created:** 2026-10-02T04:42:30.799454Z
- **Sequence:** 1
- **Parent:** —
- **Digest:** sha256:a7e436fa6c09ff8ddd675e418cd4a5bbd728e0180127618dfc3bf8e47440a0cc
- **Authoring events:** —

## Intent

Bound ReScript indexing and reject binary resource files

### Objectives

- Disambiguate .res binary Godot/Windows resources from valid ReScript source at discovery admission, using bounded content checks and existing language detection/error conventions.
- Use the existing occurrence/walk cursor for nearest-container ancestry where the walk already supplies cursor state; preserve the fallback only for real out-of-walk callers.
- Port scanner progress/termination corrections with the traversal/admission changes; every malformed input/EOF path must terminate.
- Preserve Julia, Typst, Elm, PureScript, Nickel and other callers sharing is_first_named_part_of and policy binding logic.
- Update the repository actual vendored checksums/manifests for precisely the changed scanner, adapting ea6fa49f without copying unrelated upstream checksums.

### Outcomes

- _None._

### Non-goals

- MCP adapter changes
- Remote push, PR merge or publication
- New Jenkins jobs
- Unrelated current PR14 CI repair
- Shared daemon stops, real-account cache deletion or graph reset

## Context

`{"base_commit":"5824239b9c37664f0a389bd1deb86ca1ac28a26c","base_ref":"release-tooling","graph_generation":"2026-09-29T06:21:14Z","graph_limitations":"Older primary-checkout graph is not exact-worktree coverage; src/cli/cli.c is partial/metadata_changed. Workers must confirm their own access and index/fallback; no negative claim from inherited graph.","graph_project":"lump-apps-codebase-memory-cli","graph_tier":"Verify with exact-source fallback","upstream_boundary":"0f52d30c","upstream_sources":["8093d10b","ea6fa49f"]}`

## Scope

### Included

- src/discover/discover.c
- src/discover/discover.h
- src/discover/language.c
- internal/cbm/extract_usages.c
- internal/cbm/vendored/grammars/rescript/scanner.c
- internal/cbm/vendored/grammars/MANIFEST.md
- tests/test_language.c
- tests/test_extraction.c
- tests/test_complexity.c

### Excluded

- src/mcp/**
- Primary dirty checkout
- Unrelated release/packaging changes

### Protected

- _None._

### Constraints

- _None._

## Requirements

### `REQ-001` — Disambiguate .res binary Godot/Windows resources from valid ReScript source at discovery admission, using bounded content checks and existing language detection/error conventions.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Disambiguate .res binary Godot/Windows resources from valid ReScript source at discovery admission, using bounded content checks and existing language detection/error conventions.
- **Type:** functional
- **Verification Refs:** AC-001, EVAL-001

### `REQ-002` — Use the existing occurrence/walk cursor for nearest-container ancestry where the walk already supplies cursor state; preserve the fallback only for real out-of-walk callers.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Use the existing occurrence/walk cursor for nearest-container ancestry where the walk already supplies cursor state; preserve the fallback only for real out-of-walk callers.
- **Type:** functional
- **Verification Refs:** AC-002, EVAL-002

### `REQ-003` — Port scanner progress/termination corrections with the traversal/admission changes; every malformed input/EOF path must terminate.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Port scanner progress/termination corrections with the traversal/admission changes; every malformed input/EOF path must terminate.
- **Type:** functional
- **Verification Refs:** AC-003, EVAL-003

### `REQ-004` — Preserve Julia, Typst, Elm, PureScript, Nickel and other callers sharing is_first_named_part_of and policy binding logic.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Preserve Julia, Typst, Elm, PureScript, Nickel and other callers sharing is_first_named_part_of and policy binding logic.
- **Type:** functional
- **Verification Refs:** AC-004, EVAL-004

### `REQ-005` — Update the repository actual vendored checksums/manifests for precisely the changed scanner, adapting ea6fa49f without copying unrelated upstream checksums.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Update the repository actual vendored checksums/manifests for precisely the changed scanner, adapting ea6fa49f without copying unrelated upstream checksums.
- **Type:** functional
- **Verification Refs:** AC-005, EVAL-005

## Interfaces

_None._

## Data contracts

_None._

## Decisions

_None._

## Acceptance criteria

### `AC-001` — Binary .res fixtures are excluded from ReScript text parsing; valid .res source remains admitted; short/empty/unreadable input handling is bounded and consistent.

- **Criterion:** Binary .res fixtures are excluded from ReScript text parsing; valid .res source remains admitted; short/empty/unreadable input handling is bounded and consistent.
- **Requirement Ids:** REQ-001
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites language,extraction,complexity; full union gate before push.

### `AC-002` — Error-heavy wide/deep parse fixture pins cursor-path usage or bounded parent-fallback counts, and extraction remains deterministic for all occurrence policies sharing the helper.

- **Criterion:** Error-heavy wide/deep parse fixture pins cursor-path usage or bounded parent-fallback counts, and extraction remains deterministic for all occurrence policies sharing the helper.
- **Requirement Ids:** REQ-002
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites language,extraction,complexity; full union gate before push.

### `AC-003` — A bounded regression for malformed/binary-like source detects a hang without hanging the entire Jenkins runner; valid ReScript definitions/calls remain unchanged.

- **Criterion:** A bounded regression for malformed/binary-like source detects a hang without hanging the entire Jenkins runner; valid ReScript definitions/calls remain unchanged.
- **Requirement Ids:** REQ-003
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites language,extraction,complexity; full union gate before push.

### `AC-004` — Trace/caller evidence plus representative existing extraction/complexity fixtures verify binding classification and ordering are unchanged.

- **Criterion:** Trace/caller evidence plus representative existing extraction/complexity fixtures verify binding classification and ordering are unchanged.
- **Requirement Ids:** REQ-004
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites language,extraction,complexity; full union gate before push.

### `AC-005` — Existing vendored-integrity contract matches actual scanner bytes; no unrelated grammar/dependency change.

- **Criterion:** Existing vendored-integrity contract matches actual scanner bytes; no unrelated grammar/dependency change.
- **Requirement Ids:** REQ-005
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites language,extraction,complexity; full union gate before push.

## Evaluations

### `EVAL-001` — Evaluations

- **Acceptance Ids:** AC-001
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-001
- **Suite Selection:** language,extraction,complexity
- **Venue:** existing shared Jenkins job only

### `EVAL-002` — Evaluations

- **Acceptance Ids:** AC-002
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-002
- **Suite Selection:** language,extraction,complexity
- **Venue:** existing shared Jenkins job only

### `EVAL-003` — Evaluations

- **Acceptance Ids:** AC-003
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-003
- **Suite Selection:** language,extraction,complexity
- **Venue:** existing shared Jenkins job only

### `EVAL-004` — Evaluations

- **Acceptance Ids:** AC-004
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-004
- **Suite Selection:** language,extraction,complexity
- **Venue:** existing shared Jenkins job only

### `EVAL-005` — Evaluations

- **Acceptance Ids:** AC-005
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-005
- **Suite Selection:** language,extraction,complexity
- **Venue:** existing shared Jenkins job only

## Implementation tasks

### `TASK-IMPLEMENT` — Bound ReScript indexing and reject binary resource files

- **Dependencies:** —
- **Phase Id:** PHASE-IMPLEMENT
- **Requirement Ids:** REQ-001, REQ-002, REQ-003, REQ-004, REQ-005
- **Result Contract:** `{"required_evidence":["exact base and final commit","source adaptation map","changed paths","diff/format checks","Jenkins suites requested but not locally run","limitations and review risks"],"schema":"agent-workflow/task-result/v1"}`

## Implementation phases

### `PHASE-IMPLEMENT` — Discover, adapt and commit scoped implementation


### `PHASE-VERIFY` — Parent integrates, evaluates in Jenkins, independently reviews and accepts


## Risks

### `RISK-001` — A binary admission guard alone leaves malformed text traversal/scanner hangs. Worker must handle all three seams and preserve vendored integrity.


## Unresolved questions

_None._

## Preservation claims

### `PRES-001` — Maintain local release-tooling integration, existing shared Jenkins validation and deferred publication; no direct main push, rebase or force-push. No local tests/builds.

- **Source Ref:** SRC-USER
- **Statement:** Maintain local release-tooling integration, existing shared Jenkins validation and deferred publication; no direct main push, rebase or force-push. No local tests/builds.
- **Status:** represented
- **Target Refs:** REQ-001, REQ-002, REQ-003, REQ-004, REQ-005

## Provenance sources

### `SRC-USER` — Provenance sources

- **Description:** Implement the three selected verticals with GPT-6-Luna, full permissions, isolated worktrees and interactive Herdr panes.
- **Kind:** user_input

### `SRC-BASE` — Provenance sources

- **Description:** Exact release-tooling source baseline and selected upstream diffs
- **Kind:** repository
- **Locator:** /tmp/claude-1000/-lump-apps-codebase-memory-cli/70854de5-bd44-40b4-8ac2-4cf2d6b24d01/scratchpad/wt-rt
- **Revision:** 5824239b9c37664f0a389bd1deb86ca1ac28a26c

## Extensions

`{"delivery":{"interactive_host":"Herdr; launch pending managed-session context","production_branch":"main","promotion":"Future origin PR targets main after QA passing; no automatic merge/tag/release.","push_gate":"Exact-SHA shared Jenkins required before remote push.","qa_branch":"release-tooling","review":"Worker commits are candidates; parent evaluation/review/acceptance remain separate.","sync_policy":"Frozen shared QA baseline; parent reconciles with normal merges, never rewrites published history.","version_gate":"No version/tag changes for these compatible fixes.","worker_permissions":"danger-full-access; approval never"}}`

## Traceability

- `REQ-001` — **verified_by** → `AC-001`
- `AC-001` — **evaluated_by** → `EVAL-001`
- `REQ-001` — **implemented_by** → `TASK-IMPLEMENT`
- `REQ-002` — **verified_by** → `AC-002`
- `AC-002` — **evaluated_by** → `EVAL-002`
- `REQ-002` — **implemented_by** → `TASK-IMPLEMENT`
- `REQ-003` — **verified_by** → `AC-003`
- `AC-003` — **evaluated_by** → `EVAL-003`
- `REQ-003` — **implemented_by** → `TASK-IMPLEMENT`
- `REQ-004` — **verified_by** → `AC-004`
- `AC-004` — **evaluated_by** → `EVAL-004`
- `REQ-004` — **implemented_by** → `TASK-IMPLEMENT`
- `REQ-005` — **verified_by** → `AC-005`
- `AC-005` — **evaluated_by** → `EVAL-005`
- `REQ-005` — **implemented_by** → `TASK-IMPLEMENT`
