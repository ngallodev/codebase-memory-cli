# Upstream synchronization campaign — 2026-11-01

## Live continuation — 2026-11-02

13A is on the authoritative fork main at `27b731cec45e2353303738ac6d8e77acbef42b7c`.
The pinned continuation target is `a5b248f47272413189746589b9a2c551c6b6c5ec`.
13B–13L are integrated provisionally in order. The final 13L merge is
`8a686f225b2c64f81c0716f3129c36c50fb10ffe` with receipt [13l.md](13l.md); the
pinned target is now an ancestor and no pinned upstream commits remain. Independent
review, final Jenkins qualification, and host acceptance remain open. Current state is in
[ledger.json](ledger.json); older campaign inventory and PR bases below are
historical, and do not override the continuation run contract.

This campaign supplements the [canonical integration ledger](../UPSTREAM_308_CANDIDATE_AUDIT_20260912.md); it does not supersede older unresolved semantic dispositions. Machine status and per-checkpoint acceptance live in [ledger.json](ledger.json). [commits.tsv](commits.tsv) enumerates all 933 reachable campaign commits once, including 578 merges and 355 non-merges. Subsystem tags derive from changed paths; they are triage, not proof of implementation.

## Verified baseline

- Fork `origin/main`: `ff8b8906c6807953336609cc7f464e7e4ef3cf7b`.
- Common ancestor: `8df0639c6268afe9888979e72b9797aa2cc1de7c`.
- Observed upstream: `edc2301b022b1f5e1e0ec11e291917e22e9736aa`.
- Pinned campaign: `6c0ce230ee26ab8d263eb5235ebf73755558344e`; nine subsequent commits deferred.
- `git rev-list --left-right --count origin/main...upstream/main`: **348 fork-only / 942 upstream-only** after fetch.
- All consecutive handoff checkpoints and the final nine-commit extension pass `git merge-base --is-ancestor`; historical batch counts are correct.
- One open PR: #25, public documentation cleanup; no sync PR at inventory time.
- Classic branch-protection endpoint returns 404. Active `protect` ruleset prevents deletion/non-fast-forward of default branch; it requires no status checks. No gate was bypassed or weakened.
- Main checkout contains pre-existing untracked local artifacts and the handoff. Implementation uses separate clean worktrees; unrelated artifacts remain intact.

## Sequence and acceptance

1. Finish narrow security/dependency fixes and verify actual advisories, versions and fork applicability. Preserve installer download/extraction limits and explicit workspace authorization.
2. Harden the local/Jenkins Actions policy guard and add ancestry verification with positive/negative checks.
3. Complete true merge of upstream `5dc83ac9fa363d51706a70977840b61ea59ce9db` (13A, 79 commits); reconcile reviewed hotfix/guard commits, retaining upstream merge ancestry. Publish only sanitized draft branches.
4. Batches 13E–13L build sequentially on the latest provisional checkpoint. They remain dependent and unaccepted until independent review. Publish only sanitized stacked draft PRs with explicit dependencies; never base them on stale main.
5. Run focused checks after each completed batch, then full exact-SHA Jenkins qualification before recommending merge. Native Windows, >4 GiB SQLite offsets, graph determinism and sanitizer/recovery checks remain explicit gates when affected. Pending gates are not successful gates.

Every batch must preserve CLI JSON/compact contracts, native HTTP, daemon ownership, durable storage and source provenance. No MCP server, transport/session registration or mandatory MCP dependencies. Retain useful shared changes via native CLI operations. Record every material exclusion/adaptation and schema migration in its batch receipt.

## CI venue and operator gap

PR #26's Jenkins-only policy remains mandatory. `Jenkinsfile` executes the Actions policy guard; every pushed branch must pass it before publication. Do not dispatch GitHub workflows.

Read-only live Jenkins inspection found `codebase-memory-cli-main` (local `refs/heads/main`) and `codebase-memory-cli-release-tooling` (local `refs/heads/release-tooling`), both with no configured triggers. The release-tooling job's latest pre-campaign build was #83 SUCCESS; it does not validate campaign code. No CLI multibranch PR-discovery job was present in the inspected job list.

To verify PRs automatically, an operator should configure a Jenkins GitHub Branch Source multibranch job for the fork, enable draft/regular PR discovery as intended, register GitHub webhook delivery to Jenkins, expose branch/PR-specific status contexts, and configure the default-branch ruleset to require those Jenkins contexts after a successful trial. Do not substitute Actions checks or weaken protection. A dedicated exact-commit job can qualify this campaign without changing existing jobs. Record actual build URLs and source revisions in receipts.

## Platform and benchmark follow-up

Use the existing pinned corpus at `benchmark-runs/linux/20260911T045124Z/BENCHMARK_CORPUS.frozen.json` for Linux/Windows parity: codebase-memory-cli `0c6d00d6011c1fdb50b90c53538f536cc3b5e4d9`, specgen-aw `466debfec788c7aeeee858c6b51e98b1b9afe653`, agent-workflow `e84925205301ff7169aff5a8cb14d10680c3739c`, agent-workflow-spec-contracts `0b32e556ef33bd00c0111d30a8b9225eb5196fcd`, and herdr `61ca85d5895bb530b59da85beeccdd767f4720d2`. Preserve these manifest pins; candidate version and platform evidence are separate. No current benchmark or Windows result is implied by this inventory.

## Historical delivery checkpoint (2026-11-01)

At that historical checkpoint, security PR #27 targeted main, CI-policy PR #28 targeted the security branch, and the 13A integration branch targeted the CI-policy branch while preserving upstream merge `a03f1f8ab85e8861d8d6820061ccfd88797efae1`. Its source and regression receipts are in [13a.md](13a.md). The inventory recorded 854 pinned upstream commits after 13A. The user later authorized and completed promotion of 13A; current authoritative main is `27b731cec45e2353303738ac6d8e77acbef42b7c`. The 2026-11-02 continuation receipts supersede the old pending status; the current next checkpoint after provisional 13G is `a3831c979547d33adaa36998dca2b004d735fc84`.

A dedicated Jenkins job `codebase-memory-cli-sync-13a-20261101` is prepared for
an exact integration SHA, with no automatic triggers. Existing main/release jobs
were not reconfigured. Existing release-tooling build #85 subsequently passed
the full Linux gate at d1a4c9d2; see 13a.md for exact archived evidence. Do not
recommend merge while the local clang-tidy assessment, native platform checks
or review/acceptance remain unresolved.

Initial durable workers lacked bound native acceptance commands. Their source,
tests and sealed evidence are retained, but `agent finish` returned
`verification_required`; no lifecycle acceptance is claimed. Evaluation-plan
recovery was investigated, but the installed evaluation schema supports only a
Docker venue while these checks ran natively. Do not relabel native execution
or mutate sealed records to manufacture acceptance.
