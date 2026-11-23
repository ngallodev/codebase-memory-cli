# Continuation prompt

> Current 2026-11-23 continuation: main `420959524` (PRs #33, #34, #35 merged) contained upstream through `612eee2518ef84cc573e491fdee5fa5c90264cb5` and was 355 commits behind upstream main `9a4da7a314833d8d965b74612e1f25fef5371861`. Stacked local branches 14A (`sync/upstream-14a-20261123`, head `984d34dd2`, upstream `b62a2ffaf`, index format 4 -> 5) and 14B (`sync/upstream-14b-20261123`, head `76ec0f51b`, index format 5 -> 7) bring it to zero commits behind. See [14a.md](14a.md), [14b.md](14b.md), and [ledger.json](ledger.json). Jenkins release-tooling: 14A #101 on `984d34dd2` SUCCESS (#99 aborted by mistake, #100 failed at the language-count contract and was fixed); 14B pending. Native Windows, macOS, FreeBSD, and the TSan image probe were not run. Local test runs need umask 022. Independent review and host acceptance remain separate gates. No push, PR publication, or GitHub Actions dispatch occurred.

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

The pinned campaign is integrated through 13L. Preserve the true merge ancestry and
per-batch receipts. This source run is provisional; independent review and host
acceptance remain separate gates. Codebase-memory graph cache-resolve failures are
recorded; use exact-source evidence for those documented gaps without changing
cache roots or allow-roots.

Keep finite npm/PyPI/Go resource limits: compressed 256 MiB, 64 members,
member 512 MiB, expanded 512 MiB. Preserve CLI product identity, native operations,
Jenkins-only automatic CI and valid manual/reusable workflow YAML. Run the strict
policy checker before every push. 13K ends at
6c0ce230ee26ab8d263eb5235ebf73755558344e; the active pinned final target is
a5b248f47272413189746589b9a2c551c6b6c5ec, with 19 commits in 13L. Do not chase
later upstream commits. Reconcile ledger entries against exact source and ancestry
instead of treating history membership as semantic completion. The older 13B–13K
pending status above is superseded by the current ledger.
