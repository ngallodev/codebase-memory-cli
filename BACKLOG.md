# Backlog

## Resolved

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

- [ ] Complete semantic reconciliation of all frozen upstream merges and
  constituent commits. Sol R2 inventories all 923 merges / 2,429 non-merges;
  311 merges / 367 non-merges differ by ancestry. It reconciles the older 87
  canonical rows, but 356 non-merge whole-commit remainders and all 311 merge
  dispositions remain unresolved. Census, patch ID and titles are not coverage.
  See [the canonical source audit](docs/UPSTREAM_308_CANDIDATE_AUDIT_20260912.md).
- [ ] Finish independent reconciliation of the Luna team evidence. All R3
  handoffs were partial; all 40 declared checksums and exact 111/170/86 assignments
  were verified and archived. R4 successors continue interactively in Herdr
  `wC:pB` / `wC:pC` / `wC:pD`. The 2026-10-02T23:53Z checkpoint contains 54
  non-unresolved worker claims and 313 unresolved claims; no whole-commit coverage
  or completion is inferred. Parent retains 356 whole non-merge / 311 merge
  remainders pending exact source/test review. Full hashes are in the canonical
  ledger; the [consolidated report](docs/UPSTREAM_RECONCILIATION_REPORT_20261002.md)
  records provenance, source corrections, scope and validation limits.
  Parallel split followup: `CBM-RECON-ADAPTER-LOW-20261003` ran Luna low in
  Herdr `wC:pE` on 16 explicitly released, previously unreviewed runtime hashes;
  all 16 were source-reviewed, 8 closing as adapter-only and 8 retaining
  shared-semantic remainders. The consolidation auditor verified the run's
  67-file manifest, confirmed every changed path is `src/mcp/**` or
  `tests/test_mcp.c` (that subtree exists upstream and is absent from
  production and both QA trees), and confirmed the 16 are upstream ancestors
  absent from those three CLI trees.
  After the Phase1 transfer, original runtime retains 39 hashes; all 367 hashes still have one
  effective primary owner. The transfer, acknowledgement, index and launch
  receipts are in the new run's evidence. Completed work was not reassigned.
  Phase1 successor plan now transfers 115 still-unreviewed runtime hashes to
  three prepared DeepSeek Flash assignments (39/26/50 disjoint groups), preserving the
  exact releases and completed work. Unstarted low2 was retired; corrected low1
  finished partial with detailed handoff and its pane closed. See the canonical
  Phase1 plan. Native assignment delivery failed: all three returned predecessor
  reports, including after persisted steering and direct follow-ups. Sessions
  were stopped; zero assigned comparisons or correlated worker acknowledgements
  were established by that failed attempt. Interactive recovery now runs Codex
  `--model deepseek-flash` in Herdr `wC:pF` / `wC:pG` / `wC:pH`, with correct
  worker-issued run/cwd/count launch acknowledgements and actual process Jev-token
  inheritance verified. The same 115 hashes remain the comparison scope; see the
  canonical interactive recovery section. No completion/acceptance is inferred.
- [ ] Review additional bounded missing behaviors: malformed confidence parsing
  `62b44519c68b403ea44d9a85ba8ee74cddee66e6` (first additional localized candidate),
  route handler position `592894a4387a50e1d128ff6a2695ff48f8cc32a5`, late annotation
  arguments `c36b4fbc446f9085704a6073b81ec743fd4180fc`, and client-registry selection
  `1e25d962d69999438229b0562d44ea60e97f5456`. Group route followups with slice 5;
  review full tests/dependencies before broadening implementation scope.
- [ ] Resolve daemon memory policy before selecting active-job division
  `21591d51168d26f931f64d62edc36013cd1f6249`. Fork uses fixed worker-capacity
  division; upstream active divisor is not represented. Review aggregate
  reservation invariants, separate Windows job-memory capability hunks from
  MCP adapter policy in merge `2b8bd0c065c1d2728994fddc1683a66f70745748`.
