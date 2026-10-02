# Upstream 308-commit candidate audit

Audit date: 2026-09-12. Source: `codebase-memory-mcp` non-merge history (`git log --no-merges -308`). This inventory is deliberately broader than an immediate port queue: it records changes that appear useful to the CLI fork and can be reviewed/ported without importing the MCP frontend.

## Result

There are **150 potentially useful CLI-core candidates** after removing the explicitly integrated, represented, active, and previously categorized commits supplied by the maintainer. Statuses are preliminary:

- **clean**: likely applies to this fork with little or no semantic adaptation;
- **adapt**: shared C/CLI/daemon code, but tests, names, packaging, or fork-specific surfaces need adaptation;
- **blocked**: useful idea, but dependent on an excluded MCP/frontend or a feature intentionally absent from this fork.

No candidate below recommends `src/mcp/**`, MCP tests/adapters, OpenHands integration, Jenkins, or post-commit hooks.

## Existing port-plan completeness audit

This pass also walked the ancestry around every completed first-wave and active second-wave vertical. “Represented” means the behavior is already in the CLI fork even when the exact upstream hash is not present; “categorized” means it is tracked elsewhere and must be resolved before accepting a dependent fix.

| Vertical | Existing integrated/planned commits | Required predecessors or follow-ons | Verdict |
|---|---|---|---|
| Parser budget | `52701625` integrated | No missing source predecessor found; `e5c17de6` is an independent coverage/reporting vertical. | complete |
| LLVM CI/MSan | `557cb010`, `8134712` integrated | `288d4155`, `77063bca`, and `1d38be33` are separate later CI/toolchain pins in the candidate inventory. | complete, review follow-ons |
| Daemon startup/reliability | `33c4fd80`, `0567dc70`, `00060cec`, `ecfb9642`, `8116672a`, `df579830`, `61884951`, `6091db71` integrated | `8e70590d`, `9d4ac2b9`, `fc1b1ee7`, `3a8c0d97`, `7710f86e`, `4a29f0da`, `3e1703d7`, `1a6f80c3`, `daac6bd4`, `7f3e30e1` are later daemon follow-ons; `9846c3f1` is separately categorized and includes an MCP adapter gap. | complete predecessor set; follow-on queue valid |
| CLI uninstall/config safety | `c572ddc4`, `e5aabdf8`, `7bf366f6`, `91c63be5`, `a3c24a71`, `4fa11d1a`, `8135be28` integrated | `a530c8aa` is an already-represented help guard; `4c1b2347`, `4cafe8eb`, `902f85b0`, `1e0aac52`, `1e25d962`, `33d1ecea`, `b54998d2`, `d58962c0` are later independent CLI/client verticals. | complete; no omitted required predecessor |
| Search/pagination | No CLI port planned from MCP frontend | MCP-only pagination/search commits are explicitly excluded. `cc9c2f4c` is categorized as a CLI-neutral canonical-source scan fix and has no required MCP predecessor. | keep excluded; evaluate `cc9c2f4c` separately |
| Cypher limits/scope | `98899ba5` planned; `04ba2fa3`, `426e415f`, `63b99976` categorized | `afc948ba`, `25a7aacc`, and bounded-trail chain `8546070d` -> `0c5ffa50` -> `0d97729c` -> `cd22487c` -> `b3d31ca0` -> `58ef9f19` are distinct required-order candidates. | planned set has no missing predecessor; follow-on chains recorded |
| Pipeline budget/ordering | `699be6c2`, `77a0c7d9`, `e410c86f` planned/categorized | `d024f41e`, `396ac348` -> `e5f3aab0`, and Go-binding chain ending `349aecba` are separate verticals; no hidden prerequisite identified. | valid, do not combine unrelated fixes |
| Daemon memory/session | `21591d51`, `9846c3f1` planned | `c1b9c451` is the earlier Windows separator spelling change; `9846c3f1` should include only the CLI-neutral daemon portion and explicitly omit the MCP session adapter. | adapt with represented predecessor |
| Symlink ownership | `3c854ae2`, `8622d6a0` planned | `3a8b8e82` is categorized as the later agent-root policy follow-on; port in order after inode/opt-in semantics. | valid three-step vertical |
| CLI output/discovery | `b1100a49`, `48149cac` planned; `c25c1620`, `4a9fcd97`, `5f58e233` categorized | `b7952030` is the earlier library log default; quiet-mode changes are CLI-policy adaptations, not MCP dependencies. | adapt; preserve fork output policy |
| Complexity determinism | `e410c86f` planned | CP90 already contains equivalent complexity determinism coverage; compare semantics before porting. | likely represented; verify, then skip if equivalent |
| Coverage reporting | `e5c17de6` planned; `b04f5450`, `a9535092`, `cb167b9d` categorized | `f871178b` and `219b3482` are chain tests/terminology; include them only as evidence or reproduce equivalent tests. | no missing source predecessor; test chain has gaps |
| CI/release provenance | Existing workflow cleanup/pins integrated locally | Candidate `9427dd07` -> `8eff872d` -> `c3604402` is a separate fail-closed release vertical; dependency bumps are not source prerequisites. | review against fork workflows |

