# Project index operations must preserve internal cache databases

> Spec `cbm-cache-stores` · version `0.1.0` · snapshot `SNAP-001` · current · draft

## Snapshot

- **Created:** 2026-10-02T04:42:29.288583Z
- **Sequence:** 1
- **Parent:** —
- **Digest:** sha256:2bf67a183790afff0b4055c0278acc12ece8ce7022b65700491be9c6d9948ab3
- **Authoring events:** —

## Intent

Project index operations must preserve internal cache databases

### Objectives

- Introduce or reuse one exact-filename classification of internal cache databases _config.db and _cross_repo.db and project index filenames with nonempty .db stems. Reject neither underscores nor config substrings generally.
- Apply shared classification to every project DB list/count/remove enumeration and cross-repo wildcard enumeration. Trace callers instead of fixing only the reset path.
- Reset only project databases under the existing explicit reset semantics; preserve internal database bytes and all internal sidecars/staged files.
- Preserve install/update/uninstall prompts, index counts, project enumeration order and errors outside corrected internal-store classification.
- Keep filename constants consistent with internal-store creation paths without changing schema, cache location or store ownership.

### Outcomes

- _None._

### Non-goals

- MCP adapter changes
- Remote push, PR merge or publication
- New Jenkins jobs
- Unrelated current PR14 CI repair
- Shared daemon stops, real-account cache deletion or graph reset

## Context

`{"base_commit":"5824239b9c37664f0a389bd1deb86ca1ac28a26c","base_ref":"release-tooling","graph_generation":"2026-09-29T06:21:14Z","graph_limitations":"Older primary-checkout graph is not exact-worktree coverage; src/cli/cli.c is partial/metadata_changed. Workers must confirm their own access and index/fallback; no negative claim from inherited graph.","graph_project":"lump-apps-codebase-memory-cli","graph_tier":"Verify with exact-source fallback","upstream_boundary":"0f52d30c","upstream_sources":["46015320"]}`

## Scope

### Included

- src/foundation/str_util.c
- src/foundation/str_util.h
- src/cli/cli.c
- src/cli/cli.h
- src/pipeline/pass_cross_repo.c
- tests/test_cli.c
- tests/test_pipeline.c
- tests/test_str_util.c

### Excluded

