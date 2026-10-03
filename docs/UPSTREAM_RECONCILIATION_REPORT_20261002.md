# Upstream reconciliation evidence report — 2026-10-02

**The history inventory is complete; the semantic audit is partial.** This report
collects the Sol source audit, Luna handoffs, corrections, QA delta and remaining
limits. It is a dated synthesis, subordinate to [BACKLOG.md](../BACKLOG.md) and
[the canonical 308 ledger](UPSTREAM_308_CANDIDATE_AUDIT_20260912.md). It creates no
second implementation queue and establishes no review, acceptance or publication.

## Comparison boundaries

| Tree | Frozen full revision | Meaning |
|---|---|---|
| Integrated upstream | `96c3f41cf334d87670cb085f1fcf16f637293222` | Reachable merged history under audit |
| Production origin/main | `bbabc07386f347ac416a60fe530b1007ced5c27b` | Frozen shipped-source comparison |
| Original QA | `5fe820df8bfeec059fa646e0050c1652f38a4165` | Audit worktree source baseline |
| Later captured QA | `559214af28ce2d6f5d6ec86662f6871316d28b45` | Candidate integration, runtime-unaccepted |
| Common ancestor | `17786374dfc39b8c751933d34d269501f2b4d6b7` | Shared history boundary |

Original QA is 13 commits ahead of production; `src/`, `internal/` and
`Makefile.cbm` are identical. Its release/CI/package/documentation delta cannot be
attributed to production. The isolated audit branch adds documentation only; a
fresh base-to-HEAD source comparison is empty. The later QA boundary is a captured
snapshot, not a claim about today's live refs or GitHub state. No ref was fetched
or moved for this consolidation.

Canonical primary dirty snapshots were preserved and reconciled in the isolated
branch: 123/610 backlog/ledger lines versus 104/438 at original QA. Existing dirty
files, deletions, notebook untracked files and other worktrees were not overwritten.
The comprehensive report is a proposal committed locally for parent review.

## Complete ancestry and constituent inventory

| Exact inventory | Count |
|---|---:|
| All upstream reachable commits | 3,352 |
| Actual merges / non-merges | 923 / 2,429 |
| Differing merges / non-merges against frozen production/original QA | 311 / 367 |
| First-parent commits / actual merges | 1,412 / 724 |
| Differing first-parent merges | 202 |
| Differing non-merges with equal patch ID | 3 |
| Differing merges with nonempty combined diff | 24 |
| Cached upstream-only branch refs outside integrated scope | 69 |

Actual merges are identified by multiple parents, not subjects. Every merge has
full parents, first-parent membership, first-parent result paths, constituent
non-merges and nested merges in the crosswalk. Introduced ancestry for `M` is
`rev-list M --not M^1`, excluding `M`; nested merges are retained separately.
Later captured QA also retains the same 311/367 ancestry-differing hash sets;
its source adaptations do not import those upstream originals into history.
All 923/2,429 hash sets and the 311/367 differing sets were checked against the
frozen Git objects again during consolidation. Each differing non-merge has
exactly one auditor assignment. Constituents can occur in nested containers;
those memberships are not unique implementation tasks.

The 612 inherited merges and 2,062 inherited non-merges are ancestors. They were
not freshly semantically or operationally revalidated. Equal stable patch IDs for
`25a7aacc87250bff752ff61cd33e9236be5d1310`,
`349aecba9528427fa635975e246f5b8959b14f30`, and
`557cb0102863e66cbd5cc622509a29a4964d1831` establish a mechanical relationship;
they do not close test, followup or acceptance requirements. An empty `--cc` diff
likewise does not establish safe merge resolution or whole-result parity.

The 69 cached upstream-only branch refs have commits outside the frozen integrated
upstream tip. Full tips and hashes are in `upstream-branches-outside-scope.json`.
Remote freshness and open-PR status were not checked, so these are separately
excluded from integrated scope rather than silently counted as missing merged work.

## What the evidence can establish

| Claim | Required evidence | Result here |
|---|---|---|
| Ancestry | Exact Git reachability | Complete frozen census |
| Patch equivalence | Stable patch ID | Three matches; discovery only |
| Semantic representation | Target source behavior, dependencies and regression hunks | Bounded checks; unresolved remainder explicit |
| Grouping | Common behavior plus prerequisites/fixes/checksums/tests | Source-checked verticals and provisional containers |
| Implementation | Scoped source commit/tree | Historical main work and later QA candidates separated |
| Review | Independent findings/disposition | R4 full-hunk review pending |
| Jenkins validation | Existing venue artifacts at exact revision | No new result collected or triggered |
| Acceptance | Host decision against evidence | Not asserted |
| Integration | Exact target ancestry/tree | Frozen production versus later QA distinguished |

Fourteen Sol R2 source-only rows cover bounded behavior: failed COUNT reads, linear
LSP array decode, TS awaited generic calls, optional COUNT, .NET XML admission,
properties payload state, macro scanning, specified growable AST stacks, Cypher
parser/variable-capacity guards and empty-function sorting. Those are not whole
commit certificates. Watcher scheduler execution and AST allocation-failure
injection remain unestablished. The scratch lifetime change is separate from
previously ported growable stacks.