No integrated or planned vertical was found to require an unlisted MCP frontend, OpenHands, Jenkins, or hook commit. Where an upstream chain crosses an excluded commit, this artifact records the gap and requires a semantic adaptation rather than a direct cherry-pick.

## Highest-value merge groups

| Group | Commits | Touched paths | Status | Dependency notes |
|---|---|---|---|---|
| Daemon/activation safety | `8e70590d`, `9d4ac2b9`, `fc1b1ee7`, `3a8c0d97`, `7710f86e`, `4a29f0da`, `3e1703d7`, `1a6f80c3`, `daac6bd4`, `7f3e30e1` | `src/daemon/**`, `src/main.c`, daemon tests | adapt | Port in daemon-order; platform-specific tests are optional evidence, not MCP code. |
| Pipeline/Cypher/store correctness | `25a7aacc`, `c86f1ffa`, `afc948ba`, `4cbc9e1c`, `d90f98f5`, `396ac348`, `e5f3aab0`, `b3d31ca0`, `cd22487c`, `0d97729c`, `0c5ffa50`, `8546070d`, `58ef9f19`, `d024f41e` | `src/pipeline/**`, `src/cypher/**`, `src/store/**`, `src/pipeline/artifact.*`, tests | adapt | Keep each fix with its regression test; bounded-trail commits are one dependency chain. |
| Extraction/language coverage | `97517a46`, `592894a4`, `c36b4fbc`, `fd73c347`, `44caa4c3`, `a2ea711c`, `3f831936`, `8f50841a`, `cb7cb444`, `47116b8e`, `0b0d143c`, `95689b5c`, `2910e284`, `b6a22843`, `706bb2ce`, `7b72652a`, `1d6a140f`, `229b4fe1`, `0f1e65d6`, `5cabeb4b`, `43ecc098`, `6b23078c`, `dc0f8ae6`, `4b1cdf57` | `internal/cbm/**`, `src/discover/**`, `src/pipeline/**`, vendored grammars, tests | adapt | Language additions require license/checksum/manifest review and are independently selectable. |
| CLI/install/client behavior | `4c1b2347`, `4cafe8eb`, `8d46d258`, `09c66241`, `902f85b0`, `1e25d962`, `1e0aac52`, `410cd9a3`, `f518eaee`, `23dfea58`, `2bff501a`, `20ad3e5b`, `276664eb`, `33d1ecea`, `b54998d2`, `d58962c0`, `00e0cf38`, `7d4dc779`, `72ee8805`, `ee32816e` | `src/cli/**`, `src/main.c`, `install*`, package metadata, CLI tests | adapt | Review against the fork's intentionally reduced client surface; `4cafe8eb` is potentially blocked if its AGENTS policy is MCP-specific. |
| Resource/performance hardening | `57ef0a6d`, `40f2722d`, `fece72b0`, `ffc29f73`, `92812ae1`, `51770de0`, `f95fe55b`, `a4126427`, `a4c0ffbc`, `32ae20d0`, `e24f0c63`, `793716dc`, `f6e3af43`, `3491a8e8`, `bf46b91f`, `8669ba8f` | `src/foundation/**`, `src/store/**`, `src/pipeline/**`, `internal/cbm/**`, tests | adapt | Benchmark and sanitizer evidence recommended before porting performance-only changes. |
| CI/release/build integrity | `3b173288`, `77063bca`, `288d4155`, `7d896fee`, `1d38be33`, `9427dd07`, `32633bab`, `8eff872d`, `547f355e`, `18edfa00`, `98c1f8c3`, `c3604402`, `2a02d128`, `3308f360` | `.github/workflows/**`, `Makefile.cbm`, `scripts/**`, `flake*`, release contracts | clean/adapt | Apply only to this repository's workflows; do not introduce upstream links or Jenkins behavior. |

## Complete candidate inventory

The following hashes are the exact 150 candidates. The path family is given by the group above; exact paths can be verified with `git show --stat --oneline <hash>` in the MCP checkout. All source-bearing candidates are **adapt** unless listed clean in the CI/build group. Tests-only and documentation-only commits were not counted.

