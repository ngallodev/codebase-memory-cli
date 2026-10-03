# Backlog

## Resolved

<<<<<<< Updated upstream
- [x] Installer fork defaults and complete AST/watcher/macro/COUNT integration
  landed via PR #12; Windows hook/macOS deadline repairs landed via PR #13
  (`bbabc073`). Historical Jenkins evidence remains revision-specific.

- [x] Release-tooling implements deferred npm/PyPI/public publication,
  removes Claude automation, enables CodeRabbit, adds Linux-only prerelease
  selection. Package migration is deferred until exact fork assets and checksums
  exist; the previously merged package revisions are retained. PR #14 merge and real artifact
=======
- [x] Release-tooling implements deferred npm/PyPI/public publication,
  removes Claude automation, enables CodeRabbit, adds Linux-only prerelease
  selection and repoints package fork endpoints. PR #14 merge and real artifact
>>>>>>> Stashed changes
  qualification remain open; these are implementation-complete tasks only.

- [x] Reconciled completed upstream verticals: COUNT/status, persisted LSP
  decode (PR #4), Cypher semantics (PR #5), XML admission (PR #6), TypeScript
  await (PR #7), properties scanner cleanup, macro coverage, watcher backoff
  and all AST walker stacks are represented on main. See the canonical ledger
  for exact commits and remaining scheduler/allocation-failure evidence gaps.
- [x] Removed stale upstream candidates: `77a0c7d9` empty-graph sort and
  `04ba2fa3`/`98899ba5` Cypher capacity checks already exist in current source.
- [x] One-shot index metrics are implemented (`0f5a84bc`). Neutral operation/
  daemon test source lists supersede the historical MCP link-time blocker.

- [x] Resolve application-teardown ownership omission for neutral session
  state. `cbm_daemon_application_free()` now calls
  `cbm_operation_session_state_free()` before freeing each session. Valgrind
  identified the omission through `test_daemon_application_free_releases_live_watch_once`
  as 80 direct + 107 indirect lost bytes.
- [x] Validated local `release-tooling` through Jenkins job #34 (full gate
  passed at `37dd63ba`). PR #12 merged with installer fix `c9841610`
  (install.sh and install.ps1 default to `ngallodev/codebase-memory-cli`).
  Resolved thread `PRRT_kwDORjrg5c6n5KE9` 2026-10-01.
- [x] Validated complete AST slice `77ab4bea` / `7707c9d7` in shared
  release-tooling Jenkins job #34 (full gate passed at `37dd63ba`); all
  definition and auxiliary stacks ported to main.
- [x] Added opt-in, single-run index metrics for performance diagnosis via
  `index --metrics-out` flag (`0f5a84bc`). Coverage in src/operations/index.c
  and tests/test_index_resilience.c. Merged via PR #12.
- [x] Fixed the PR #12 Windows/macOS CI failures in `bbabc073` (PR #13).
  The Windows hook-lifecycle test now uses the fork's installed binary name
  (`tests/test_cli.c`), and the macOS connection-cap test gets a 30 s server
  hello deadline (`tests/test_daemon_runtime.c`). Jenkins #35 and full GitHub
  CI passed. The pre-commit hook also clang-formatted both files.

## Open

- [ ] Reconcile interactive upstream assessment and independent completeness
  audit (`CBM-UPSTREAM-ASSESS-20261002`, `CBM-UPSTREAM-VERIFY-20261002`).
  Both workers running in pre-indexed isolated trees; frozen MCP `96c3f41c`,
  CLI `9cb5cc21`, 19 new / 367 ancestry-differing nonmerge commits. Canonical
  applicability/vertical classification and verification remain pending.

- [x] Closed daemon/cache interactive workers after verifying committed clean
  candidates `028a4bfb` / `fa5a180b`; sealed partial run receipts, preserved
  branches/bundles/evidence and removed only their two worktrees.
- [ ] Review/integrate those candidates and run parent-scheduled Jenkins;
  daemon AC-005 additionally needs a direct CLI diagnostic assertion.

- [x] Specified all three selected verticals and prepared external interactive
  GPT-6-Luna Agent Runs at frozen base `51aa7a86`, each in an independent
  worktree with full permissions/approval never. See the latest 308 dispatch record.
- [x] Launched all three interactive GPT-6-Luna workers in Herdr's right-hand
  column (`wC:p5`, `wC:p6`, `wC:p7`), bound to durable external Agent Runs.
  Full permissions/approval never verified; all three acknowledged steering
  and are observed working. Implementation/Jenkins/review remain pending.

- [ ] Next upstream slice 1: adapt `ed76cf8e` daemon cache-ownership safety
  across eager shutdown and quiescence callback; preserve foreign/unknown
  sessions and existing exclusive mutation authority. Jenkins only.
- [ ] Next upstream slice 2: adapt `8093d10b` + `ea6fa49f` as a complete
  binary `.res` / ReScript admission, traversal, scanner and checksum slice.
- [ ] Next upstream slice 3: adapt `46015320` to preserve `_config.db` and
  `_cross_repo.db` across project-index list/count/reset; share exact-name
  classification with cross-repo enumeration.
- [ ] Complete full 348-commit semantic disposition and fresh exact-worktree
  graph indexes for both repos; this top-three selection is a bounded shortlist.
- [ ] Finish `manage-adr` alias/help and explicit outline parity: existing ADR
  candidate failed Jenkins lint before tests and is not integrated.
- [ ] Retain validation gaps: live watcher scheduler integration and AST
  allocation-failure injection are not established by completed helper tests.
- [ ] PR #14 at `1ce4e164` has failing GitHub diagnostic, macOS LSan,
  Unix/Windows test and shard-completeness checks (2026-10-02T04:14Z snapshot).
  Diagnose and pass required checks before merge/release; no release is qualified.

- [ ] Qualify native Windows/WSL and real fork package installations. Fork
  endpoints and a Linux-only prerelease route are implemented on release-tooling
  (`1ce4e164`), but implementation is not artifact qualification or publication.

- [ ] Selectively evaluate remaining upstream fixes from
  `../codebase-memory-mcp` before merging into the CLI fork. Do not
  cherry-pick MCP-only changes or overwrite CP89/release-tooling work; apply
  compatible patches into a temporary worktree, run focused suites, then
  review the resulting diff before integration.
  - `9846c3f1` and `c1b9c451` Windows daemon/session-root separator
    handling; port daemon-only hunks and omit MCP hunks.
  - `92725a5e` Windows stdio parent-death cleanup; verify relevance to the
    CLI-only process model before porting.
  - `e5c17de6` and `b04f5450` coverage correctness/performance; manually
    reconcile against the fork's changed coverage implementation.
  - MCP-only output/search/pagination/compact-output commits are intentionally
    excluded from this backlog item.

<<<<<<< Updated upstream
=======
- [ ] Assess a lightweight path for store-only test execution. Run just the
  store test binaries after shared prerequisites are built, while keeping the
  existing incremental suite selection and full-suite gates intact. First
  inspect the runner's configuration and targets to determine whether this is
  already supported. The `cbm-upstream-count-20260929` focused run passed 96
  store tests, but a cold worktree spent several minutes compiling shared
  prerequisites including `lsp_all.c`.

>>>>>>> Stashed changes
- [ ] Classify residual Heaptrack allocations in the neutral daemon test path.
  - Baseline evidence: Heaptrack reported 91 leaked allocations across the
    daemon application and IPC suites. Valgrind reported 538 bytes from
    `cbm_daemon_ipc_endpoint_new()` in the forked crash-simulation child of
    `test_daemon_ipc_posix_current_generation_crash_cleanup_requires_startup_lock`
    (`src/daemon/ipc.c:889`, test at `tests/test_daemon_ipc.c:3735`).
  - Remediation applied: the child now frees its inherited endpoint object
    before `_exit()`; it still bypasses `listener_close`, so kernel-released
    descriptors/locks and crash artifacts remain part of the test.
  - Current evidence: fresh `scripts/analyze-memory.sh` reports no static
    findings and Valgrind reports 0 definite, 0 indirect, and 0 possible lost
    bytes. Heaptrack reports 87 leaked allocations, which require attribution
    as still-reachable allocator/process-lifetime state before any further
    cleanup is attempted.
  - Next path: inspect the saved Heaptrack allocation call trees; only add
    teardown code for allocations proven to be owned by the neutral daemon
    rather than allocator or test-process lifetime state.

- [ ] Finish exact-revision Jenkins evaluation and independent review of the
  2026-10-02 daemon/cache/ReScript integration candidate. Parent corrected the
  premature maintenance signal and ReScript child assertion; runtime claims,
  remote push and main integration remain pending. See the canonical 308 ledger.
- [ ] Reconcile the GPT-6.1-Sol full upstream merge/constituent audit
  `CBM-UPSTREAM-SOL-20261002-R2`, including ungrouped verticals and the next
  five recommended slices. Do not count running audit work as coverage.