- [x] Daemon/cache/ReScript candidates and ADR alias/outline are integrated
  in local QA `559214af28ce2d6f5d6ec86662f6871316d28b45`, per acknowledged
  parent steering and source comparison. Candidate implementation is separate
  from runtime validation, acceptance and production-main integration.
- [ ] Finish parent review and shared Jenkins validation for that QA revision
  and its later repairs. No current Jenkins success or acceptance is established
  by Sol R2. The pre-commit candidate retains formatting-only behavior; candidate
  ancestry does not establish production Git-child environment isolation.
- [ ] NEW upstream slice 1: Cypher OPTIONAL relationship fallback growth,
  `2fabaa2b877986bdededb4f4f78e9a31014c6fdf`; existing append helper avoids
  the per-binding overflow. Jenkins `cypher`.
- [ ] NEW upstream slice 2: SQLite zero-cell trailing interior-page packing,
  `e7324bb6904c6d0a295b956e83c8585219198358`.
  Jenkins `sqlite_writer,graph_buffer,pipeline`.
- [ ] NEW upstream slice 3: Swift scanner shift widths, regression and integrity,
  `3f831936b5f3012d13175e7c97326165efecefad` ->
  `a2ea711c29560511eb8467bea7ad309ebf500d6e` ->
  `e8204092333ee1c38cd681a81abb36f125288894`.
  Jenkins `language,extraction,grammar_regression` and vendored integrity;
  preserve QA's ReScript manifest/checksum row.
- [ ] NEW upstream slice 4: ancestor ignore chain with correct anchoring,
  `c8184ed94887a6b5e130726885b44a07c63a47b7` ->
  `282fbfcdabfec79c19c36f5bec5f75c22eb36df5`.
  Jenkins `discover,gitignore,git_context,pipeline`.
- [ ] NEW upstream slice 5: deterministic route selection and >64 proto services,
  `6b48097471e8d1a3e5cf16447304899a21e12d7a`; adapt route hunks without
  importing upstream memory core or unrelated tests. Jenkins `pipeline,parallel`.
  Ranking is bounded by inspected source; unresolved work can change priorities.
- [ ] Evaluate GitHub ancestry reconciliation after semantic review. Adapted
  commits have different IDs; documentation cannot alter GitHub's behind count.
  An upstream ancestry merge requires explicit review of missing/excluded changes
  and merge-resolution behavior. Do not use an `ours` merge to conceal unresolved
  work, rewrite published history, or declare the fork globally caught up.
- [ ] Clarify CLI coverage array syntax and missing-path results. At dev CLI /
  project `tmp-cbm-upstream-sol-20261002`, generation `2026-10-02T17:30:32Z`,
  JSON/comma `--paths` values became literal nonexistent paths with
  `no_recorded_issue` / `freshness=missing`. Repeated `--paths` / `--scopes`
  flags work. Document this spelling and make missing paths prominent.
- [ ] Align trace truncation with documented pagination. `extract_func_def`
  both-direction trace at limit 100 returned 127 total callees and truncation
  without a next cursor; limit 300 returned all 127 callees / four callers.
  Emit a cursor or document the limit restriction. Exact receipts are in Sol R2
  evidence; these are observed CLI issues, with no source fix in this audit.
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
  - Exclude direct MCP-only adapter imports at the source seam; shared neutral
    capability parity still requires source verification. Historical exclusions
    are superseded by the canonical audit where mixed hunks were found.

- [ ] Assess a lightweight path for store-only test execution. Run just the
  store test binaries after shared prerequisites are built, while keeping the
  existing incremental suite selection and full-suite gates intact. First
  inspect the runner's configuration and targets to determine whether this is
  already supported. The `cbm-upstream-count-20260929` focused run passed 96
  store tests, but a cold worktree spent several minutes compiling shared
  prerequisites including `lsp_all.c`.

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