```text
3b173288 f4c7d201 e93db6a9 8d46d258 7e47043a 1770d68a 57ef0a6d c4e90284
e0785b35 572725e6 40f2722d 97517a46 4c1b2347 4cafe8eb 592894a4 c36b4fbc
fd73c347 44caa4c3 25a7aacc 9d4ac2b9 a2ea711c 3f831936 da61c81a 4d20a427
8f50841a c86f1ffa 8e70590d 77063bca cc00a027 17b5a432 288d4155 cb7cb444
47116b8e d6417ada 7d76b88f afc948ba fc1b1ee7 0b0d143c 4cbc9e1c fece72b0
95689b5c d90f98f5 2910e284 b6a22843 5821078e 7d896fee ee0a2e6d 706bb2ce
396ac348 e5f3aab0 7b72652a 1d6a140f fb7cd2f8 229b4fe1 09c66241 7fe2f87b
3f926997 5b96b067 11bbf0d8 0f1e65d6 7710f86e 6c74ae38 ee32816e 97ecfe7f
6c2f82c8 72ee8805 5a02822f fde1695d ffc29f73 3a8c0d97 902f85b0 032ebf53
0df990aa ee126155 0365ef94 680bc72e 48cb94f6 1e25d962 1e0aac52 c537bf0f
3e1703d7 1a6f80c3 547f355e 7583376f fa3a3ea3 410cd9a3 f518eaee 36b7b18b
23dfea58 606052a2 2bff501a 5bbc7b7c d9f3088b 4a29f0da b3d31ca0 1af49dfe
cd22487c 0d97729c 0c5ffa50 8546070d 9eba0279 5cabeb4b 43ecc098 c4224cf3
6b23078c dc0f8ae6 9427dd07 58ef9f19 4b1cdf57 92812ae1 773bb037 109299f2
0eb22f02 8eff872d d024f41e 47bd4b68 20ad3e5b 276664eb 4cd84422 33d1ecea
b54998d2 51770de0 459be8bf d918fce2 cf5eb61c f95fe55b a4126427 a4c0ffbc
32ae20d0 e700b621 e24f0c63 793716dc cffb4e37 170590bc f277034f f6e3af43
3491a8e8 bdb99d77 8669ba8f bf46b91f d58962c0 00e0cf38 7d4dc779 c3604402
329cfc3c a530c8aa 416ba994 daac6bd4 7f3e30e1
```

## Earliest-prerequisite merge order

These are the recommended verticals' contiguous orders. A later fix must not be selected by itself when an earlier commit supplies the data structure or behavior it changes.

- Bounded Cypher trails: `8546070d` -> `0c5ffa50` -> `0d97729c` -> `cd22487c` -> `b3d31ca0` -> `58ef9f19`.
- Go struct extraction/binding: `47116b8e` -> `cb7cb444` -> `cc00a027` -> `d6417ada` -> `7d76b88f` -> `349aecba` (`e07a12b6` is a test-only gap).
- Python bare calls: `0b0d143c` -> `95689b5c` -> `97517a46`.
- Swift scanner safety: `3f831936` -> `a2ea711c` -> `e8204092` (manifest-only gap; verify checksums).
- Cross-LSP resolution: `2910e284` -> `32ae20d0` -> `da61c81a`; `e24f0c63` and `793716dc` are optional later performance extensions.
- Importance scoring: `e5f3aab0` -> `396ac348`.
- Pkl support: `1d6a140f` -> `7b72652a`.
- ArkTS support/provenance: `0df990aa` -> `032ebf53` -> `2b51b7dc` (checksum-only).
- Ensemble routing: `ee126155` -> `6c2f82c8`.
- OMP client: `f518eaee` -> `410cd9a3` (`fa3a3ea3` is formatting-only).
- URL-builder route extraction: `d9f3088b` -> `5bbc7b7c` -> `606052a2`.
- CLI client selector: `1e0aac52` -> `1e25d962` -> `902f85b0`.
- Release integrity: `9427dd07` -> `8eff872d` -> `c3604402`.

Explicit gaps are excluded or categorized commits between an earliest prerequisite and a later fix. Port those gaps semantically, or mark the vertical blocked; do not cherry-pick only the terminal hash.

## Rejected / excluded accounting

- 18 first-wave integrated commits: excluded by exact hash.
- 12 active or categorized second-wave commits: excluded by exact hash.
- 5 already represented skips: excluded by exact hash.
- 18 previously categorized next-tranche commits: excluded by exact hash.
- MCP-only frontend/search/pagination/session changes: rejected regardless of subject; no `src/mcp/**`, MCP tests, or adapters are candidates.
- Tests-only, formatting-only, documentation-only, dependency-only, and empty CI retrigger commits: rejected from the 150-count queue unless part of a source change's dependency chain.
- OpenHands integration, Jenkins, and hook changes: explicitly rejected by scope.

The 150 count is an audit inventory, not permission to bulk-cherry-pick. Each group needs reverse applicability checks, dependency review, and focused CLI validation before implementation.

## Completion update — 2026-09-12

The completed/planned second-wave sources are now merged on the CLI branch:
`d3bb290c`, `c716cb7d`, and `fc9aeedc`. Do not select those source verticals
again.

The high-value vertical plan was then checked from each earliest prerequisite:

| Vertical | Source-chain result | CLI result |
|---|---|---|
| Bounded Cypher trails | `8546070d -> 0c5ffa50 -> 0d97729c -> cd22487c -> b3d31ca0 -> 58ef9f19` all already ancestors | no delta needed |
| Go struct extraction/binding | first five commits already ancestors; `349aecba` absent | merged as `e497dd0c` |
| Daemon activation/rendezvous | seven commits already ancestors; `8e70590d`, `9d4ac2b9`, `1a6f80c3` absent | merged as `6e289d5a` |

