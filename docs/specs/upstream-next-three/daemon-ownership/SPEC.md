# Activation must preserve foreign and unconfirmed daemon sessions

> Spec `cbm-daemon-ownership` · version `0.1.0` · snapshot `SNAP-001` · current · draft

## Snapshot

- **Created:** 2026-10-02T04:42:29.977029Z
- **Sequence:** 1
- **Parent:** —
- **Digest:** sha256:67a04f2e0c25e7da4071a742025f254bf3837ced67289863dbed9b19d1532ed2
- **Authoring events:** —

## Intent

Activation must preserve foreign and unconfirmed daemon sessions

### Objectives

- Before eager shutdown or quiescence shutdown, establish a positive match between the active daemon/cohort cache identity and the activation target cache. Never infer ownership from an absent identity or a busy observation.
- Adapt unknown-is-foreign behavior to the fork transaction lifecycle rather than copying upstream production reserve/scope code unchanged. Account for every eager and callback shutdown route and relevant install/update/uninstall/reset caller.
- Preserve exact same-cache install/update/uninstall behavior, exclusive mutation authority, lock ordering, bounded waits, cleanup and truthful audit outcomes. A skipped shutdown must not grant authority to mutate a foreign namespace.
- Close time-of-check/time-of-use gaps through the existing cohort reservation/identity contracts. If the ownership identity changes between observation and reservation/callback, refuse or skip without stopping the changed cohort.
- Retain existing wire/API compatibility and user-visible CLI semantics except accurate diagnostics for preserved foreign/unknown sessions.

### Outcomes

- _None._

### Non-goals

- MCP adapter changes
- Remote push, PR merge or publication
- New Jenkins jobs
- Unrelated current PR14 CI repair
- Shared daemon stops, real-account cache deletion or graph reset

## Context

`{"base_commit":"5824239b9c37664f0a389bd1deb86ca1ac28a26c","base_ref":"release-tooling","graph_generation":"2026-09-29T06:21:14Z","graph_limitations":"Older primary-checkout graph is not exact-worktree coverage; src/cli/cli.c is partial/metadata_changed. Workers must confirm their own access and index/fallback; no negative claim from inherited graph.","graph_project":"lump-apps-codebase-memory-cli","graph_tier":"Verify with exact-source fallback","upstream_boundary":"0f52d30c","upstream_sources":["ed76cf8e"]}`

## Scope

### Included

- src/cli/cli.c
- src/cli/cli.h
- src/cli/activation_transaction.c
- src/daemon/runtime.c
- src/daemon/version_cohort.c
- tests/test_cli.c
- tests/test_daemon_runtime.c
- tests/test_daemon_ipc.c

### Excluded