- src/mcp/**
- Primary dirty checkout
- Unrelated release/packaging changes

### Protected

- _None._

### Constraints

- _None._

## Requirements

### `REQ-001` — Introduce or reuse one exact-filename classification of internal cache databases _config.db and _cross_repo.db and project index filenames with nonempty .db stems. Reject neither underscores nor config substrings generally.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Introduce or reuse one exact-filename classification of internal cache databases _config.db and _cross_repo.db and project index filenames with nonempty .db stems. Reject neither underscores nor config substrings generally.
- **Type:** functional
- **Verification Refs:** AC-001, EVAL-001

### `REQ-002` — Apply shared classification to every project DB list/count/remove enumeration and cross-repo wildcard enumeration. Trace callers instead of fixing only the reset path.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Apply shared classification to every project DB list/count/remove enumeration and cross-repo wildcard enumeration. Trace callers instead of fixing only the reset path.
- **Type:** functional
- **Verification Refs:** AC-002, EVAL-002

### `REQ-003` — Reset only project databases under the existing explicit reset semantics; preserve internal database bytes and all internal sidecars/staged files.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Reset only project databases under the existing explicit reset semantics; preserve internal database bytes and all internal sidecars/staged files.
- **Type:** functional
- **Verification Refs:** AC-003, EVAL-003

### `REQ-004` — Preserve install/update/uninstall prompts, index counts, project enumeration order and errors outside corrected internal-store classification.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Preserve install/update/uninstall prompts, index counts, project enumeration order and errors outside corrected internal-store classification.
- **Type:** functional
- **Verification Refs:** AC-004, EVAL-004

### `REQ-005` — Keep filename constants consistent with internal-store creation paths without changing schema, cache location or store ownership.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Keep filename constants consistent with internal-store creation paths without changing schema, cache location or store ownership.
- **Type:** functional
- **Verification Refs:** AC-005, EVAL-005

## Interfaces

_None._

## Data contracts

_None._

## Decisions

_None._

## Acceptance criteria

### `AC-001` — Table-driven predicate coverage includes both internal names, .db, ordinary.db, _private.db, orders_config_service.db, api-wal.db and non-.db names.

- **Criterion:** Table-driven predicate coverage includes both internal names, .db, ordinary.db, _private.db, orders_config_service.db, api-wal.db and non-.db names.
- **Requirement Ids:** REQ-001
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites cli,pipeline; full union gate before push.

### `AC-002` — All discovered enumeration sites reuse the classification; list/count outputs exclude internal stores while including similarly named legitimate projects.

- **Criterion:** All discovered enumeration sites reuse the classification; list/count outputs exclude internal stores while including similarly named legitimate projects.
- **Requirement Ids:** REQ-002
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites cli,pipeline; full union gate before push.

### `AC-003` — A disposable fixture reset retains byte-identical _config.db/_cross_repo.db and their -wal/-shm/-journal/.tmp artifacts while removing intended project DBs and project sidecars.

- **Criterion:** A disposable fixture reset retains byte-identical _config.db/_cross_repo.db and their -wal/-shm/-journal/.tmp artifacts while removing intended project DBs and project sidecars.
- **Requirement Ids:** REQ-003
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites cli,pipeline; full union gate before push.

### `AC-004` — Existing lifecycle and reset tests pass; counts/prompt assertions reflect only project indexes and repeated cleanup is idempotent.

- **Criterion:** Existing lifecycle and reset tests pass; counts/prompt assertions reflect only project indexes and repeated cleanup is idempotent.
- **Requirement Ids:** REQ-004
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites cli,pipeline; full union gate before push.

### `AC-005` — Internal stores are opened at the same paths; source and regression checks reveal no schema/path/public API drift.

- **Criterion:** Internal stores are opened at the same paths; source and regression checks reveal no schema/path/public API drift.
- **Requirement Ids:** REQ-005
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites cli,pipeline; full union gate before push.

## Evaluations

### `EVAL-001` — Evaluations

- **Acceptance Ids:** AC-001
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-001
- **Suite Selection:** cli,pipeline
- **Venue:** existing shared Jenkins job only

### `EVAL-002` — Evaluations

- **Acceptance Ids:** AC-002
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-002
- **Suite Selection:** cli,pipeline
- **Venue:** existing shared Jenkins job only

### `EVAL-003` — Evaluations

- **Acceptance Ids:** AC-003
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-003
- **Suite Selection:** cli,pipeline
- **Venue:** existing shared Jenkins job only

### `EVAL-004` — Evaluations

- **Acceptance Ids:** AC-004
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-004
- **Suite Selection:** cli,pipeline
- **Venue:** existing shared Jenkins job only

### `EVAL-005` — Evaluations

- **Acceptance Ids:** AC-005
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-005
- **Suite Selection:** cli,pipeline
- **Venue:** existing shared Jenkins job only

## Implementation tasks

### `TASK-IMPLEMENT` — Project index operations must preserve internal cache databases

- **Dependencies:** —
- **Phase Id:** PHASE-IMPLEMENT
- **Requirement Ids:** REQ-001, REQ-002, REQ-003, REQ-004, REQ-005
- **Result Contract:** `{"required_evidence":["exact base and final commit","source adaptation map","changed paths","diff/format checks","Jenkins suites requested but not locally run","limitations and review risks"],"schema":"agent-workflow/task-result/v1"}`

## Implementation phases

### `PHASE-IMPLEMENT` — Discover, adapt and commit scoped implementation


### `PHASE-VERIFY` — Parent integrates, evaluates in Jenkins, independently reviews and accepts


## Risks

### `RISK-001` — Exact-name exclusions are required. Broad underscore/config filters silently hide valid projects; fixture tests must never touch the real account cache.


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