The parent integration check passed `git diff --check`, incremental production
build, and the selected pipeline/registry/import/daemon/CLI focused suites.

## Ponytail completion update — 2026-09-12

| Vertical | Upstream source | CLI disposition |
|---|---|---|
| Cypher live scope/capacity | `63b99976 -> 426e415f -> 04ba2fa3` | merged as `a7721244` |
| Registry receiver chain | `2c76563a` | merged as `e52b31cd` |
| Cohort handoff retry | `9104feb6` | already represented by `3036e195`; verified, no duplicate port |

The next Ponytail candidate is the earlier remaining Cypher correctness pair
`25a7aacc -> afc948ba`. Do not reselect the completed source hashes above.

## MCP refresh — 34 new non-merge commits (2026-09-13)

Source window: the next 34 non-merge commits at `codebase-memory-mcp` `main`,
from `3c854ae2` through `09c0e88a`. This refresh is a candidate list only;
it does not authorize a bulk merge. Ponytail screening excluded formatting,
tests-only, documentation-only, dependency-only, represented, MCP-only,
OpenHands, and already-tracked work before selecting the following verticals.

| Priority | Upstream commit(s) | Disposition | Why / dependency boundary |
|---|---|---|---|
| 1 | `91e31211` | **adapt — high value** | Closes the pipeline staging create-to-lock race. Shared `src/pipeline/**`; carry the full lock-before-visible change and regression test, not a partial cherry-pick. |
| 2 | `9a460b38` | **adapt — high value** | Moves SHA-256 message-schedule scratch out of a per-block stack frame, reducing ASan use-after-return overhead. Shared foundation code; retain existing digest vectors. |
| 3 | `09c0e88a -> 1ad52f5f` | **adapt — security CI** | Fails CodeQL alert reads closed and grants reusable-workflow permissions. Confirm this fork's callers/workflow names first; do not add upstream links or Jenkins behavior. |
| 4 | `8c2692c1` | **clean — dependency security** | Bumps Graph UI Vitest to 4.1.11. Select only with package-lock verification. |
| 5 | `fa99ff64` | **adapt — test throughput** | Reduces a proven idle wait in the CLI activation negative probe and marks `cli` as slow for scheduler budgeting. Reassess against this fork's activation fixture before porting. |
| 6 | `013e1f7e` | **adapt — large CLI safety** | Prevents an activation targeting another HOME/cache namespace from draining the active cohort. The upstream diff crosses excluded MCP supervisor code, so port only CLI-neutral cohort-scope logic after caller mapping. |

### Represented, rejected, or not independently useful

- Already represented/integrated: `52701625`, `6091db71`, `0567dc70`,
  `8116672a`, `df579830`, `61884951`, `4fa11d1a`, `557cb010`, `91c63be5`,
  `8134712d`, `ecfb9642`, `a3c24a71`, `7bf366f6`, `8135be28`, and `3c854ae2`.
- MCP-only or MCP-coupled: `f6a29270`, `9ef7da8c`, `e45d7051`, `208ee99c`,
  `262ab012`, and `3a4160b4`; retain the CLI fork exclusion.
- No standalone value: `eeff3f19`, `e65bc6b7`, `7e73b48e` (format/docs),
  `92cb3b9a` (test-only), and `aacf96a2` (installer wording only).

The refresh adds six potential verticals: two core safety/performance fixes,
one CI security pair, one dependency update, one test-throughput adaptation,
and one large scoped activation safety adaptation.


## 2026-10-01 local integration and PR comment follow-up

Candidate `local-mr/jev-integration-20261001` is based on release-tooling
`4d6bd4d8`. It contains COUNT, properties scanner cleanup, Jenkins daemon
recovery, and installer fork defaults (`c9841610`). PR #12 thread
`PRRT_kwDORjrg5c6n5KE9` remains open until Jenkins validation and publication.
PR #8 has no inline review threads as of the GitHub refresh.

| Rank | Upstream slice | Local integration evidence | Remaining gate |
|---|---|---|---|
| 1 | Macro coverage `e4780c20` | `bba31937`, `8e76597f`: containment first, shared line offsets, original byte predicate and allocation fallback | Full Jenkins |
| 2 | Watcher `76e54f75` and QA follow-ups | `322d39ed`, `05cd7731`: capped hard-failure backoff, diagnostics, monotonic scheduling | Full Jenkins; helper tests do not establish scheduler integration |
| 3 | AST auxiliary walkers `7707c9d7` | `db6d1f57`, `f37c02c9`: body-token, branching and complexity stacks grow; metric failure returns unavailable sentinel | Full Jenkins; allocation failure not exercised |

The separate definition-stack ceiling removal in upstream `77ab4bea` is
not included: `CBM_WALK_DEFS_MAX` remains. Do not classify that commit as
fully represented. LSP array optimization `d7eba5a7` is already represented
by ancestor `fcc51bc9` and was excluded from the three slices.