Historical Jenkins #34 at `37dd63ba5b4dfc45790cad6722e7e5c097216607` and #35/GitHub evidence tied to `bbabc07386f347ac416a60fe530b1007ced5c27b`
remain historical claims at those revisions. They do not validate the later QA
ports, this audit's candidates, or current PR #14. Its remote `1ce4e164033d6407c902b6d3b462558df2f30ee6` red checks
are a prior snapshot; they were neither refreshed nor repaired here.

## Auditor results and recovery

Original headless workers lost their live processes near 18:16Z despite `running`
projections. Their evidence remains preserved; process-loss cause was not proved.
Unstarted R2 headless retries were superseded at the user's request. R3 interactive
Luna high workers ran in real managed Herdr panes with independent full exact-root
indexes and clean frozen QA trees. All R3 handoffs were **partial**:

| R3 auditor | Assignment | Touched rows | Untouched rows |
|---|---:|---:|---:|
| Extraction | 111 non-merges | 10 | 101 |
| Runtime | 170 non-merges | 20 | 150 |
| History | 86 non-merges plus 311 merges | 4 non-merges | 82 non-merges; all whole merges open |

All assignment sets and all 40 declared artifact checksums (6/25/9) were verified.
Thirty-four touched rows were not 34 proven whole commits: some regression hunks
were missing or unverified, and some verdicts confused presence upstream with
representation in the target. Raw claims remain available separately from parent
judgment. None was automatically accepted from a finished terminal state.

R4 successors preserve official retry lineage and continue interactively in
`wC:pB`, `wC:pC`, `wC:pD`. They retain all remaining assignments and incrementally
save full diffs/source/coverage. The capture time is `2026-10-02T23:53:00.456924+00:00`. It is not a sealed result:

| R4 auditor | Rows accounted for | Non-unresolved raw claims | Unresolved raw claims |
|---|---:|---:|---:|
| `CBM-RECON-EXTRACT-20261002-R4` | 111 | 30 | 81 |
| `CBM-RECON-RUNTIME-20261002-R4` | 170 | 5 | 165 |
| `CBM-RECON-HISTORY-20261002-R4` | 86 | 19 | 67 |

These 54 claims are pending independent whole-hunk/target/test review. The
remaining 313 worker claims are explicitly unresolved. The conservative parent
inventory retains 364 non-merge whole-commit remainders and all 311 merge remainders;
three exact empty-tree changes require no source port. Full hashes, bounded parent
dispositions, groups and provisional R4 claims are in the canonical appendix.
Do not subtract touched rows from the parent remainder or infer semantic coverage
from assignment completeness.

## Source corrections and additional gaps

The strongest new correction concerns `21591d51168d26f931f64d62edc36013cd1f6249`.
In `src/daemon/application.c:1709–1717`, production and both QA trees count active
jobs but divide aggregate memory by `physical_job_limit`. Upstream lines1760–1770
divide by `active`. Function/log names and the default four-worker test do not
prove equivalence. Fork comments/test expect fixed quarter-budget slices. Upstream
adds a spawn-time active-slice test; an already-running worker retains its old cap
as concurrency grows. Whether direct porting preserves the aggregate global
budget is unresolved. Record a behavior difference and reservation-policy review;
copying the divisor alone is not a ready adaptation.

Merge `2b8bd0c065c1d2728994fddc1683a66f70745748` separately contains Windows
Job Object memory-cap/peak evidence in `src/foundation/subprocess.[ch]`, plus
MCP-only supervisor floor/1.5x headroom diagnostics. The neutral subprocess
capability is missing in the fork and potentially usable by
`src/operations/index_supervisor.c`; its policy and regression adaptation need
review. Exclude direct MCP adapter imports, not the entire shared capability.
The merge's broad first-parent result is still unresolved after inspecting these
hunks; constituent review or one function snippet cannot close it.

Parent frozen-source checks also establish these bounded gaps:

| Upstream constituent | Target evidence and missing behavior | Adaptation / remaining review |
|---|---|---|
| `62b44519c68b403ea44d9a85ba8ee74cddee66e6` | `src/graph_buffer/graph_buffer.c`, `edge_props_confidence`, uses `strtod(..., NULL)`; upstream checks the end pointer | Localized strict numeric parse; review malformed/empty/trailing cases and tests; first additional candidate |
| `592894a4387a50e1d128ff6a2695ff48f8cc32a5` | `internal/cbm/extract_calls.c`, `extract_handler_arg`, chooses first eligible handler and caps scan | Terminal eligible handler after middleware; route followup; full regression/dependency review open |
| `c36b4fbc446f9085704a6073b81ec743fd4180fc` | `internal/cbm/extract_defs.c` limits outer annotation argument scan to three positions | Scan all outer positions while retaining bounded inner traversal; route followup |
| `1e25d962d69999438229b0562d44ea60e97f5456` | `src/cli/cli.c` install/uninstall registry dispatch lacks upstream client-selection guards | Apply supported CLI selection policy, preserve unrelated registrations; full interface/tests open |

