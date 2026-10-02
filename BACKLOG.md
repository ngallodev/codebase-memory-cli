# Backlog

## Resolved

- [x] Installer fork defaults and complete AST/watcher/macro/COUNT integration
  landed via PR #12; Windows hook/macOS deadline repairs landed via PR #13
  (`bbabc073`). Historical Jenkins evidence remains revision-specific.

- [x] Release-tooling implements deferred npm/PyPI/public publication,
  removes Claude automation, enables CodeRabbit, adds Linux-only prerelease
  selection and repoints package fork endpoints. PR #14 merge and real artifact
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

## Open

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