COUNT focused Jenkins build 2 at `784974f9` passed 71 store_nodes checks.
ADR build 2 at `ee4a04e3` failed lint before tests (banned NOLINT comments);
ADR is excluded from this integration. Jenkins recovery build 1 at `523731ea`
passed. These results do not validate the combined candidate.

Full Jenkins validation, independent acceptance, local merge and origin push
remain pending. Agent-Workflow AST correction run `JEV-AST-METRICS-R2-20261001`
finished partial pending Jenkins. Historical external run lifecycle records do
not establish acceptance. Exhaustive commit classification, index reset, package
manager fork defaults, and native Windows/WSL release qualification remain open.

### Shared Jenkins policy and complete AST scope

Maintainer instruction: commit to local `release-tooling` and use the existing
`codebase-memory-cli-release-tooling` job. `CBM_TEST_SUITES` accepts whitespace
or comma-separated names; blank runs the full gate. Do not create task jobs.
All five temporary jobs were retired after archiving configuration and build
logs under `/tmp/cbm-jev-delegation/retired-jenkins`. Historical URLs no longer
resolve; archived logs remain the evidence source.

The AST slice now also includes upstream `77ab4bea` definition-stack ceiling
removal, adapted by Luna commit `c43d8958` with a 1000-definition regression.
This supersedes the earlier missing-scope statement. Source review confirms
all `wd_push` routes share the growth/failure handling; allocation failure is
not runtime-tested. The CLI lacks upstream memory-core headers, so this port
uses its existing heap convention with failure-preserving `realloc`.

Temporary full gate at `93fb593f` passed lint, memory analysis, security and
build, then failed preparation on six packaging version surfaces still at
0.10.8 while the newest stable local tag is v0.11.0. It was subsequently
stopped during cleanup and recorded ABORTED; no suites ran. Six version
surfaces are aligned to v0.11.0 for the next shared-job full gate. Pinned
package versions/checksums and wrapper repository endpoints remain unchanged.
Push and PR thread resolution remain pending a passing shared-job result.

### Linux publication readiness audit (2026-10-01)

Shared Jenkins #34 passed the full Linux gate at `37dd63ba` and archived
Linux amd64 bytes, checksums, version, and source revision. #35 passed focused
`daemon_runtime cli` checks at `7e9005f8`; this does not qualify that newer
revision through the full gate. PR #12 is merged; PR #13 carries subsequent
release-tooling fixes. Jenkins evidence publication formerly pushed directly
to main. The release-tooling correction archives locally without GitHub writes;
only after a passing gate may the orchestrator push release-tooling and post
an exact-SHA status. A regression contract rejects remote writes in the script.

Live GitHub audit: Actions are enabled and all actions are allowed. The active
main ruleset prevents deletion and non-fast-forward updates; it has no required
status checks. This does not establish that a release workflow will pass.
At the initial audit, the only repository secret was CLAUDE_CODE_OAUTH_TOKEN.
VIRUS_TOTAL_SCANNER_API_KEY has since been configured; its validity awaits a
release scan. npm/PyPI credentials and publication are deferred.

Publication blockers in the existing all-platform `release.yml`:

- `VIRUS_TOTAL_SCANNER_API_KEY` is required for candidate scanning and verification.
- `NPM_TOKEN` and `PYPI_TOKEN` are required for registry publication; public
  un-drafting depends on successful registry publication.
- External Windows qualification is held by default. Setting the hold to false
  does not create a Linux-only release: all-platform build/smoke gates still run.
  Registry publication requires both `hold_for_external_qualification=false` and
  `publish_registries=true`; it is skipped when `publish_registries=false`, even
  with the qualification hold disabled. Keep the hold until the applicable evidence
  is complete; do not bypass it to compensate for missing credentials.
- Package wrappers still download upstream releases. Fork endpoints and real
  artifact checksums must be aligned before advertising those installation routes.
- No public or draft fork GitHub releases exist at this audit boundary.

Obtain the service credentials through their providers and supply a local
credential-file or secret-manager reference for secure GitHub secret setup.
Do not commit token values or remove release checks to bypass missing secrets.

Maintainer update: defer all public/registry publication. Both release and
promotion workflows now expose `publish_registries` with default false.
Default dispatches skip npm/PyPI jobs and public un-drafting, so registry
credentials are not required to create/verify a draft candidate. VirusTotal
and the external qualification hold remain unchanged. Publication is still
blocked until explicitly opted in, with credentials and qualification evidence.

## Remaining verticals — source review and Jev ranking, 2026-10-01 PDT

This section supersedes earlier pending/completion statements above. Reviewed
CLI main `bbabc073` and release-tooling `1ce4e164`; upstream remains the frozen
`0f52d30c` census boundary. No fetch, indexing, builds or tests ran in this
assessment. This is a source-reviewed shortlist of seven remaining verticals,
not an exhaustive semantic ranking of the 348 non-merge census entries.

### Finished work removed from the active queue