- src/mcp/**
- Primary dirty checkout
- Unrelated release/packaging changes

### Protected

- _None._

### Constraints

- _None._

## Requirements

### `REQ-001` — Before eager shutdown or quiescence shutdown, establish a positive match between the active daemon/cohort cache identity and the activation target cache. Never infer ownership from an absent identity or a busy observation.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Before eager shutdown or quiescence shutdown, establish a positive match between the active daemon/cohort cache identity and the activation target cache. Never infer ownership from an absent identity or a busy observation.
- **Type:** functional
- **Verification Refs:** AC-001, EVAL-001

### `REQ-002` — Adapt unknown-is-foreign behavior to the fork transaction lifecycle rather than copying upstream production reserve/scope code unchanged. Account for every eager and callback shutdown route and relevant install/update/uninstall/reset caller.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Adapt unknown-is-foreign behavior to the fork transaction lifecycle rather than copying upstream production reserve/scope code unchanged. Account for every eager and callback shutdown route and relevant install/update/uninstall/reset caller.
- **Type:** functional
- **Verification Refs:** AC-002, EVAL-002

### `REQ-003` — Preserve exact same-cache install/update/uninstall behavior, exclusive mutation authority, lock ordering, bounded waits, cleanup and truthful audit outcomes. A skipped shutdown must not grant authority to mutate a foreign namespace.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Preserve exact same-cache install/update/uninstall behavior, exclusive mutation authority, lock ordering, bounded waits, cleanup and truthful audit outcomes. A skipped shutdown must not grant authority to mutate a foreign namespace.
- **Type:** functional
- **Verification Refs:** AC-003, EVAL-003

### `REQ-004` — Close time-of-check/time-of-use gaps through the existing cohort reservation/identity contracts. If the ownership identity changes between observation and reservation/callback, refuse or skip without stopping the changed cohort.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Close time-of-check/time-of-use gaps through the existing cohort reservation/identity contracts. If the ownership identity changes between observation and reservation/callback, refuse or skip without stopping the changed cohort.
- **Type:** functional
- **Verification Refs:** AC-004, EVAL-004

### `REQ-005` — Retain existing wire/API compatibility and user-visible CLI semantics except accurate diagnostics for preserved foreign/unknown sessions.

- **Priority:** must
- **Provenance Refs:** SRC-BASE
- **Statement:** Retain existing wire/API compatibility and user-visible CLI semantics except accurate diagnostics for preserved foreign/unknown sessions.
- **Type:** functional
- **Verification Refs:** AC-005, EVAL-005

## Interfaces

_None._

## Data contracts

_None._

## Decisions

_None._

## Acceptance criteria

### `AC-001` — Different-cache, busy and unreadable ownership fixtures observe zero shutdown requests and unchanged live session state.

- **Criterion:** Different-cache, busy and unreadable ownership fixtures observe zero shutdown requests and unchanged live session state.
- **Requirement Ids:** REQ-001
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites cli,daemon_runtime,daemon_ipc; full union gate before push.

### `AC-002` — Source trace enumerates all shutdown routes and lifecycle callers; each route enforces the same ownership rule.

- **Criterion:** Source trace enumerates all shutdown routes and lifecycle callers; each route enforces the same ownership rule.
- **Requirement Ids:** REQ-002
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites cli,daemon_runtime,daemon_ipc; full union gate before push.

### `AC-003` — Same-cache success, busy/refusal, failure cleanup and audit fixtures remain valid; foreign/unknown paths neither drain foreign sessions nor bypass mutation reservations.

- **Criterion:** Same-cache success, busy/refusal, failure cleanup and audit fixtures remain valid; foreign/unknown paths neither drain foreign sessions nor bypass mutation reservations.
- **Requirement Ids:** REQ-003
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites cli,daemon_runtime,daemon_ipc; full union gate before push.

### `AC-004` — Deterministically controlled ownership-change fixture proves no foreign shutdown; no timing-only sleep oracle.

- **Criterion:** Deterministically controlled ownership-change fixture proves no foreign shutdown; no timing-only sleep oracle.
- **Requirement Ids:** REQ-004
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites cli,daemon_runtime,daemon_ipc; full union gate before push.

### `AC-005` — No wire/schema/version change; regression verifies diagnostics describe the reason and that nothing foreign was stopped.

- **Criterion:** No wire/schema/version change; regression verifies diagnostics describe the reason and that nothing foreign was stopped.
- **Requirement Ids:** REQ-005
- **Verification Method:** Exact-source review plus regressions through existing Jenkins codebase-memory-cli-release-tooling suites cli,daemon_runtime,daemon_ipc; full union gate before push.

## Evaluations

### `EVAL-001` — Evaluations

- **Acceptance Ids:** AC-001
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-001
- **Suite Selection:** cli,daemon_runtime,daemon_ipc
- **Venue:** existing shared Jenkins job only

### `EVAL-002` — Evaluations

- **Acceptance Ids:** AC-002
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-002
- **Suite Selection:** cli,daemon_runtime,daemon_ipc
- **Venue:** existing shared Jenkins job only

### `EVAL-003` — Evaluations

- **Acceptance Ids:** AC-003
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-003
- **Suite Selection:** cli,daemon_runtime,daemon_ipc
- **Venue:** existing shared Jenkins job only

### `EVAL-004` — Evaluations

- **Acceptance Ids:** AC-004
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-004
- **Suite Selection:** cli,daemon_runtime,daemon_ipc
- **Venue:** existing shared Jenkins job only

### `EVAL-005` — Evaluations

- **Acceptance Ids:** AC-005
- **Implementation Worker Runs:** False
- **Kind:** acceptance
- **Requirement Ids:** REQ-005
- **Suite Selection:** cli,daemon_runtime,daemon_ipc
- **Venue:** existing shared Jenkins job only

## Implementation tasks

### `TASK-IMPLEMENT` — Activation must preserve foreign and unconfirmed daemon sessions

- **Dependencies:** —
- **Phase Id:** PHASE-IMPLEMENT
- **Requirement Ids:** REQ-001, REQ-002, REQ-003, REQ-004, REQ-005
- **Result Contract:** `{"required_evidence":["exact base and final commit","source adaptation map","changed paths","diff/format checks","Jenkins suites requested but not locally run","limitations and review risks"],"schema":"agent-workflow/task-result/v1"}`

## Implementation phases

### `PHASE-IMPLEMENT` — Discover, adapt and commit scoped implementation


### `PHASE-VERIFY` — Parent integrates, evaluates in Jenkins, independently reviews and accepts


## Risks

### `RISK-001` — Highest adaptation risk: fork reserve currently sends shutdown before reservation and differs from upstream scope/quiesce flow. Positive ownership checks must cover races, not only a preflight guard.


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