R3/R4 also identify source-backed candidates requiring parent whole-hunk review:
Python parameter shadowing `95689b5c20101db7d92b69767bbea75e5bf0d4fc` with
superseded bounded lookup `0b0d143c2d733e3313eef5b9d9745b0fb08cc2c0` and final
walk-carried lookup `97517a4652f376cb6bf096255e235edf456f86b1`; scratch stack lifetime
`40f2722d6166c1f97fa1667c1e63e8c97e6f05f0`; dotted JS/TS import basenames
`48cb94f65ce221e1494119d850d35019c86a59e2`; Swift argument unwrapping
`c4e902849f8062e37a0f0ddaed1c6cc042780cb3`; Elixir positional arguments
`dc0f8ae642bf39b4b11bec2a88a8cdc7a1e4ab5b`; Ruby base collection
`4b1cdf57804afffbcb61e69fd484b42858fcc02f`; strict numeric environment parsing
`6b801eec68d6465aad2020de438e629cead99f5c`; FreeBSD package/UI resolution
`2bff501a62a3bba1aeb39f49f7c785d541ca209e`; EOF VT/FF/CR whitespace
`572725e61a00b9deda7407971dc9290477170515`; and EINTR-safe spawn backoff
`c537bf0f8b3b6a77e39df49fec78560b5e8e87b7`. These are findings to reconcile,
not an independently accepted additional queue.

Mixed saturation/error-reporting commit `3909e5a0e1d55236fccac9ab680a1c6103ce5daf`
has shared Cypher/BFS behavior and MCP/UI response hunks; adapter exclusion must
be at the hunk seam. Followup `619c6834807532b69c419454fd53074d6d10050a` requires
the truncation carrier. Parse-unusable/range-drop marker
`a953509275a4fa42a907f88852d62a613f27bc56` also mixes shared extraction with MCP
rendering. Result-field, neutral CLI formatting and test dependencies require
review together. A path/title exclusion cannot discharge the shared portions.

## Older canonical rows, exclusions and grouping

All 87 old selected/deferred/excluded rows (16/24/47) and 190 distinct revision
tokens have full-object crosswalks. This resolves references, not historical
semantics. The canonical appendix preserves each old label next to the current
bounded disposition. Every differing non-merge has one primary group; the 199
Sol groups include checked verticals and provisional native first-parent containers.
A broad container remains to be split by behavior, not imported as a single slice.

Important prerequisite and followup corrections:

- ReScript checksum `ea6fa49f` follows `8093d10b`; JSX checksum `c5c13a87` follows
  grammar `1b795589`. Full objects are in the canonical token crosswalk.
- Swift manifest/checksum `e8204092333ee1c38cd681a81abb36f125288894` stays with both
  width fixes. No runtime delta does not mean no integrity work.
- SQL regression `259dbb67` follows `c0a06b3d`; watcher regressions `07eb426c` and
  `5a9c72d8` remain grouped. Source backoff presence does not supply
  `4fc5999a85f50bbb131dd87c5f41949c08617c1a`'s absent accessor/test seam.
- `fa229211` touches shared pipeline artifact-export failure state; its MCP title
  does not justify whole-commit exclusion. `4c466fc4` has partial build-stamp
  coherence: ordinary C objects are covered, grammar/LSP/preprocessor/vendor
  recipes and complexity-test cleanup remain to inspect/adapt.
- Test reliability/harness claims such as
  `7e47043acce9bdfd87e7becc7fe7a348b02753ec` and
  `132e8fc32309e843ccd97138c8f032e317333673` are not missing runtime features,
  but applicable validation followups cannot be discarded on that basis.

Only exact empty tree changes can be excluded without a runtime behavior question.
Pure MCP response/session adapters require exact hunk reasons; their exclusion
never proves neutral CLI parity. Historical artifact-sharing draft
`9baa0a8dc5cbe7a9fa687e0a8811dfcbdcd0551e` is superseded by
`957b27fca50d2c3b1ac86d771733f39612bae841`; source review must compare the final
practical documentation, not label the draft's title missing forever.

## Captured QA integration and production boundary

Daemon ownership `028a4bfb4233a0cda91ad57c6859f214fcb112a8`, cache stores `fa5a180b7afee333109b3a1041f102fc74562bb8`, and ReScript hang work
`0eaa19f84544151068111a262772d32d5c5e9a64` plus `9766a378a73905cc7ad1101c0358bf64e40e8fc9` were candidates when launched. Parent subsequently
reported integration repairs including `42b0248e9571d5e72bf51944e44d38e0e45e14ec`, fixture followups, ADR alias/
outline and hook-environment ancestry in captured QA `559214af`. Source comparison
confirms daemon/cache/ReScript and ADR changes. Parent found child assertion
placement and maintenance intent-before-ownership defects, then compile signature
and ADR pagination/allocation fixes. These are parent-reported repairs, not a
Jenkins success or independent acceptance established by this audit.

The pre-commit file has no original-to-later QA tree delta and remains formatting
only. Candidate ancestry does not establish production Git-child environment
isolation or restore test hooks. The three ports and ADR are QA-integrated,
runtime-unaccepted and outside frozen production; they are excluded from the
next five **new** slices. Preserve later QA's ReScript manifest/checksum row when
adapting Swift. No candidate was pushed or merged to main by this auditor.

## Next five new verticals