| Work | Current evidence | State |
|---|---|---|
| Failed COUNT reads and status propagation (`99c3bb4c`) | `14fc5e88`, `7a388edb` | Integrated into main via PR #12 |
| Persisted LSP linear array decode (`d7eba5a7`) | `fcc51bc9`, PR #4 | Merged into main |
| Cypher partial-WHERE / optional COUNT | `8f35274b`, PR #5 | Merged into main |
| .NET XML admission | `72faa8cd`, PR #6 | Merged into main |
| TypeScript awaited generic call edges | `38153854`, PR #7 | Merged into main |
| Properties scanner state and parser-cache cleanup | `008137f1`, `cf92f418` | Integrated into main via PR #12; PR #8 closed |
| Macro coverage (`e4780c20`) | `bba31937`, `8e76597f`, grammar-compatible regression `37dd63ba` | Integrated into main via PR #12 |
| Watcher hard-failure backoff and QA | `322d39ed`, `05cd7731` | Integrated into main via PR #12; live scheduler integration remains unproven |
| Complete AST walker growth (`77ab4bea`, `7707c9d7`) | `26b31280`, `db6d1f57`, `f37c02c9` | Integrated into main via PR #12; allocation-failure injection remains open |
| Empty-graph function-sort guard (`77a0c7d9`) | Current `phase1_scan_functions` guards qsort with `func_count > 0` | Already represented; remove stale backlog candidate |
| Cypher capacity + unnamed-head follow-up (`04ba2fa3`, `98899ba5`) | Current `check_pattern_var_capacity` counts `CYP_ANON_HEAD_VAR` | Already represented; remove stale backlog candidates |
| One-shot index metrics | `0f5a84bc` | Present on main; remove stale implementation task |
| MCP-dependent monolithic test sources | Current `Makefile.cbm` uses neutral operation/daemon sources; no MCP entries in source lists | Old unresolved-link blocker is obsolete; no new runtime validation claimed |
| Installer fork defaults and Jenkins recovery | `c9841610`, `8d64e6e7`, PR #12 | Merged into main |
| Windows hook-lifecycle/macOS connection-cap repairs | `bbabc073`, PR #13 | Merged into main |
| Deferred publication, Claude removal, CodeRabbit enablement | `b05fbcd0`, `9c6e795c`, `810725ab` | Committed on release-tooling; PR #14 open |
| Linux-only prerelease route and package fork endpoints | `1ce4e164` | Committed on release-tooling; qualification and PR #14 merge remain open |

Historical Jenkins results apply only to their recorded revisions. Live PR #14
at `1ce4e164` has failing GitHub diagnostic, macOS LeakSanitizer, Unix/Windows
test and shard-completeness checks, with additional checks still running as of
2026-10-02T04:14Z. Do not equate implemented release tooling with CI acceptance
or an available release. Failures were observed, not diagnosed in this ranking.

### Next three selected vertical slices

| Rank | Slice / upstream commits | Source-grounded value and minimal adaptation | Jenkins acceptance |
|---|---|---|---|
| 1 | Daemon cache-ownership safety — `ed76cf8e` | Current `cli_activation_production_reserve` sends an eager activation-shutdown request before a positive cache-ownership check; the daemon validates requester build but the shutdown frame has no cache identity. Adapt the positive-match/unknown-is-foreign rule to this fork's transaction lifecycle, including the later quiescence callback. **M–L; highest adaptation risk.** | `cli,daemon_runtime,daemon_ipc`: same-cache activation still works; different cache, busy/unreadable ownership and raced ownership send no foreign shutdown and preserve sessions. Keep exclusive mutation authority and lock cleanup. |
| 2 | Binary `.res` admission and ReScript traversal/scanner termination — `8093d10b`, checksum follow-up `ea6fa49f` | Current discovery lacks `.res` disambiguation; `is_first_named_part_of` still climbs via repeated `ts_node_parent`. Port admission, cursor-based traversal and scanner progress together, preserving valid ReScript and vendored integrity. **M.** | `language,extraction,complexity`: binary resources bypass text parsing; malformed/adversarial source terminates within a bounded check; valid source keeps its definitions/calls; cursor fast path and checksums verified. |
| 3 | Preserve internal cache stores during project-index reset — `46015320` | `cbm_list_indexes`, `count_db_indexes` and `cbm_remove_indexes` currently accept every nonempty `.db` stem, including `_config.db`. Reuse one exact-filename predicate for those callers and cross-repo enumeration; do not reject legitimate names merely containing `config` or starting with `_`. **S–M.** | `cli,pipeline`: reset/list/count exclude `_config.db` and `_cross_repo.db`; project DBs and their sidecars still follow current reset semantics; internal bytes/sidecars remain unchanged; legitimate similar names remain visible. |

All three are adaptation candidates, not clean cherry-picks or accepted fixes.
Implementation stays in isolated worktrees, integration targets local
release-tooling, and validation uses the existing shared Jenkins job; no new
per-task jobs. Do not push until the required Jenkins evidence passes.

### Other reviewed candidates and Jev evidence