- [ ] Complete DeepSeek Phase1 independent source review. All115 assignments and
 342 evidence-manifest entries pass integrity checks; six bounded semantic
 questions were reviewed in three batched Jev requests. Documentary result is
 changes requested: narrow runtime `9724d903046626df0640ced218259cb58070e4e8`
 committed-counter/budget claim, preserve operations
 `b04f54506ab42eddcbd89232a0d47d8355edd596` mechanism remainder and
 `f6afd6138e605365a76d065d04521ec2037bed3d` title-policy question, and classify
 integration `00411ce7bc41995783bfb16aada673515585446d` changed Linux alias
 behavior as missing. Other109 hashes remain independently unreviewed; integration
29 partial rows plus1 unresolved row require function-level work. Exact hashes,
requests, full distributions and source corrections are in the canonical308 review
section and consolidated report; machine crosswalk stays handoff evidence. No
completion/acceptance or semantic-closure counter advanced.

- [x] Gather Luna low pilot and successor source evidence; independent review is
 recorded in canonical308. Remaining16 full-hash whole-commit reviews are tracked
 below; former launch-only20-unverified snapshot is superseded.

- [ ] Complete Luna medium runtime20 whole-commit source review: documentary
 changes requested; manifest31 passes, two bounded source dispositions verified,
18 exact full-hash remainders in parent evidence/medium20-independent-review/review.json.
 Group262ab012 telemetry with3507d955 Windows OS hard cap; no runtime/acceptance credit.
 Canonical308 finished-batch review owns corrections.
- [ ] Diagnose generic full-index pipeline failure for clean frozen worktree
 `/tmp/cbm-recon-luna-medium20-20261003`; logs preserved in medium20 staging.
 Worker safely uses verified existing runtime graph; no daemon changes made.

- [x] Review resumed DeepSeek pilot20 submission; four bounded source dispositions
 verified and16 retained unresolved (see following task). Worker completion is not acceptance.

- [ ] Resolve prior DeepSeek pilot20 independent review changes:16 exact full-hash
 whole-commit remainders in parent evidence/pilot20-independent-review/review.json.
 Correct false absent-alias/YAML/fd-guard/Codex claims, map missing CLI regressions,
 and preserve stale worker checksum failure; four source reviews credited, no
 runtime/acceptance claims. Canonical308 verification section owns corrections.
- [ ] Complete remaining17 DeepSeek ops20 whole-commit source reviews, exact hashes
 in parent evidence/ops20-followup-v4-review/review.json. Three source dispositions
 verified, including missing Scala route composition d4931241. Preserve56516eff
 recursive-grep fallback correction,26650255/f339aa62 dependencies, separate
 e5c17de6 unchanged cap context and canonical SQL/stale refusal policy obligations.
- [x] Review followup-v2/v3 evidence and record their semantic limits; v3 hunk
 inventory is discovery only. Followup-v4 corrects wrong-function/test exclusion
 for d4931241; other17 full-hash semantic mappings remain pending above.
- [x] Independently verify d4931241adaef732066afb5ebf1cb225e2474403 source comparison:
 all3 hunks missing across production and both frozen QA refs. Applicable shared
 Scala regression/harness verified; prerequisite041eb99c11692b93d43ea72c884cc50c51285db5
 present. Future implementation/Jenkins/acceptance remain unperformed.

- [x] Confirm ten historical merged/adapted tasks against current release-tooling
327dccc0 source and regression contracts: Go selector binding; daemon UI/Unicode;
Cypher scope/star; registry receiver chains; cohort retry; Windows YAML guards;
cache-root cohort handoff; complexity determinism; Razor; embedded sibling hosts.
10/10 source confirmed, selected remainder0. Canonical308 owns evidence/limits;
no subtraction from678 ancestry or separate35 ops/runtime pending reviews.
Native Windows qualification and fresh Jenkins51 result remain separate.
