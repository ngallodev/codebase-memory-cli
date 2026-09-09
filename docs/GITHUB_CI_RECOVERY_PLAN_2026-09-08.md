# GitHub CI Recovery Plan — 2026-09-08

Status: **BLOCKED — upstream Actions reruns require repository administrator permission**

## Execution record — 2026-09-09

- Prepared durable Agent Run `ci-recovery-20260908` against local source
  `bed05b9947bb10959cbf112b8051b80450145304`.
- Confirmed PR #2115 includes commit `92cb3b9`, which bounds the activation
  fixture's readiness/reap waits. That commit is not present in this local
  checkout, so this worktree cannot supply source-level acceptance for it.
- Confirmed the current `apt.llvm.org` Noble LLVM 22 index contains `clang-22`,
  `libclang-rt-22-dev`, and `llvm-22`; the MSan package outage is no longer
  reproducible from the configured source.
- A focused local `scripts/test.sh --suites cli` run on `bed05b99` remained
  incomplete after 204 seconds and was stopped. It made no worktree changes.
- GitHub rejected reruns of failed workflow runs `34288397854` and
  `34282381014`: `Must have admin rights to Repository`.
- Direct GitHub CLI verification confirms `ngallodev` has `pull` only (no
  push, maintain, or admin) on `DeusData/codebase-memory-mcp`; both affected
  PR branches are owned by `DeusData`.

Required upstream action: an administrator must rerun the failed jobs (or push
a new commit to each PR) and inspect the fresh `MSan`, Windows guards, ARM
Unix shard, and `ci-ok` results. Until those runs pass on their current merge
SHAs, this plan is not complete.

## Scope and evidence

This plan resolves the failures seen in the required `PR` workflows, then
verifies fresh GitHub Actions results. It does not weaken or bypass a test.

| Failure | Evidence | Classification |
| --- | --- | --- |
| Windows concurrent cold start | Runs [34288397854](https://github.com/DeusData/codebase-memory-mcp/actions/runs/34288397854) and [34282381014](https://github.com/DeusData/codebase-memory-mcp/actions/runs/34282381014) both fail `test-windows-guards`: a cold-storm client reports `secure CLI coordination could not be created (endpoint)`. | Shared product/coordination defect; the same failure crosses unrelated PRs. |
| ARM CLI suite | Run [34288397854](https://github.com/DeusData/codebase-memory-mcp/actions/runs/34288397854) exhausts the 900-second `cli` suite limit after `ASSERT(host_serving)` in `cli_install_into_host_namespace_still_drains_host_cohort`. | Product or test-contract regression specific to PR #2115's activation-guard change. |
| MSan image build | Run [34282381014](https://github.com/DeusData/codebase-memory-mcp/actions/runs/34282381014) cannot install `clang-22`, `libclang-rt-22-dev`, or `llvm-22` from its configured LLVM apt source. | CI image/toolchain provisioning defect, before product tests run. |

`ci-ok` is an aggregate failure and requires no separate repair.

## Order of work

1. Record a fresh failure matrix before editing: workflow run, merge SHA, failed
   job/step, runner image, and exact first error. Keep this evidence with the
   repair PR. Do not use historical logs as pass evidence.
2. Repair MSan provisioning first because it is isolated from product behavior.
3. Repair the Windows cold-start race on a branch based on current `main`.
4. Rebase the activation-guard PR onto that baseline, then resolve its ARM CLI
   hang without changing the suite timeout.
5. Rerun required checks on the resulting merge SHAs and inspect each job,
   rather than treating a successful workflow dispatch as success.

## 1. MSan provisioning

1. In the exact failing Docker build context, capture `apt-get update` output
   and `apt-cache policy` for all three LLVM 22 packages. Confirm the
   repository suite, key, architecture, and package names actually published
   by apt.llvm.org for the runner's Ubuntu release.
2. Replace the unavailable source with one reproducible LLVM 22 provisioning
   route already compatible with the MSan image. Prefer the repository's
   existing source-built runtime path if it supplies the required compiler and
   runtimes; otherwise use the correct published LLVM repository. Pin the
   selected compiler/runtime version together.
3. Run `scripts/ci/msan-lane.sh build`, then `scripts/ci/msan-lane.sh run`.
   Retain the build output proving the compiler and MSan runtimes are present.

Acceptance: the MSan image builds without retries masking a missing package,
and the MSan test job reaches and passes its intended suite.

## 2. Windows concurrent cold start

1. Reproduce the existing six-client scenario from
   `tests/windows/test_daemon_stability.py` on the Windows runner or the
   authoritative Windows host. Repeat it enough to distinguish deterministic
   failure from a timing-only flake.
2. Instrument only the shared endpoint/coordination creation failure path:
   capture the platform error code, operation, and safe runtime-path category.
   Do not log endpoint secrets, user paths, or broaden the test's deadlines.
3. Trace endpoint construction, startup-lock acquisition, generation probing,
   spawn handoff, and cleanup as one state transition. Fix the shared state
   handoff that permits a racing client to observe an unusable endpoint; do not
   add per-client retries that merely conceal a failed handoff.
4. Add or extend the smallest deterministic regression assertion at the shared
   boundary if the current cold-storm test cannot distinguish the broken state.

Acceptance: repeated cold storms succeed, all clients either share the valid
ephemeral daemon or receive the defined recoverable state, and the daemon
retires as the existing guard requires. Run the focused Windows guard followed
by the complete Windows test matrix.

## 3. ARM activation-guard hang

1. Run the `cli` suite alone with its normal 900-second ceiling, preserving its
   captured output. On the stall, collect the host/target daemon status, cohort
   claims, startup-lock state, and child-process tree before cleanup.
2. Compare the install target's `HOME`/runtime namespace with the host namespace
   used by `cli_install_into_host_namespace_still_drains_host_cohort`. Establish
   whether the test's expectation is still valid or the activation guard now
   fails to drain a legitimate host cohort.
3. Repair the lifecycle ownership at the common activation/cohort boundary. A
   sandbox install must not drain an unrelated host daemon, while an explicit
   host-target install must complete its expected drain and restore service.
4. Add a bounded regression test for both namespaces. Do not solve the failure
   by skipping the test, extending its timeout, or serializing unrelated suites.

Acceptance: the focused `cli` suite completes under its current limit on ARM
and x86 Linux; the exact host and sandbox cases pass; the full Unix shard matrix
has no timed-out suite.

## Integration and evidence gate

1. Keep MSan, shared Windows coordination, and activation-guard changes in
   reviewable commits. Rebase dependent PRs after the shared repair merges.
2. Before push, run the smallest relevant local checks:

   - `scripts/ci/msan-lane.sh all` for the image repair;
   - the focused Windows daemon stability guard on Windows;
   - `scripts/test.sh --suites cli` for the activation path.

3. For each changed branch, inspect the new `PR` workflow's individual jobs:
   MSan, Windows guards/matrix, Unix ARM shards, diagnostics, lint, security,
   and final `ci-ok`.
4. Declare the work complete only when every required job is `success` on the
   post-repair merge SHA. Attach the URLs, SHAs, command output, and any
   remaining environmental limitation to the PR or a durable handoff.

## Explicit non-solutions

- Do not rerun until green without identifying the failing transition.
- Do not suppress Windows cold-storm coverage.
- Do not raise the ARM suite timeout or mark the CLI suite optional.
- Do not make `ci-ok` ignore a failed `test` job.