Jev `jev-1.13.0` scored seven alternatives in two independent batches using
verbatim upstream production diffs and current release-tooling source excerpts.
No previous Jev results, agent verdicts or test claims were supplied as evidence.
The same rubric was used throughout: 0 defer, 1 useful, 2 high, 3 urgent.
Scores are advisory and are not probabilities of correctness; batch boundaries
and a single call's numerical precision are limitations.

| Candidate | Score / 3 | Full distribution P(0), P(1), P(2), P(3) |
|---|---|---|
| Daemon ownership | 2.95 | 0.01, 0.00, 0.02, 0.97 |
| ReScript hang | 2.86 | 0.02, 0.03, 0.04, 0.91 |
| Internal cache stores | 2.60 | 0.03, 0.06, 0.19, 0.72 |
| Ancestor ignore rules (`c8184ed9`, `282fbfcd`) | 2.15 | 0.02, 0.02, 0.75, 0.21 |
| C return pointer/qualifiers (`343d7dd5`) | 1.64 | 0.03, 0.31, 0.65, 0.01 |
| Publish errno/failure diagnostics (`460ebefc`) | 1.31 | 0.07, 0.70, 0.06, 0.17 |
| Python complete-binding lookup (`f339aa62`, `df8dd101`) | 0.98 | 0.07, 0.89, 0.04, 0.00 |

The question for each candidate assessed priority from concrete harm, current
source applicability, adaptation cost and regression seams, with destructive
state loss, unrelated-session interruption and hangs ranked above diagnostics
or speed. Jev supported the selected three; exact source inspection determined
what is already represented and what needs fork-specific adaptation.

Graph discovery used generation `2026-09-29T06:21:14Z`; CLI source coverage is
partial and metadata-changed, so exact release-tooling source reads were used
for material claims. Git porcelain was unchanged by these read-only probes.
Fresh graph resets, exhaustive census classification, ADR alias/outline lint
repair, native Windows/WSL qualification and real release-artifact checks
remain open. Package endpoint implementation is complete on release-tooling;
package installation/runtime qualification is separate.

## Three-vertical specification and interactive dispatch preparation — 2026-10-02

Specifications are finalized in SpecGen `agent-workflow` mode (zero readiness
blockers), committed on release-tooling at `51aa7a86`. Each defines five must
requirements, acceptance/evaluation links, preservation boundaries and structured
worker evidence. See `docs/specs/upstream-next-three/{daemon-ownership,rescript-hang,cache-stores}/spec.json`
and their derived `SPEC.md` projections.

| Vertical | Durable Agent Run | Independent branch | Execution state |
|---|---|---|---|
| Daemon ownership | `CBM-DAEMON-OWNERSHIP-20261002` | `impl/cbm-daemon-ownership-20261002` | Prepared external; not launched |
| ReScript hangs | `CBM-RESCRIPT-HANG-20261002` | `impl/cbm-rescript-hang-20261002` | Prepared external; not launched |
| Internal cache stores | `CBM-CACHE-STORES-20261002` | `impl/cbm-cache-stores-20261002` | Prepared external; not launched |

All worktrees share frozen base `51aa7a86`, with GPT-6-Luna / medium reasoning,
interactive Codex, `danger-full-access`, and approvals `never`. Global runtime
configuration was not changed. Builds/tests remain parent-scheduled in the existing
shared Jenkins job; workers may format, inspect diffs and commit regression source.

The requested Herdr right-hand panes could not be launched because this caller
is outside a Herdr-managed pane (`HERDR_ENV` unset). No Herdr control was attempted
and no headless fallback was substituted. The durable launch handoff is
`/home/nate/.local/state/agent-workflow/handoffs/CBM-THREE-VERTICALS-20261002/README.md`.
Preparation is not running, implementation completion, review or acceptance.

### Interactive Herdr launch verified — 2026-10-02T04:52Z

The maintainer confirmed this is a Herdr session and authorized setting
`HERDR_ENV=1`. Herdr then verified the current Codex parent in `wC:p1`.
All three external workers are launched in the same tab, stacked in the
right-hand column; the parent's focus remains unchanged.

| Vertical | Herdr worker / pane | Durable delivery and live state |
|---|---|---|
| Daemon ownership | `luna-daemon-ownership`, `wC:p5` | Working; steering acknowledged by worker |
| ReScript hangs | `luna-rescript-hang`, `wC:p6` | Working; steering acknowledged by worker |
| Internal cache stores | `luna-cache-stores`, `wC:p7` | Working; steering acknowledged by worker |

Each prepared Agent Run is bound to its Herdr pane at external generation 1;
`start-external` records launch, and all three correlated steering IDs have
worker acknowledgements. Herdr's actual launch argv confirms GPT-6-Luna,
medium reasoning, `danger-full-access` and approvals `never`. This supersedes
the earlier prepared/not-launched state. Implementation, Jenkins evaluation,
independent review, acceptance and integration remain separate pending gates.

### Daemon/cache worker closeout — 2026-10-02