The authoritative source/dependency/acceptance detail is in
[the canonical recommendation section](UPSTREAM_308_CANDIDATE_AUDIT_20260912.md#next-five-new-vertical-slices).
This condensed comparison records why the five remain recommended among inspected
work; unresolved commits may change the order. Every slice is absent in the
relevant frozen production/original/later QA source. Parent selects existing
shared `codebase-memory-cli-release-tooling` Jenkins suites and records the actual
revision/result. No suite below was run or scheduled here.

| Rank / impact | Constituents in adaptation sequence | Actual upstream first-parent merge | Smallest adaptation and meaningful acceptance | Existing Jenkins suites |
|---|---|---|---|---|
| 1. Memory safety; very small | `2fabaa2b877986bdededb4f4f78e9a31014c6fdf` | `ea7e01e91e64da62a74c1abbfe7a138710d1c88a` | `src/cypher/cypher.c`, `cross_join_with_rels`: reuse `binding_out_append`; four missing-label OPTIONAL inputs retain four distinct rows with unbound optional variables and no overflow, retain nonempty/COUNT behavior | `cypher` |
| 2. Persisted index correctness; small | `e7324bb6904c6d0a295b956e83c8585219198358` | `6bce86383ea192a2f0fbf795e161d3ddca4a78cc` | `internal/cbm/sqlite_writer.c`, `fill_interior_page` / `pb_build_interior`: give back final cell and rewind when only a right child would remain; long-key row sweep, rightmost miss/delete and SQLite integrity | `sqlite_writer,graph_buffer,pipeline` |
| 3. Scanner UB and integrity; very small | `3f831936b5f3012d13175e7c97326165efecefad` → `a2ea711c29560511eb8467bea7ad309ebf500d6e` → `e8204092333ee1c38cd681a81abb36f125288894` | `620613ede6a983f04b7c1539a2ea4c65b34d42f6` | Swift `eat_operators` / `OP_SYMBOL_SUPPRESSOR`: use `1ULL` at both sites, retain regression and fork-byte manifest/checksum provenance; preserve ReScript row; force-unwrap/try! extraction and failing UBSan trap policy | `language,extraction,grammar_regression` plus integrity gate |
| 4. Discovery completeness; medium | `c8184ed94887a6b5e130726885b44a07c63a47b7` → `282fbfcdabfec79c19c36f5bec5f75c22eb36df5` | `92910147aba1e56a1cb8fff7da5e74dbf8011b35` | `discover_impl`: reuse ignore chains/walk helpers with per-ancestor base offsets; rooted patterns, intermediate ignores, deeper negation, linked worktree/common-dir exclusions and OOM cleanup agree with Git | `discover,gitignore,git_context,pipeline` |
| 5. Deterministic graph and complete routes; small/medium | `6b48097471e8d1a3e5cf16447304899a21e12d7a` | `f1e51133e9ec0708756e0ca9311b6b0f3a8cca34` | `pass_route_nodes.c`, `match_one_infra_route` / `create_grpc_routes`: total QN ordering and exact-size service pointer array, free on all exits; >64 services and permuted insertion orders yield identical HANDLES sets | `pipeline,parallel` |

Existing parser/capacity groundwork is not a substitute for OPTIONAL buffer growth.
SQLite atomic-publish diagnostics are independent. Ancestor ignore fix must include
its anchoring followup. Route change can be adapted without the omnibus memory
core or unrelated spill/staging tests; those hunks remain unresolved separately.
If route scope adds handler-position/late-annotation fixes, add their full hashes
and meaningful middleware/late-position regressions plus existing
`edge_types_probe,extraction` selections before implementation. They are related
followups, not prerequisites for the narrow route admission slice.

Jev `jev-1.13.0` advised on six bounded prioritization alternatives from 52,640
bytes of verbatim primary requirements/source. Full request/answer distributions
are in consolidation evidence; no prior verdicts or Jev answers were supplied.
Scores (out of three) and P(defer/useful/high/urgent):

| Candidate | Score | Distribution |
|---|---:|---|
| Cypher | 2.89 | 0.01 / 0.01 / 0.08 / 0.90 |
| SQLite | 2.71 | 0.02 / 0.05 / 0.14 / 0.79 |
| Swift | 1.69 | 0.03 / 0.29 / 0.65 / 0.03 |
| Ignore | 1.93 | 0.08 / 0.26 / 0.31 / 0.35 |
| Routes | 1.97 | 0.04 / 0.11 / 0.69 / 0.16 |
| Confidence parse | 2.03 | 0.01 / 0.08 / 0.78 / 0.13 |

Exact safety evidence and tiny Swift effort retain its third place; small advisory
score gaps do not establish queue order. Confidence parsing is first additional.
Jev provided decision support, not semantic proof, code, permissions or validation.
The history worker separately advised reservation accounting before active-job
division; that provisional reasoning does not replace fork policy/source review.

## GitHub's behind count and honest integration

GitHub compares commit ancestry. Adapting/cherry-picking upstream behavior generally
creates different hashes; equal source or patch ID does not make the originals
ancestors. Editing commit hashes rewrites the objects and descendants. This report
cannot alter GitHub's count, and no rewrite or remote action was performed.

A normal upstream-history merge would record ancestry and change the count, but
only an independently reviewed integration that handles missing/excluded behavior
and all resolution hunks would honestly establish incorporation. An `ours` merge
would mark unresolved history incorporated while retaining missing source. Keep
the frozen census plus canonical semantic ledger until an authorized integration
has reviewed resolutions and exact-revision Jenkins evidence. Do not claim the
fork caught up globally from counts, selected ports or a paperwork update.

## Evidence custody, reproducibility and precise limits

Durable authority for this consolidation is `CBM-UPSTREAM-CONSOLIDATE-20261002`;
original Sol R2 and partial R3 evidence remain unchanged. Machine-readable
crosswalks/grouping stay in run evidence rather than another repository ledger:

| Artifact | Purpose |
|---|---|
| `consolidation-summary.json` | Exact capture time, sets, raw checkpoint counts and conservative limits |
| `nonmerge-crosswalk.jsonl` / `merge-crosswalk.jsonl` | All 2,429/923 commits, exact memberships, parent notes and raw claims |
| `first-parent-history.tsv` | Complete upstream first-parent history |
| `canonical-reconciliation.jsonl` | 190 revision tokens / all 87 older rows resolved to objects |
| `slice-grouping.json` / `next-five-slices.json` | One primary group per differing non-merge; source-grounded shortlist |
| `unresolved-dispositions.tsv` | Full-hash 364 non-merge and 311 merge remainders |
| `auditor-checkpoints/` / `artifact-integrity.json` | Preserved partial R3 artifacts and hashed live R4 checkpoint copies |
| `frozen-source/`, source/combined diff receipts and `graph/` | Exact material source, traces, coverage and fallback |
| `jev-prioritization-{request,response}.json`, `jev-decision-use.md` | Question/model/distributions and actual effect on ranking |
| `source-drift-check.json`, `doc-consistency-review.json` | Doc-only provenance and final permitted checks |

The initial exact-worktree nonpersistent index receipt recorded 26,723 nodes /
128,682 edges at generation17:30:32Z. The later live status is ready at the same
root with 26,736/128,694 and still 91 partial files; it exposes no generation
field. Source-to-original-QA diff remains empty. Do not infer unchanged generation
from source identity or call the counts exhaustive. Earlier saved material coverage
receipts carry generation, qualified complete searches and both-direction traces;
all cited partial/ignored ranges use exact source fallback. `vendored` scopes and
MCP adapter paths require direct frozen Git reads. No reindex/cache-root/daemon
change was necessary for the doc-only consolidation.

Acknowledgement correction on the later parallel split: both runtime and history
correction steering have correlated worker-issued durable acknowledgement intents.
The earlier parent check missed the history receipt while inspecting only message/
event projections. Its acknowledgement existed before the report checkpoint;
the alleged ordering gap is withdrawn. The original receipt is retained in the
new split run's evidence. A terminal submission alone remains insufficient.
Launch, delivery, application and completion are separate evidence claims.

The remaining limits are explicit: 364 differing non-merges and all 311 differing
merges need whole-commit semantic closure; 24 combined merge diffs and additional
first-parent result changes require review; older exclusions/shared MCP seams,
provisional groups and regression followups need source verification. No local
build/test/binary/sanitizer/install/benchmark/hang harness or Jenkins trigger ran.
No push, PR, main merge, tag, fetch or publication occurred. Only doc consistency,
exact Git/source reads, evidence hashing and `git diff --check` are permitted here.
The report and ledger can be committed while the full audit honestly remains open.

## Later parallel split of unfinished work

After the snapshot above, the user authorized splitting only unfinished work and
Luna low for straightforward comparisons. Runtime R4 released exactly 16 still
unreviewed adapter/test followup hashes, outside both its reviewed rows and active
batch, and issued a correlated durable acknowledgement. New interactive external
run `CBM-RECON-ADAPTER-LOW-20261003`, worker `luna-adapter-low`, is observed working
in Herdr `wC:pE` with Luna low. It has a separately indexed clean exact-QA worktree
and TypeSafe credentials sourced silently in the actual launch shell. No finished
comparison was reassigned. Runtime retains 154 hashes; the effective ownership
crosswalk still accounts for all 367 differing non-merges once. Exact source and
hash comparisons use Git/CBM; bounded semantic comparison may use proper Jev
requests, with complete primary payloads/distributions and decision-use notes.
This launch changes ownership, not verification/acceptance or the frozen inventory.
The original checkpoint tables remain dated snapshots rather than live counts.

## Appendix: non-unresolved auditor checkpoint claims

This is a source-finding snapshot, not another task ledger. It makes the collected
work readable without relying on private run paths. Verdicts are the workers' CLI
target claims at the capture above; they remain provisional. Parent corrections
in the source section and canonical disposition column take precedence. Reasons
below are worker findings, not independent acceptance. Full source/diff/test
evidence and unresolved hunks are retained with each raw record in run evidence.

| Full upstream hash | R4 raw target claim | Recorded source reason / remaining scope |
|---|---|---|
| `c537bf0f8b3b6a77e39df49fec78560b5e8e87b7` | missing | cbm_spawn_backoff in src/foundation/subprocess.c calls cbm_nanosleep(&delay,NULL) in production/original/current QA; upstream uses cbm_nanosleep_full, which loops on EINTR via nanosleep remaining in src/foundation/compat.c:20-31. Full production/QA defect and tests are in the exact diff. |
| `1e0aac527b016d519eb7f8fc7dd610dbeb66e988` | missing | The changed two help lines advertise install --clients; absent from production src/main.c install usage. Existing install-hooks --clients help is a distinct command. Frozen upstream src/main.c:1108-1109 has the added install help. |
| `3f831936b5f3012d13175e7c97326165efecefad` | missing | The source operation shifts an int by token indices that can exceed int width; upstream widens the left operand to unsigned long long. production/QA have not picked up the fix. |
| `a2ea711c29560511eb8467bea7ad309ebf500d6e` | missing | The BANG suppressor shifts an unsigned long whose width is 32 bits on LLP64; upstream uses a 64-bit literal. |
| `e8204092333ee1c38cd681a81abb36f125288894` | excluded | This commit adds only vendored-source manifest/checksum provenance; it does not change CLI runtime behavior. Keep it as a dependency/follow-up when carrying the adjacent Swift scanner fixes, but exclude it from semantic-behavior missing counts. |
| `25a7aacc87250bff752ff61cd33e9236be5d1310` | represented | Parser failure now returns an error instead of silently executing a parsed prefix and returning misleading rows. |
| `44caa4c386c578e59c5919735e1e4d6094ac1af7` | represented | `.razor` files are admitted to discovery and C# extraction. |
| `fd73c347fbcfd7f66c210577b4d713fc6ce7780b` | partially represented | Source behavior is present, but this commit’s route-specific positive/negative regression hunks are not in production/QA. |
| `592894a4387a50e1d128ff6a2695ff48f8cc32a5` | missing | Upstream selects the terminal route handler when middleware precedes it; production/QA can bind middleware or stop before the handler. |
| `349aecba9528427fa635975e246f5b8959b14f30` | partially represented | The source guard is shape-based rather than testing dot presence in reference text. Full test-hunk parity remains unverified, so whole-commit disposition is partial. |
| `e0785b353af1af3812d5d14cad55c839efc5749d` | missing | The commit adds GET /api/ui-config version response (src/ui/http_server.c) and App fetch/display. Production/original/current QA handler returns lang and upstream_issues_url only; corresponding App version fetch/title are absent. Frozen upstream has response at src/ui/http_server.c:155 and display graph-ui/src/App.tsx:95. |
| `40f2722d6166c1f97fa1667c1e63e8c97e6f05f0` | missing | Growable traversal avoids fixed-cap truncation; this commit specifically moves temporary stacks into per-file scratch so they do not live as long as cached extraction results. |
| `95689b5c20101db7d92b69767bbea75e5bf0d4fc` | missing | Avoids fabricating a project CALLS edge from a module function when the bare callee is shadowed by an enclosing Python parameter. |
| `8bcfa2915fa02721c5b6d2a6c7082a26fef79d88` | excluded | No semantic behavior: the exact commit only clang-formats existing test rows. The CLI test formatting has no functional parity impact; target feature tests are absent with their parent feature. |
| `0b0d143c2d733e3313eef5b9d9745b0fb08cc2c0` | superseded | The intermediate bounded cursor walk was replaced upstream by O(1) walk-carried binding lookup in commit 97517a4652f376cb6bf096255e235edf456f86b1. Target lacks the final suppression behavior and test. |
| `97517a4652f376cb6bf096255e235edf456f86b1` | missing | The final O(1) walk-carried Python parameter binding helper is upstream at internal/cbm/extract_calls.c:3234-3238 and used by the invocation path; production and both QA refs lack the helper, call site and regression. |
| `c4e902849f8062e37a0f0ddaed1c6cc042780cb3` | missing | Swift call arguments are reached through call_suffix/value_arguments and labeled values are unwrapped; upstream helper/call site at internal/cbm/extract_calls.c:3317,3940 feeds URL/service route extraction. Target source and added extraction/pipeline regressions are absent. |
| `9baa0a8dc5cbe7a9fa687e0a8811dfcbdcd0551e` | missing | README change instructs team repos to use LFS and replace generated merge=ours attributes; target README at production/original/current QA has no corresponding LFS guidance. Upstream follow-up 957b27fca50d2c3b1ac86d771733f39612bae841 materially refines the advice. |
| `957b27fca50d2c3b1ac86d771733f39612bae841` | superseded | The commit corrects the prior LFS guidance: retain nested .codebase-memory/.gitattributes; track only .zst from root attributes; explain cadence, rewrite requirement, LFS costs/install. This full current guidance is in upstream README.md:253-259 and absent from all CLI target snapshots. |
| `57ef0a6d8662e9854dc5711922f801c3da381071` | missing | trace fix adds end-to-end integer parsing, CBM_DURATION_UNKNOWN and checked-duration propagation into HTTP span records. Production/original/current QA have only cbm_parse_duration and assign its 0 on unreadable start; frozen upstream contains checked parser and unknown sentinel. Full diff includes API header, code, regression tests. |
| `2c76563a137876b110cec15b9b6fa08d35069f73` | represented | The type-root receiver-chain guard gates both unique-name and suffix-match resolution; receiver-chain rejection regression is present in all target snapshots. |
| `1770d68a2c934501b889e625dc94ef933e3a1815` | represented | `.cshtml` is admitted as C# and `@page` route extraction handles `.cshtml`; all target refs contain positive route and no-page/layout coverage under adapted test names. |
| `48cb94f65ce221e1494119d850d35019c86a59e2` | missing | Upstream strips only recognized JS/TS module extensions and preserves dotted extensionless basenames; production and both QA refs still call `strip_ext`, truncating at the last dot, and lack import regression. |
| `73f4a432e4ac1d575635a490d96e0f5ea588344d` | missing | Windows guard changes add per-section CBM_RUNTIME_DIR isolation, SetupFailure classification, complete stdout/stderr, timeout preservation, and independent failures. Target tests/windows/test_daemon_stability.py has none of these revised paths; its cache-only isolation and head-slice diagnostics remain. Adjacent Windows tests likewise lack the diff additions. This is test/CI coverage, not a production daemon implementation change. |
| `be66a2d2c6f1538a4a831eb2c91622997be5b5ec` | missing | Pi smoke probe now provides TypeBox stub and rejects extension imports other than Node builtins plus Pi host package typebox. Target scripts/smoke-test.sh has no PI_HOST_PACKAGES/typebox import allowlist or stub; the exact smoke-test hunk is absent. |
| `63b99976a21463697d6e980e32aa42f5913319da` | represented | RETURN * deduplicates repeated pattern variables and reads live WITH aliases; both source branches and regressions are present in all target refs. |
| `426e415f5427f2fb2e9c6f918cc7cc9abd9c109b` | represented | WITH rejects a projection wider than CYP_MAX_VARS before silent binding truncation; the guard and boundary regression are present in all target refs. |
| `f404dfbc7ae47f9912b18a0554a5e5d91f65be1a` | missing | MSan image hunk installs bounded retry shell wrapper and uses it for apt/network steps and clean-retry clones. Production/original/current QA Dockerfile still directly runs apt-get and git clone, pins LLVM 21.1.8; frozen upstream has retry and LLVM 22. This commit changes image build resilience, not indexed product runtime. |
| `9104feb67f78959e64ddff78d236e0dea475a434` | represented | Full diff adds the shared daemon cohort conflict retry, preserving immediate failure for an unbounded deadline, emitting the conflict kind once, plus atomic test seam and two regressions. Production and both QA targets contain this exact behavioral contract and corresponding test cases; upstream also contains it. Representation is source-level only; no tests or runtime acceptance were run. |
| `4b1cdf57804afffbcb61e69fd484b42858fcc02f` | missing | Ruby base collection now handles constant and scope_resolution children; upstream includes Ruby inheritance regressions. Production and both QA refs lack both. |
| `7c34adf8bb60f873bd525e3d45ab78a395874582` | excluded | The commit has one parent and an empty tree diff (zero paths/hunks). It is an event to rerun CI, with no source/config/test artifact change to port. |
| `cd3ea5b35bfad379f3d76b5cc83f78826b57555c` | excluded | The commit has one parent and an empty tree diff (zero paths/hunks). It is an event to rerun CI, with no source/config/test artifact change to port. |
| `dc0f8ae642bf39b4b11bec2a88a8cdc7a1e4ab5b` | missing | Elixir call nodes expose arguments positionally, not via the generic arguments field. Upstream fallback at internal/cbm/extract_calls.c:545,3929-3931 passes their string literals to route/HTTP/config extraction. Production and both QA refs have no fallback/call site; existing elixir_call_arguments is used only for definition-role analysis. |
| `7e47043acce9bdfd87e7becc7fe7a348b02753ec` | excluded | The change adds a monotonic waiter-enqueue counter and uses it to make the absolute-deadline fixture deterministic; it does not change lock acquisition decisions or public lock behavior. Production/original/current QA lack this internal test seam and updated fixture. Treat as test reliability follow-up, not a missing CLI runtime feature. The atomic counter does add instrumentation to the lock path. |
| `ffc29f73bb27dfbad778acbc48e1a3a54f17d1b5` | missing | For positive committed_nodes with persisted_nodes==0 and ratio>0, the change reports degraded even below min_floor. Production/original/current QA return false at the min_floor short-circuit and miss total persistence loss in sparse projects. Frozen upstream adds that explicit zero-persisted check and regression test. |
| `6b0c44afcfeae65eed82e2dbbd58f00b043f21f6` | missing | The exact package-lock hunk updates graph-ui fflate 0.8.2 to 0.8.3 and nested three-stdlib fflate 0.6.10 to 0.6.11 with new registry URLs and integrity hashes. Production/original/current QA lockfile retains 0.8.2 and 0.6.10; frozen upstream carries both updated lock entries. No package manifest source change is in this commit. |
| `132e8fc32309e843ccd97138c8f032e317333673` | excluded | The hunk changes only the Windows test-wave harness: descendant probe gets an independent 15-second budget/two attempts, preserves fail-closed outcomes, and reports blockers distinctly; contract tests pin those properties. No shipped CLI/product source changes. The target harness/test contract lacks the change, so retain as test-infrastructure reliability evidence outside runtime parity. |
| `7d20ff9398cb494a61f6cd259a51b309dbe5bd54` | missing | Full diff replaces shell-script command registration with direct exec-form registration for Claude PreToolUse/PostToolUse/session/subagent hooks; exact command+args identity is used for owned-hook update/removal, and installed binary path is passed across install/uninstall. Production and both QA targets retain shell command resolution through cbm_resolve_hook_command(CMM_HOOK_GATE_SCRIPT); they lack the new binary/args path and exact args ownership. Upstream contains it. Windows integration test also changes to invoke the binary with forward-slash argv and direct stdin payload. The existing shell-form hook feature is related but does not provide this shell-free contract. |
| `39c74bdd5c8b5dc3bcd43f962195add0ba794150` | missing | Complete diff changes tests/test_cli.c and scripts/smoke-test.sh only. It updates Windows lifecycle assertions to count five exec-form hooks, verify prior owned shell hooks are removed while foreign hooks remain, checks uninstallation removes exec-form entries, and makes smoke assert installed binary plus hook-augment args. These are applicable CLI regression checks for 7d20, not new shipped behavior. They are absent in production and both QA target test sources; upstream contains them. No production behavior is inferred from test text. |
| `fece72b049db1954eb38f2fce139cec5eb499b00` | missing | ELF hardening now explicitly links `-Wl,-z,relro -Wl,-z,now`, and binary-composition CI asserts PT_GNU_RELRO coverage of every GOT section and eager binding where dynamic. Production/original/current QA lack those flags and A1c/A1d artifact checks. The commit gives a measured static Linux GOT gap and targets the shipped ELF artifact; this is security/build behavior, not a generic linker preference. |
| `7b92d2a6218936767b112081b9aa4e79d5b81dd9` | partially represented | Mixed commit: the shared scripts/ci/check-binary-composition.sh guard and tests/test_release_candidate_derivation_contract.sh fixture declaration are CLI release-qualification relevant and missing in production/QA; old script fails A1d for lazily linked fixture because it applies the release-link property to all ELF inputs. The tests/test_mcpb_bundle_contract.sh fixture change is MCPB-only and the file is absent from CLI targets; exclude only that exact hunk. Upstream retains every change. This is not a runtime behavior claim. |
| `1a6f80c37baed2d4e670304f5b95e766289aeaa4` | represented | Production/original/current QA already treat refused UI config as nonfatal for bare `daemon start` (warning with daemon left running), while `--port`/`--open` still fail; the refused-config test seam and Windows lifecycle assertions are present. Exact behavior in frozen upstream is present too. This source behavior arrived via another path even though this exact commit hash is not target ancestry. |
| `3909e5a0e1d55236fccac9ab680a1c6103ce5daf` | partially represented | Mixed commit. Missing shared CLI/core hunks: Cypher result truncation and generic plain-BFS saturation/edge-error behavior; CLI process logging/verbose behavior is also absent from target. `cbm_store_bfs_with_edge_limit` is only called from MCP. MCP compact response, MCP pagination/format and graph-UI/hook adapter hunks are excluded from CLI semantic missing counts. |
| `17d61ea9be09b004a3e64f330c82c2f716a10a4e` | excluded | The full diff changes only tests/test_incremental.c calls to MCP `detect_changes` and their response assertions (empty changed_files omission, changed_total/changed_returned scalars); no operation or CLI source changed. The test uses `call_tool_timed` against the MCP adapter. Direct adapter response-shape assertions do not establish neutral CLI behavior and cannot be ported unchanged. |
| `4cbc0536ac8d672482068375340382cc78a033e1` | represented | Pagination generation validation and generation advance at the staged-publish tail are present in production and both QA refs, including canonical UID/counter validation. |
| `f4c7d2010d8f77bc4dd1e5ae62e2e5aa84e44c42` | excluded | The two call sites switch to main_daemon_endpoint_new so the opt-in test runtime-parent seam isolates one-shot daemon coordination. Production/original/current QA already contain that helper and test seam, but those two callers still pass NULL directly. Without CBM_ENABLE_TEST_SEAMS the calls resolve to the same bootstrap endpoint, so shipped product behavior is unchanged. This is a Windows/CLI test-isolation follow-up, not a runtime parity gap. |
| `619c6834807532b69c419454fd53074d6d10050a` | missing | The variable-length traversal probes beyond the engine depth cap and marks the result truncated if a candidate exists beyond it. Upstream has the probe in src/cypher/cypher.c; target lacks it and the truncated carrier. |
| `c25c16206be0d2abf9faf8a7069ef72dc554116d` | superseded | The commit changes format-migration logging from INFO to WARN, but assigned commit b795203025596562fcfcce08c052ce782b411464 changes it back to INFO. Frozen upstream and target all show INFO at pipeline.route/format_change_reindex. |
| `b795203025596562fcfcce08c052ce782b411464` | partially represented | Library default INFO is present in production and both QA refs. The final upstream also installs role-aware process policy via cbm_log_init_for_process; that startup behavior is absent from production and both QA. |
| `0365ef94173f22677e31de98ea62dc0d00415669` | missing | Upstream distinguishes symlink/reparse-point skips and records ignored-file reason `symlink`; production and both QA silently return from safe_stat failures. Added regression exists upstream only. |
| `24f54c0eb64c516941abee2ad0d7de758b34ac86` | represented | Markup embedded-script attribute selection and full definitions/imports/calls extraction are present in production and both QA, including Svelte/HTML/Astro host behavior and Astro TypeScript defaults. Regression coverage is present. |
| `6db22aadb6e5a92b9b193608e2ac3f420e461ec3` | represented | VB6 marker detection, .frm/.cls disambiguation to unsupported, and regressions are present in production and both QA. |
| `a953509275a4fa42a907f88852d62a613f27bc56` | partially represented | Core error-range cap/dropped-marker and parse_unusable behavior (including ObjectScript range aggregation and parallel/serial phase storage) are absent from production and both QA. MCP-only coverage wording/serialization is excluded from CLI semantic counts. |
| `21591d51168d26f931f64d62edc36013cd1f6249` | partially represented | The decisive arithmetic differs: production and both QA targets return aggregate_memory_budget_bytes / physical_job_limit despite computing and logging active job count. Frozen upstream returns aggregate_memory_budget_bytes / active, where active comes from nonterminal jobs and has a floor of one. Thus symbol presence, active_jobs logging, and the shared helper do not establish parity. The upstream merge plan explicitly lists this hash as divide by active jobs rather than configured capacity and assigns daemon_application,daemon suites. The commit also adds a sequenced two-job regression for aggregate then half-aggregate slices and a test that a clean worker over-budget error passes through without a recovery retry; neither test name is in production/QA. Do not infer the separately assigned Windows job-memory-resolution change from this hash. |
