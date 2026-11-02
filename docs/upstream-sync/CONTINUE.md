# Continuation prompt

Continue the 2026-11-01 CLI-first upstream campaign. Read PLAN.md, ledger.json,
13a.md and the canonical ../UPSTREAM_308_CANDIDATE_AUDIT_20260912.md first.
Preserve all unrelated main-checkout artifacts. Do not merge PRs, enable
GitHub auto-merge, push main, or dispatch Actions.

Refresh origin refs and all draft PRs, Git status/worktrees and Jenkins state.
Security draft #27 is sync/security-20261101 -> main; guard draft #28 is
sync/ci-guard-20261101 -> security; the 13A draft #29 is
sync/upstream-13a-20261101 -> guard. Verify current PR numbers/SHAs live.
Upstream true merge a03f1f8ab85e8861d8d6820061ccfd88797efae1 has exact parents
ff8b8906c6807953336609cc7f464e7e4ef3cf7b and
5dc83ac9fa363d51706a70977840b61ea59ce9db. Never rewrite these parents.

The existing codebase-memory-cli-release-tooling build #85 finished SUCCESS
at exact d1a4c9d244e6a0446e1ab45fffc5da2d4cce3e1c with empty CBM_TEST_SUITES.
Archived qualification and binary source revisions match; checksum is verified.
C shards: 9348 passed, 0 failed, 5 platform skips; TSAN: 919 passed, 3 platform
skips. Post-test and wrapper gates passed. Preserve failed special-job builds
and #84 as separate evidence. See 13a.md and ledger.json for the final receipt.
Receipt-only commits after d1a4c9d2 are not independently Jenkins qualified.
Local diff-scoped clang-tidy cause/baseline remains unverified; native Windows
commands and frozen corpus pins are in windows-qualification.md. Keep draft
status until remaining platform/review/acceptance gates are resolved.

After review and required gates, the next true merge boundary is 13B:
4c80d89edf1283cfeaa7e60ecbf40c9d2eb2b689, 119 commits after the 13A upstream
checkpoint. Start from the reviewed integration result, never stale main.
Use isolated indexed worktrees and bounded Luna workers where helpful. Read
Agent-Workflow/index-preflight and worker footer instructions before launch.
Bind a compatible, truthful acceptance contract before durable preparation;
keep worker exit/completion/evaluation/review/acceptance separate. Initial
cbm-sync-{security,ci,13a}-20261101 runs are not accepted; do not rewrite their
sealed evidence. Parent CLI graphs worked but Luna runtimes failed cache-resolve;
use exact source fallback for recorded gaps, without changing allow-roots.

Keep finite npm/PyPI/Go resource limits: compressed 256 MiB, 64 members,
member 512 MiB, expanded 512 MiB. Preserve CLI product identity, native operations,
Jenkins-only automatic CI and valid manual/reusable workflow YAML. Run the strict
policy checker before every push. Pinned campaign ends at
6c0ce230ee26ab8d263eb5235ebf73755558344e; the 9 newer observed commits through
edc2301b022b1f5e1e0ec11e291917e22e9736aa belong to a later campaign. Reconcile
ledger entries against exact source and ancestry instead of treating history
membership as semantic completion. 13B–13K remain planned, not implemented.