Both workers finished committed implementation candidates and were idle.
Daemon branch `impl/cbm-daemon-ownership-20261002` retains `028a4bfb`;
cache branch `impl/cbm-cache-stores-20261002` retains `fa5a180b`.
Their worktrees were verified clean, bundles verified, patches/reports/terminal
transcripts preserved under each run's `evidence/closeout/`, and Herdr panes
`wC:p5` / `wC:p7` closed at the user's request. Only those two worktrees were
removed; branches remain available for review/integration.

Worker `agent finish --result partial` had produced handoff completions but left
controller summaries running with placeholder blocked completions. Public
`external-exit` followed by `finalize` sealed valid partial results at the actual
candidate SHAs, closing both assignments without fabricating process results.
Jenkins evaluation, independent review and acceptance remain pending. Daemon
AC-005 still lacks a direct CLI diagnostic assertion. ReScript was not closed.

## Updated upstream audit dispatch — 2026-10-02

Frozen source boundaries: CLI release-tooling `9cb5cc21`, locally updated MCP
`96c3f41c` (dirty `graph-ui/tsconfig.tsbuildinfo` preserved). The MCP clone was
already current per maintainer; Git objects were copied from the local clone
without a remote fetch or moving shared CLI refs. `0f52d30c..96c3f41c` contains
19 new non-merge commits; `9cb5cc21..96c3f41c` contains 367 ancestry-differing
non-merge commits. These are deterministic census counts, not completed semantic
classification or applicability claims.

| Agent Run | Independent worktree / branch suffix | Herdr pane | Task |
|---|---|---|---|
| `CBM-UPSTREAM-ASSESS-20261002` | `cbm-upstream-assess-20261002` | `wC:p8`, `codex - luna-upstream-assess` | Source/Jev assessment of new and older unclassified commits; group applicable verticals with prerequisites/prior QA commits; propose canonical ledger/backlog edits on isolated branch |
| `CBM-UPSTREAM-VERIFY-20261002` | `cbm-upstream-verify-20261002` | `wC:p9`, `codex - luna-upstream-verify` | Independent full-hash/source coverage audit of all differing applicable commits; evidence/report only, no competing canonical-doc edits |

Both GPT-6-Luna interactive workers launched with full access, approvals never,
and acknowledged generation-1 durable steering. Each worktree was indexed in
full mode with persistence false BEFORE Agent Run preparation/worker launch;
projects/status verified the exact root and ready graph, and Git porcelain was
unchanged. Assessor graph: 26,722 nodes / 130,694 edges. Verifier graph: 26,722
nodes / 131,278 edges. Both report 91 parse-partial files; exact source fallback
and path-specific coverage remain required. Counts do not imply graph completeness.
The initial prompts include verified index receipts and the shared Jev footer.

No audit completion, independent acceptance or integration is claimed yet.
Parent will reconcile canonical proposed edits against the independent report.
Previously implemented daemon/cache/ReScript candidates remain unmerged and
unvalidated; do not confuse their surviving branch commits with accepted ports.
Compact handoff, frozen universes, index/process receipts, native launch and
acknowledgement evidence are retained at
`/home/nate/.local/state/agent-workflow/handoffs/CBM-UPSTREAM-AUDIT-20261002/`.


## Three-candidate integration review resumed — 2026-10-02

The parent pulled `main` (`bbabc073`, already current) and fetched upstream
`96c3f41c`. The daemon (`028a4bfb`), cache (`fa5a180b`) and ReScript
(`0eaa19f8` + checksum follow-up `9766a378`) candidates merge without conflicts
onto the frozen QA branch `5fe820df` in isolated branch
`integration/cbm-upstream-three-20261002`. They remain candidates until the
exact integrated revision passes the shared Jenkins job and review.

Parent source review corrected two defects before promotion:

- ReScript deadline-test child now reaches its parse/alarm path before the
  parent-only positive-PID assertion.
- Ownership-aware activation verifies the lifetime identity under maintenance
  SH and admission EX before publishing maintenance EX; a nonblocking
  promotion preserves the admission/maintenance deadlock boundary. Foreign or
  unreadable ownership never publishes the cancellation signal.

Regression coverage adds admission-contention maintenance observation and a
production CLI foreign-cache refusal diagnostic with binary/index preservation.
The existing activation-ordering fixture now fingerprints its actual test cache.
Pinned formatting, whitespace checks and ReScript manifest/scanner checksum
comparison passed. No local builds or tests ran. Requested Jenkins suite union:
`cli pipeline str_util version_cohort daemon_runtime daemon_ipc language extraction complexity`.
Jenkins evaluation, runtime acceptance, remote push and main integration remain
separate pending gates.

The requested GPT-6.1-Sol audit is independently running as
`CBM-UPSTREAM-SOL-20261002-R2`, pane `wC:pA`, against frozen QA base
`5fe820df` and upstream `96c3f41c`. Its scope includes all upstream merge
commits, constituent commits, canonical backlog reconciliation, vertical
grouping and the next five new slices. Its exact-worktree index completed with
persistence disabled and unchanged Git porcelain; launch and steering were
observed and acknowledged. This is launch evidence, not audit completion.
