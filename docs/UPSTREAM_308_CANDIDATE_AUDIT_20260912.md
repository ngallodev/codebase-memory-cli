# Upstream merge progress — canonical ledger

This is the single canonical document for upstream merge work in this
repository. The filename is retained for stable links; the old 308-commit
snapshot and the separate dated plan documents are superseded by this ledger.

## Consolidation checkpoint — 2026-10-02

**Current conclusion: ancestry accounting is complete; semantic reconciliation is
partial.** The [comprehensive evidence report](UPSTREAM_RECONCILIATION_REPORT_20261002.md)
collects the work without becoming a second backlog. The full-hash appendix below
accounts for all 367 differing non-merges, all 311 differing merges, and all 87
older selected/deferred/excluded rows. Machine crosswalks include all 3,352 reachable
upstream commits and exact merge constituents in durable run evidence.

The R3 interactive auditors finalized **partial**: extraction touched 10/111,
runtime 20/170, history 4/86, and no whole differing merge was closed. Touching a
row did not prove all hunks. Parent verified exact assignment coverage and all 40
declared artifact checksums, preserved the raw evidence, and rejected ambiguous
upstream-versus-target verdict orientation. R4 interactive successors in the same
three Herdr panes continue those assignments. At the evidence capture
`2026-10-02T23:53:00.456924+00:00`, extraction had 30 non-unresolved checkpoint claims,
runtime 5 and history 19; 313 other claims remained explicitly unresolved.
These 54 claims are not 54 parent-verified whole commits. No R4 final result,
review, current Jenkins success, acceptance or production integration is asserted.
The parent retains 364 whole non-merge semantic remainders and all 311 merge
remainders, including bounded missing/represented rows. Three exact empty-tree
commits require no source port.

Source correction: `21591d51168d26f931f64d62edc36013cd1f6249` is **not equivalent**
in the CLI targets. `application_worker_memory_slice_locked` divides by
`physical_job_limit` in production and both QA snapshots, versus `active` upstream.
The existing fork test/comments favor fixed per-worker slices. Upstream's spawn-time
active division does not, by itself, prove aggregate reservation safety when workers
overlap. Keep this policy/adaptation question open. The Windows job-memory hunks in
`2b8bd0c065c1d2728994fddc1683a66f70745748` are a separate missing shared capability;
MCP headroom/logging hunks require neutral adaptation rather than direct import.

Additional parent-checked bounded gaps are malformed confidence parsing
`62b44519c68b403ea44d9a85ba8ee74cddee66e6`, route handler position
`592894a4387a50e1d128ff6a2695ff48f8cc32a5`, late annotation route arguments
`c36b4fbc446f9085704a6073b81ec743fd4180fc`, and client-registry selection
`1e25d962d69999438229b0562d44ea60e97f5456`. Full test/dependency review remains open.
The two route fixes are related followups to slice 5, not prerequisites for importing
an omnibus memory change. Swift provenance `e8204092333ee1c38cd681a81abb36f125288894`
and applicable test/harness improvements stay in their vertical even when they do not
change runtime behavior. An R4 runtime-only exclusion is not an exclusion from work.

The five source-grounded recommendations below remain provisional among inspected
candidates, with confidence parsing the first additional localized candidate.
Current means the **captured** QA `559214af`, not a refreshed live branch.
The historical sections below retain provenance; this checkpoint and the full-hash
appendix supersede their launch/current-status claims. Workers only write evidence;
canonical task updates remain parent-owned.

### Parallel split followup — 2026-10-03 UTC / 2026-10-02 local

The user authorized splitting only unfinished work and Luna low for straightforward
comparisons. Runtime R4 explicitly released 16 unreviewed MCP-adapter/test followup
hashes, with no reviewed/current-batch overlap and a correlated worker-issued
Agent-Workflow acknowledgement. New external interactive run
`CBM-RECON-ADAPTER-LOW-20261003` is observed working in Herdr `wC:pE` on an isolated
clean `559214af` worktree, full nonpersistent index generation `2026-10-03T00:48:33Z`,
26,764 nodes / 129,702 edges, 91 parse-partial files. TypeSafe environment readiness
was observed after sourcing in its launch shell, without exposing values.
The run evidence owns the exact 16-hash release and 367-row effective ownership
crosswalk; extraction/history retain their assignments, runtime retains 154 hashes.
No completed work was reassigned, and source claims remain provisional until review.
Use exact Git evidence plus CBM discovery/coverage, and bounded primary-evidence
Jev comparisons for semantic applicability/parity choices; not for exact lookups.

Acknowledgement correction: the prior report's history-worker ordering-gap claim
was based on inspecting message/event projections without its saved control-intent
receipt. A correlated worker acknowledgement already existed before the report
checkpoint. That alleged ordering gap is withdrawn; the exact receipt is preserved
in new run evidence, with all sealed predecessor artifacts unchanged.

### Phase1 plan: three disjoint DeepSeek Flash workers — 2026-10-03 UTC

The user chose runtime Luna's **remaining unreviewed commits**, explicitly requesting
three DeepSeek Flash agents and Jev decision assistance. Source runtime R4 released
106 unfinished hashes with a correlated worker-issued acknowledgement; nine more
were already released to an unstarted second low run. That second low preparation
was retired, its prompt/index/evidence preserved. First-low's 16 source-reviewed
hashes and all completed runtime/extraction/history comparisons are excluded.
The first low worker's corrected 16-row source review finalized **partial** with
8 unresolved shared-semantic mappings and an unrecoverable original Jev request;
its manifest/reference checks passed, full handoff was preserved, and its user-created
Herdr pane `wC:pE` was closed. Closure is not acceptance or semantic completeness.

**Phase1 objective:** inspect every whole diff/hunk in the 115-hash released scope,
compare exact upstream and all three frozen CLI targets, record upstream fate,
source-backed applicability/representation/grouping and every full-hash remainder.
No code implementation, local builds/tests, Jenkins triggering, review, acceptance
or production integration is included. Phase1 cannot be called complete while a
required source comparison or evidence path remains unreviewed.

| Disjoint piece | Durable run / native task | Hashes | Evidence focus |
|---|---|---:|---|
| Runtime/process/resources | `CBM-RECON-DEEPSEEK-RUNTIME-PHASE1-20261003` / `deepseek_runtime_phase1` | 39 | Daemon, foundation, watcher, resource/locking/cancellation semantics |
| Operations/pipeline/query | `CBM-RECON-DEEPSEEK-OPS-PHASE1-20261003` / `deepseek_ops_phase1` | 26 | Neutral operations, graph/store/pipeline/query behavior |
| CLI/build/adapter/integration | `CBM-RECON-DEEPSEEK-INTEGRATION-PHASE1-20261003` / `deepseek_integration_phase1` | 50 | CLI/config/build/output seams and precise adapter/test exclusions |

Exact assignments are `evidence/assignment.jsonl` in each run. Their union is
exactly 115 unique full hashes, intersections empty, and each existing prerequisite/
followup primary group stays intact. Provisional broad containers still require
source-backed splitting by behavior within their owner; grouping is not equivalence.
The phase plan and effective 367-hash ownership crosswalk remain run evidence,
not another canonical backlog. Worker source scopes are exclusive; non-owned
prerequisites may be consulted only as context. Parent alone reconciles conclusions.

All three were prepared before native `agents.spawn_agent(model=deepseek-flash)`
execution and bound to external runtime `native-agent-api`. They are workspace
subagents, **not Herdr terminal processes**. A task-scoped fail-closed local adapter
only satisfies Agent-Workflow executable validation; it is not executed and does
not emulate a DeepSeek CLI. Shared runtime configuration was unchanged. Each has
an isolated clean exact `559214af` worktree and a verified full nonpersistent index:
runtime reuses the unchanged retired-low worktree generation `2026-10-03T01:07:59Z`;
ops generation `2026-10-03T01:27:07Z`; integration `2026-10-03T01:27:06Z`.
Each ready root has 26,764 nodes, 91 partial files and explicit source fallback
for vendored/MCP gaps; full current receipt/pagination/porcelain evidence is retained.

Execution sequence: each worker writes launch acknowledgement, reads its exact
assignment, reviews dependency batches of 5–10 with CBM discovery/snippets/traces/
coverage plus Git whole-diff/frozen-source reads, then persists dispositions,
working evidence links and checksum manifest. Genuine semantic alternatives use
proper Jev helper calls with verbatim primary evidence, saved actual requests,
full distributions/model and advice-use notes; authorized TypeSafe env is sourced
silently in the helper-launch environment and destination verified. Counts, exact
lookups/text equality and lifecycle gates stay deterministic.

At every stopping point each worker must save `checkpoint.json` and `HANDOFF.md`
with all exact done/remaining/partial hashes, target/hunk evidence, graph generation
and pagination/fallback state, Jev request/result gaps, ownership/dependencies,
commands actually executed, next commands and real stopping reason. Successors
continue from the first unreviewed item under new prepared authority. Worker finish,
external exit, independent review, Jenkins validation and acceptance stay separate.
**Delivery correction — 2026-10-03 UTC:** native spawn and external binding did
not establish execution of these assignments. All three tasks returned inherited
Sol/consolidation reports; operations explicitly reported no task payload arrived.
Persisted correction steers and direct follow-ups still produced predecessor
reports, with zero correlated Phase1 acknowledgements or dispositions observed.
The affected sessions were stopped. All **115 assigned hashes remain pending**;
no assigned source comparison, actual model identity or Jev execution is established.
Runtime disclosed a rejected predecessor re-finalization attempt that incremented
its telemetry; integration committed unrelated consolidation `76244f78` and notebook
`ea68cb4`. Preserve these artifacts separately; neither is Phase1 task progress.

The prepared plan, exact assignments and isolated index receipts remain reusable.
Each unsealed Phase1 run contains a parent-written
`parent-delivery-recovery-checkpoint.json`, `PARENT_DELIVERY_RECOVERY_HANDOFF.md`
and their checksum manifest, retaining every full remaining hash. Detailed incident
evidence is `native-delivery-recovery.json` in the Phase1 handoff directory.
Host recovery must establish functioning task delivery/binding and correct worker
run/cwd/branch/hash acknowledgement before comparison resumes. No predecessor
completion is reissued, and no sealed predecessor evidence is rewritten by recovery.
This is a preparation/failed-delivery record; Phase1 is incomplete.

### Interactive Herdr recovery — 2026-10-03 UTC

The user explicitly requested interactive **Codex `--model deepseek-flash`**
workers, with Jev credentials/instructions and indexed worktrees. Three new
external runs were prepared before launching in the same Herdr workspace:

| Piece | Successor durable run | Herdr pane / agent | Hashes |
|---|---|---|---:|
| Runtime/process/resources | `CBM-RECON-DEEPSEEK-RUNTIME-HERDR-PHASE1-20261003` | `wC:pF` / `ds-runtime-phase1` | 39 |
| Operations/pipeline/query | `CBM-RECON-DEEPSEEK-OPS-HERDR-PHASE1-20261003` | `wC:pG` / `ds-ops-phase1` | 26 |
| CLI/build/adapter/integration | `CBM-RECON-DEEPSEEK-INTEGRATION-HERDR-PHASE1-20261003` | `wC:pH` / `ds-integration-phase1` | 50 |

All three are observed working and have written correct worker-issued launch
acknowledgements naming their run, isolated cwd/branch and assigned count. The
exact assignment sets match the original plan, are disjoint, and all 115 full
hashes resolve as commits. Completed predecessor comparisons remain excluded.
The original 367-hash effective ownership crosswalk is refreshed in launch
evidence, preserving one owner per hash; it is not another canonical ledger.

Their unchanged clean QA `559214af` worktrees retain the prior full nonpersistent
indexes. Fresh status/root and coverage receipts verify ready graphs and matching
generations. Each prepared prompt includes the official Jev footer once and all
source/graph/no-test constraints. The authorized TypeSafe env was sourced directly
into each pane shell before restarting Codex; actual Codex-process inheritance of
the token was verified as a boolean, without recording its value. The verified API
destination is `https://api.typesafe.ai`. Each session uses `--no-daemon`, approvals
never and full filesystem access within its evidence-only assignment.

Run-owned evidence includes `parent-herdr-launch-receipt.json`,
`codex-process-env-receipt.json`, `launch-acknowledgement.json`, index/assignment
receipts and correlated worker acknowledgement evidence. Runtime/operations
acknowledgements are worker-issued handoff control intents; integration also has
a journal acknowledgement. Full launch staging is the original Phase1 handoff
directory's `herdr-launch/`. Detailed batch/stopping handoffs remain required.
Activity and launch acknowledgement establish execution of the assigned task;
they establish no source completeness, Jev decision result, review, Jenkins
validation, acceptance or integration. Phase1 comparison remains in progress.

## Frozen merge and source audit — 2026-10-02, Sol R2

This section supersedes historical queue, exclusion, fetch and completion claims
below. **We have not processed ALL upstream merge commits semantically.** This is
a complete ancestry inventory and a bounded source audit, with a five-slice
recommendation. It is not exhaustive semantic verification, acceptance or a
recommendation to merge upstream wholesale.

### Frozen boundaries and current QA projection

| Boundary | Exact revision |
|---|---|
| Integrated upstream scope | `96c3f41cf334d87670cb085f1fcf16f637293222` |
| Production origin/main | `bbabc07386f347ac416a60fe530b1007ced5c27b` |
| Audit QA base | `5fe820df8bfeec059fa646e0050c1652f38a4165` |
| Common ancestor | `17786374dfc39b8c751933d34d269501f2b4d6b7` |
| Later parent-reported QA, separately compared | `559214af28ce2d6f5d6ec86662f6871316d28b45` |

Frozen QA is 13 commits ahead of production. `src/`, `internal/` and
`Makefile.cbm` are byte-identical between those two frozen trees; the difference
is release/package/CI tooling, specifications and documentation. Thus the shared
source findings below apply to production too, while QA release behavior cannot
be attributed to production. PR #14's remote `1ce4e164` failures are historical
parent evidence; this run did not refresh GitHub or repair that PR.

Acknowledged parent steering reports the three daemon/cache/ReScript candidates,
ADR alias/outline and pre-commit Git-env candidate ancestry on later QA
`559214af`. Source-tree comparison confirms the three port and ADR changes;
`scripts/hooks/pre-commit` has **no tree delta** from frozen QA, consistent with
retaining formatting-only behavior. The hook candidate's ancestry does not prove
production subprocess Git-env isolation (`40dad610`) or its original test-hook
behavior is present. Parent is retrying the full shared Jenkins gate; no result,
acceptance, main merge, push or publication is established here. These QA-integrated
candidates are excluded from the next five NEW slices. Four recommended source
paths are unchanged in later QA; `discover.c` only adds `.res` disambiguation.
Swift manifest/checksum adaptation must preserve QA's new ReScript patch row.

### GitHub ancestry and the requested Luna reconciliation

GitHub ahead/behind is computed from shared commit ancestry, not patch IDs or
source-equivalent adaptations. Commit IDs cannot be edited while retaining the
same commits: changed objects rewrite history. Cherry-picking or adapting a fix
usually creates a new ID and leaves the upstream original outside fork ancestry.
A real upstream-history merge would record that ancestry; it also requires an
honest review of all incoming and resolved behavior. A blanket `ours` merge would
mark unresolved upstream history incorporated without incorporating its missing
behavior. No merge, rewrite, push or GitHub update was performed here.

The user requested a GPT-6-Luna team for the remaining comparison. Three durable
headless exploration runs launched at exact QA `559214af` after independent full
indexes, matching generations, ready roots and unchanged porcelain:

| Run | Exclusive differing non-merges | Differing merges |
|---|---:|---:|
| `CBM-RECON-EXTRACT-20261002` | 111 | 0 |
| `CBM-RECON-RUNTIME-20261002` | 170 | 0 |
| `CBM-RECON-HISTORY-20261002` | 86 | 311 |

Every differing non-merge is assigned once. The history worker also investigates
merge resolution behavior and safe ancestry options. Launch is observed through
initial executor events; source conclusions, completion, independent review and
acceptance remain pending. This section is an interim audit record, not a claim
that the team has established parity. Assignment/launch receipts are in Sol R2
run evidence under `luna-team/`; worker results belong in each child run's evidence.

**Interactive recovery correction — 2026-10-02T21:54Z.** The headless runs above
were found orphaned: no live worker and no finalized result; their last activity
was near 18:16Z. The lifecycle projection still saying `running` did not establish
live execution. Their provisional outputs remain preserved and unreviewed.
Unstarted R2 headless retries were superseded when the user requested interactive
Herdr terminals to the right. New external preparations are now visibly working:

| Current interactive run | Herdr pane | Assignment |
|---|---|---|
| `CBM-RECON-EXTRACT-20261002-R3` | `wC:pB` | 111 non-merges |
| `CBM-RECON-RUNTIME-20261002-R3` | `wC:pC` | 170 non-merges |
| `CBM-RECON-HISTORY-20261002-R3` | `wC:pD` | 86 non-merges and 311 merges |

Each uses GPT-6-Luna high, full access and approvals `never`, bound to its durable
external run. The unchanged worktrees still have exact ready graph roots, matching
generations and clean porcelain; the extraction worker confirms CLI access now
works interactively. No cache-root/daemon changes were made. Replacement evidence
records predecessor links explicitly; these are fresh external preparations,
not modifications of sealed headless contracts. The earlier worker cache-resolution
failure and the cause of process loss remain distinct observations; causality is
not established. Completion, source completeness, review and acceptance remain pending.

### Actual merge history and constituent accounting

| Inventory at frozen upstream | Count |
|---|---:|
| All reachable commits | 3,352 |
| Actual merge commits, identified by multiple parents | 923 |
| Non-merge commits | 2,429 |
| Ancestry-differing actual merges vs frozen QA/main | 311 |
| Ancestry-differing non-merges vs frozen QA/main | 367 |
| Upstream first-parent commits / actual merges | 1,412 / 724 |
| First-parent ancestry-differing merges | 202 |
| Patch-equivalent differing non-merges / other patch IDs | 3 / 364 |
| Differing merges with nonempty combined (`--cc`) diff | 24 |

Every actual merge has full hash, all parents, first-parent membership,
first-parent diff paths, nested merges and constituent non-merges in run evidence.
For a merge `M`, introduced ancestry is `rev-list M --not M^1`, excluding `M`
itself; nested merges are retained separately. This avoids substituting a list
of non-merge subjects for merge history. `--cc` is an additional resolution
signal; an empty combined diff is not semantic equivalence or acceptance.

All 311 differing merges retain an unresolved overall semantic disposition.
Twenty-four need explicit resolution-hunk review, including
`2b8bd0c065c1d2728994fddc1683a66f70745748`, whose merge itself changes Windows
worker memory-limit/headroom behavior. Neither its non-merge ancestry nor a
historical batch-port title closes that merge-level requirement. The other 612
merges and 2,062 non-merges are inherited by ancestry, not freshly runtime- or
source-revalidated by this audit.

The three patch-equivalent hashes are
`25a7aacc87250bff752ff61cd33e9236be5d1310`,
`349aecba9528427fa635975e246f5b8959b14f30`, and
`557cb0102863e66cbd5cc622509a29a4964d1831`. Parser token-consumption behavior
for the first was directly checked in QA; the other two remain semantically
unresolved here. Patch ID, ancestry and a source adaptation are distinct facts.

### Canonical reconciliation and grouping

The preserved primary snapshot has 123 backlog lines / 610 ledger lines, versus
104 / 438 at frozen QA. Its dirty edits were imported into this isolated proposal;
primary files, deletions and untracked state were not touched. Every one of the
old 87 selected/deferred/excluded rows (16/24/47) and all 190 distinct canonical
revision tokens is linked to a full Git object and a current disposition in run
evidence. This is row reconciliation, not blanket endorsement of historical
source claims. First/second-wave aggregate claims, legacy exclusions and
unreviewed followups remain open where exact semantics were not verified.

The 367 differing non-merges have one primary group each: 199 groups comprising
source-checked verticals, related source/QA dependency groups, and unresolved
native first-parent integration containers. Container grouping alone is
provisional: broad merges must still be split by behavior, and unrelated test
hunks must not be copied with a selected source fix. The selected route change,
for example, arrived inside the memory-work merge but can be adapted without
importing the memory core or unrelated spill/staging tests.

Source review corrects these old dispositions:

- ReScript checksum `ea6fa49f` is required with `8093d10b`, and JSX checksum
  `c5c13a87` is grouped with grammar `1b795589`; checksum-only is not a reason
  to discard a prerequisite/followup.
- SQL regression `259dbb67` belongs with `c0a06b3d`; watcher `07eb426c` and
  `5a9c72d8` are regression followups, not blanket test-only exclusions.
  The main backoff helper is present, but `4fc5999a`'s failure-count accessor
  is absent; the remaining QA followups are not declared fully represented.
- `fa229211` modifies shared pipeline per-run artifact-export failure state;
  its MCP title cannot justify excluding the whole commit. Direct MCP hunks
  remain out of scope; neutral CLI error propagation is unresolved.
- `4c466fc4` is partly represented: ordinary C prod objects follow
  `.build-config`, but grammar/TS/LSP/preprocessor/vendor prod recipes and
  `GRAMMAR_CFLAGS` still lack corresponding stamp/seam coherence. Its
  complexity-test temporary-directory cleanup is another scoped followup.
- Pure MCP adapter imports are excluded only at that seam; 37 such rows retain
  unverified neutral-operation parity. Mixed MCP/shared commits remain unresolved.
  Three commits have exactly their parent's tree and no change to port.

Directly checked represented behavior includes failed COUNT reads, linear LSP
array decode, TS awaited generic calls, optional COUNT, .NET XML admission,
properties payload state, macro coverage scanning, specified AST stack growth,
Cypher parser consumption and variable capacity, and the empty-function sort
guard. These 14 rows assert bounded source behavior only; they do not certify
all commit hunks, scheduler execution, allocation failure, independent review,
Jenkins results or acceptance. Historical Jenkins #34/#35 claims remain attached
to their historical revisions and are not validation of current QA.

### Next five NEW vertical slices

This ranking is among the source-reviewed candidates. The unresolved remainder
may contain higher-priority work. All five remain absent in frozen production,
frozen QA and the relevant later QA source; none is an implemented or accepted port.
Use the existing shared `codebase-memory-cli-release-tooling` Jenkins venue.
Suite selections below are existing `CBM_TEST_SUITES` values, not commands run
by this auditor. Parent schedules focused/full gates and records exact revisions.

**1. Cypher OPTIONAL relationship fallback growth — very small, memory safety.**

Source `2fabaa2b877986bdededb4f4f78e9a31014c6fdf`; actual first-parent merge
`ea7e01e91e64da62a74c1abbfe7a138710d1c88a` (PR #2445).
`src/cypher/cypher.c:4800`, `cross_join_with_rels`, allocates capacity one when
`extra_count == 0` then writes one fallback for every input binding at line 4832.
Reuse existing `binding_out_append` for that writer; its siblings already use it.
The shared call chain reaches `cbm_cypher_execute`, so this is CLI query behavior.
Earlier bounded trails, parser consumption and optional COUNT are prerequisites
already present in the inspected source, not substitute fixes for this overflow.
Jenkins `cypher`: the upstream missing-label relationship query over four Function
bindings must return exactly four distinct source rows with both optional variables
unbound, without ASan/UBSan findings; retain nonempty relationship and COUNT cases.

**2. SQLite interior-page packing — small, persisted index correctness.**

Source `e7324bb6904c6d0a295b956e83c8585219198358`; actual first-parent merge
`6bce86383ea192a2f0fbf795e161d3ddca4a78cc` (PR #2330).
`internal/cbm/sqlite_writer.c:654`, `fill_interior_page`, plus `pb_build_interior`
can consume the penultimate child as a page's right child, leaving the final child
alone on a zero-cell next page. Add the small last-cell give-back helper and guarded
rewind of `idx`; keep current page layout/ownership. Existing `PageRef`/cell builders
are sufficient. Atomic-publish errno `460ebefc` is a separate diagnostic slice,
not a prerequisite to this packing fix. Jenkins `sqlite_writer,graph_buffer,pipeline`:
use the long-key row sweep (2–120 rows, 12,000-byte keys), explicitly force
`idx_nodes_name` for a rightmost miss/delete, require SQLite success and integrity,
and retain multi-level, empty, overflow-key, round-trip and publication checks.

**3. Swift scanner shift widths and vendored integrity — very small, UB/platform safety.**

Dependency order:
`3f831936b5f3012d13175e7c97326165efecefad` ->
`a2ea711c29560511eb8467bea7ad309ebf500d6e` ->
`e8204092333ee1c38cd681a81abb36f125288894`.
Actual first-parent merge `620613ede6a983f04b7c1539a2ea4c65b34d42f6` (PR #1986).
`internal/cbm/vendored/grammars/swift/scanner.c:514` in `eat_operators` still uses
`1 << suppressor`; line 131 in `OP_SYMBOL_SUPPRESSOR` uses
`1UL << FAKE_TRY_BANG`. Use `1ULL` at both sites. Keep the regression, manifest
patch row and final scanner/manifest hashes together; regenerate from fork bytes,
preserving current QA's ReScript entry. No re-vendor or grammar parser import is
needed. Jenkins `language,extraction,grammar_regression` plus the existing vendored
integrity gate: force-unwrap/`try!` inputs preserve extraction and produce no UB.
The upstream test explicitly says ordinary recoverable UBSan can still pass; a
plain green test count is insufficient. Require a failing UBSan error/trap policy
at the parent venue; native Windows LLP64/ARM trap evidence remains a separate
qualification gap until an authorized venue actually supplies it.

**4. Enclosing-repository ignore chain — medium, discovery completeness and scope.**

Dependency order `c8184ed94887a6b5e130726885b44a07c63a47b7` ->
`282fbfcdabfec79c19c36f5bec5f75c22eb36df5`.
Actual first-parent merge `92910147aba1e56a1cb8fff7da5e74dbf8011b35` (PR #2181).
`src/discover/discover.c:1171`, `discover_impl`, only resolves a Git marker at the
indexed root. Reuse existing `gitignore_link_new`, `gitignore_chain_result` and
`walk_dir`; add enclosing-root discovery and a per-ancestor base offset, loading
intermediate `.gitignore` files and correctly anchoring common-dir `info/exclude`.
Do not stop at the first commit: flattening ancestor patterns into the child matcher
reanchors rooted patterns and silently loses or adds files. Preserve current `.res`
admission, nested-chain ownership, count-only/resource limits and cleanup on OOM.
Jenkins `discover,gitignore,git_context,pipeline`: anchored root exclusions must not
hide same-name child files, ancestor `pkg/scratch/` must exclude child scratch,
intermediate ignore files and deeper negation must match Git's verdict; relative/
absolute roots and linked worktrees must agree and preserve indexed-root behavior.

**5. Route determinism and complete proto service admission — small/medium, graph quality.**

Source `6b48097471e8d1a3e5cf16447304899a21e12d7a`; actual first-parent merge
`f1e51133e9ec0708756e0ca9311b6b0f3a8cca34` (PR #2233, broader memory-work merge).
`src/pipeline/pass_route_nodes.c:325`, `match_one_infra_route`, picks the first
insertion-order handler; `create_grpc_routes:839` considers only 64 services.
Choose a total QN order among matching handlers and allocate exactly the discovered
service count, using current heap/free conventions rather than importing the
upstream memory core. Release the borrowed-pointer array on every exit. No new
public API or graph schema is needed. Adapt only the route source and relevant
regressions; this commit's spill/staging test hunks are separate unresolved work.
Jenkins `pipeline,parallel`: >64 proto services each produce their route and HANDLES
edge; permuted insertion/worker orders produce identical selected-handler edge sets;
zero-service/no-function and allocation-failure paths remain safe. Retain existing
route and parallel resolution contracts. This does not port or validate the whole
memory merge.

Other directly confirmed candidates remain in evidence: TS imported-class receiver
resolution, C pointer/qualifier return types, Python external-import contradictions
with serial/parallel symmetry and `492ae135`, and atomic-publish error attribution.
TS await already present does not close imported-class resolution. Python's added
helpers need adaptation against missing bare-binding groundwork; do not drag in
an omnibus graph/memory commit solely to satisfy an incidental helper dependency.

Jev `jev-1.13.0` assessed seven bounded candidates from verbatim production diffs
and frozen QA source. Scores/distributions (P(defer), P(useful), P(high), P(urgent)):

| Candidate | Score / 3 | Full distribution |
|---|---:|---|
| Cypher buffer | 2.94 | 0.01, 0.00, 0.03, 0.96 |
| SQLite packing | 2.68 | 0.03, 0.02, 0.19, 0.76 |
| Route order/completeness | 1.98 | 0.01, 0.02, 0.95, 0.02 |
| Ancestor ignores | 1.80 | 0.06, 0.11, 0.81, 0.02 |
| C return fidelity | 1.78 | 0.02, 0.19, 0.79, 0.00 |
| TS imported classes | 1.72 | 0.07, 0.15, 0.78, 0.00 |
| Swift shifts | 1.42 | 0.10, 0.41, 0.45, 0.04 |

Advice supported Cypher/SQLite leading and route/ancestor applicability. Swift was
prioritized despite split advice because both unsafe literals and the token enum
are directly visible and the fix is exceptionally small. Scores are advisory,
not acceptance or probabilities of correctness; close decimal rankings are not
significant. No earlier agent verdict or Jev answer was supplied to this call.

### Exact remaining limits, evidence and validation

Run `CBM-UPSTREAM-SOL-20261002-R2`, under
`/home/nate/.local/state/agent-workflow/runs/CBM-UPSTREAM-SOL-20261002-R2/evidence/`,
owns `merge-crosswalk.jsonl`, `nonmerge-crosswalk.jsonl`,
`first-parent-history.tsv`, `canonical-reconciliation.jsonl`, `slice-grouping.json`,
`next-five-slices.json`, `unresolved-dispositions.tsv`, source/combined diffs,
`target-source-excerpts.md`, graph receipts and `current-qa-delta.json`.
These are evidence crosswalks, not a second canonical task ledger.

Of 367 differing non-merges, 288 have no target semantic review, 21 only have
adapter-import/path/hunk triage with neutral parity unresolved, 47 have bounded
source/dependency dispositions, eight are adapter-only rows closed with exact
hunk reasons, and three are verified empty tree changes.
Even a bounded represented behavior is not exhaustive whole-commit verification:
356 non-merge rows and all 311 merge rows retain explicit full-hash semantic
remainders in `unresolved-dispositions.tsv`. All 24 nonempty combined merge diffs
are retained for resolution review. The five-slice shortlist is therefore useful
implementation guidance, not proof the rest is lower value or processed.

Locally cached upstream-only branches are outside integrated-upstream scope:
69 refs have commits not reachable from the frozen tip, enumerated with full tips
and hashes in `upstream-branches-outside-scope.json`. Their open-PR status and
remote freshness were not checked; they are not silently added to this audit.

Auditor graph project `tmp-cbm-upstream-sol-20261002` was ready at generation
`2026-10-02T17:30:32Z`, with coverage generation matching and empty Git porcelain
through probes. The supplied full non-persistent index receipt is retained;
91 parse-partial files and ignored vendored scopes require source fallback.
Qualified searches were unpaginated; both-direction traces were saved, with the
127-callee `extract_func_def` trace rerun at limit 300 after the truncated limit-100
response lacked a cursor. Every material target source has direct excerpts and
path coverage; vendored Swift was read directly. Invalid JSON-array/comma-list
coverage probes were replaced with repeated-flag/single-path receipts. Coverage
is best-effort, never proof of source completeness.

No build, test binary, sanitizer, install, benchmark, hang harness, Jenkins trigger,
remote fetch/ref movement, push, PR, tag or publication occurred. Doc consistency
and `git diff --check` are the only delivery validation here. This run must finish
**partial** because exhaustive semantics/merge resolutions are unfinished; do not
convert its exit or scoped doc commit into review, acceptance or integration.

## Historical records below — superseded by the frozen Sol R2 section

## Current fetch — 2026-10-01 PDT

CLI `upstream` was refreshed with `git fetch upstream --prune`; MCP `main` was
fast-forwarded with `git pull --ff-only` in `/lump/apps/codebase-memory-mcp`.
No CLI pull was performed.

| Item | Value |
|---|---|
| CLI HEAD | `38153854c46f237b700d68b60a65c1a1078c78c5` |
| upstream/main | `0f52d30c2964a5a538149f8b39846ba52517a020` |
| merge-base | `17786374dfc39b8c751933d34d269501f2b4d6b7` |
| ancestry-only upstream commits | 646 total: 348 non-merge, 298 merge |
| non-merge patch equivalence (`git cherry`) | 2 equivalent, 346 without equivalent patch ID |
| MCP checkout after pull | `0f52d30c2964a5a538149f8b39846ba52517a020` |
| MCP pre-existing dirty path | `graph-ui/tsconfig.tsbuildinfo` (preserved) |
| graph index refresh | not completed: `cbm` and `codebase-memory-cli` both fail at secure coordination cache initialization (`cache-resolve`) |
| CLI graph index refresh | not completed for the same `cache-resolve` failure |

A full-hash non-merge census, including subjects, changed paths, `git cherry`
result, and provisional review category, is in the Agent-Workflow run evidence
file `evidence/upstream-nonmerge-census.tsv`. The two patch-equivalent hashes
are not proof of equivalent behavior; ancestry commits and semantic applicability
still require source-level review. The prior snapshot below is historical and
must not be read as an exhaustive disposition of this refreshed 348-commit set.

## Scope boundary

Eligible work is shared CLI, daemon, pipeline, store, Cypher, extraction,
build, and release behavior. Exclude `src/mcp/**`, MCP-only tests/adapters,
OpenHands integration, Jenkins-only behavior, hooks, and unrelated upstream
features. Preserve the fork's existing CLI output and packaging policy unless
a concrete CLI defect requires a change.

## Completed or represented port work

These source verticals were completed, represented, or explicitly reviewed in
the CLI branch. Do not select them again:

| Upstream work | CLI evidence | Result |
|---|---|---|
| First/second-wave core, daemon, and CLI verticals | `d3bb290c`, `c716cb7d`, `fc9aeedc` | merged |
| Bounded Cypher trails | `8546070d -> 0c5ffa50 -> 0d97729c -> cd22487c -> b3d31ca0 -> 58ef9f19` | already represented |
| Go struct binding | `e497dd0c` | merged; missing `349aecba` behavior adapted |
| Daemon activation/rendezvous | `6e289d5a` | merged; missing source behavior adapted |
| Cypher scope/capacity | `a7721244` | merged |
| Registry receiver chain | `e52b31cd` | merged |
| Daemon cohort handoff retry | `9104feb6` represented by `3036e195` | no duplicate port |
| Earlier parser, LLVM/MSan, daemon startup, install/config, and safety work | prior CLI history | represented; recheck before selecting a follow-on |
| YAML removal guards | `523d398d` | complete on Linux; native Windows qualification remains |
| Version-cohort handoff | `01af5cb2` | complete on Linux; native Windows qualification remains |
| Complexity determinism | `43d3a210` | complete |
| Razor / `.cshtml` extraction | `5b1bc085` | complete |
| Embedded Svelte/HTML/Astro extraction | `f8971e7a` | complete on Linux; native Windows not required |

The completed work was validated with `git diff --check`, incremental builds,
and the corresponding focused CLI suites at the time of integration.
Windows-only evidence remains explicitly qualified above; Linux results do not
substitute for native Windows execution.

## Prior-tip review queue — 2026-09-20 snapshot

This queue records the earlier `92abefa3` boundary. The four dispatched
verticals below have completed workers and valid receipts, but remain pending
independent review and acceptance; their branch tips are recorded in the Luna
dispatch table below. These are not authorization for bulk cherry-picks:

| Source | Initial disposition |
|---|---|
| `e8a46c16`, `6166934d` | dispatched TypeScript-await/Cypher work; worker receipts valid, pending independent review and acceptance |
| `9db48340`, `9db5217b`, `1d58385b`, `7b77fd48`, `da3258f3` | inspect memory-core overlap and CLI impact |
| `8c1a9d61` | inspect extraction/grammar portability |
| `c7ccb479`, `e0becdeb` | dispatched properties/XML work; worker receipts valid, pending independent review and acceptance |
| `6b480974`, `9a2bdc8c`, `c3714f11` | inspect route/pipeline correctness and dependencies |
| `3b7f2559`, `581f0c74`, `00411ce7` | inspect CLI alias/install policy |
| `243b405a`, `f0a86ef2` | inspect fork-specific CI/release applicability |
| `96a2fc35`, `dae1e7df`, `22ce867e`, `62fd80b8` | tests/style/docs/metadata; do not port without a concrete need |

The remainder of the 579-commit ancestry delta stays pending until its source history
is audited in dependency order. A source commit is not complete because it is
fetched, applies cleanly, or has a worker exit; completion requires the scoped
CLI change, review, focused validation, and acceptance evidence.

## Required merge record

For each future port, append one row here containing source hash, CLI commit or
represented behavior, adaptation/exclusion reason, focused test command and
result, build/diff-check result, and acceptance date. Keep raw upstream fetch
state and local integration evidence separate. No push is implied by this
ledger.

## Agent-Workflow dispatch notes — 2026-09-20

The Terra assessment run is `terra-cbm-upstream-20260920`, prepared and
started at source revision `a43b2201` with the dirty checkout recorded. The
following operational issues are retained here for the next delegation:

- `--agent-name terra` was rejected because configured preferred names are
  `luna-*`; the run was prepared with generated name `agent-47` and explicit
  model `gpt-5.6-terra` instead.
- `--role review` cannot be combined with explicit model/reasoning options;
  using `--agent-class review --executor codex --model gpt-5.6-terra` worked.
- The configured Codex executor has no evidence-capable late-steering adapter.
  Parent steering message `c8ffdcb7-ea59-4a15-8c13-7deafc089017` was
  persisted but delivery was reported `unsupported`; acknowledgement remains
  an explicit lifecycle gate.
- The repository was dirty before dispatch, so preparation required
  `--allow-dirty`; the worker was instructed to remain read-only and preserve
  all pre-existing changes.

These are workflow/evidence limitations, not merge recommendations. Do not
mark the Terra assessment accepted until its completion, evaluation, review,
and acknowledgement state are separately recorded.

## Luna implementation dispatch — 2026-09-21

Terra's top two overlapping Cypher recommendations were grouped into one
dependency-ordered worker. Four Luna implementation runs were dispatched from
clean isolated worktrees at `a43b2201`:

| Run | Vertical | Worktree | State |
|---|---|---|---|
| `luna-cbm-cypher-20260920-r1` | `458b0db1 -> 6166934d` Cypher correctness | `impl/cbm-cypher-20260920` | worker completed; valid receipt; tip `42ca613c` |
| `luna-cbm-xml-20260920` | `e0becdeb` XML discovery | `impl/cbm-xml-20260920` | worker completed; valid receipt; tip `d6177b61` |
| `luna-cbm-ts-await-20260920` | `e8a46c16` awaited TypeScript calls | `impl/cbm-ts-await-20260920` | worker completed; valid receipt; tip `c540426f` |
| `luna-cbm-properties-20260920-r1` | `c7ccb479` parser-local properties state | `impl/cbm-properties-20260920` | worker completed; valid receipt; tip `badb8cce` |

The first parallel preparation attempt returned `execution lifecycle is not
initialized` for the Cypher and properties runs after creating durable state;
reusing those IDs was rejected, so lineage-preserving `-r1` IDs were prepared
sequentially and started successfully. All four runs have persisted parent
steering messages, but the configured Codex executor reports late-steering
delivery as `unsupported`; acknowledgement is required before acceptance.

Workers must return commits and evidence only. Parent integration, independent
review, acceptance, and push remain separate gates.

## Terminal observer diagnosis — 2026-09-25

The two apparent duplicate Agent-Workflow terminals were duplicate observers of
the same Terra run, not duplicate Agent Runs. This session launched
`agent-workflow agent-run tail terra-cbm-upstream-20260920` twice, once with
`--lines 80` and once with `--lines 40`, then forwarded only captured output
and discarded the returned exec session handles.

`agent-run status` shows Terra and all four Luna runs are terminal, with
`worker_alive: false`. A bounded read-only probe of `agent-run tail` against
the already completed Terra run printed its final lines but remained attached
until the two-second timeout (`exit 124`). Thus the worker did finish; the
tail observers do not auto-exit when the run completes. They remained in the
terminal UI because this session failed to interrupt or retain handles for
those long-running observers. A process listing in the current shell did not
show their PIDs, so their present OS-level liveness in the displayed terminal
environment is not independently confirmed here.

The four Luna workers are completed and their receipts are valid. Completion
does not imply evaluation, review, acceptance, integration, or push: no such
state is established for these runs. Evaluation remains `not_planned`. Terra's
assessment also remains unaccepted pending its separate completion, evaluation,
review, and acknowledgement gates. This is separate from the still-attached
tail commands.

## New upstream delta plan — 2026-09-28

Status: **planned, not implemented**. This is a ranked set of ten candidate
verticals from the 87 non-merge commits since `92abefa3`; ranking favors a
bounded correctness/performance fix, a direct CLI seam, and an existing focused
suite. “Overlap” below describes the CLI path to inspect, not a claim that the
MCP implementation can be copied unchanged. Recheck each exact source diff and
reverse applicability against the eventual integration base.

| Rank / source | CLI overlap and minimal change | Dependency / focused check | Acceptance / effort |
|---|---|---|---|
| 1. `99c3bb4c` — failed store `COUNT` | `src/store/store.c`: preserve and return the failed COUNT read status instead of reporting a successful zero. CLI store implementation is shared in shape; verify exact error path before adapting. | None beyond existing store API. `scripts/test.sh --suites store_nodes` | Inject/read failure is distinguishable from a real zero count; existing zero-count behavior remains. **S** |
| 2. `d7eba5a7` — persisted LSP decode | `src/pipeline/lsp_surface.c`: replace repeated indexed JSON-array lookup with one linear traversal while preserving decoded order and values. CLI has the corresponding persisted-surface pipeline path; confirm decoder shape. | Keep current JSON representation and schema. `scripts/test.sh --suites pipeline` | Round-trip fixture decodes identically; iteration is linear in array length. **S** |
| 3. `e4780c20` — macro parse-coverage scan | `internal/cbm/cbm.c`: avoid rescanning all source text once per parser error region; prepare line offsets once and reuse while deriving coverage gaps. | Reuse coverage/error-region contracts; no new parser. `scripts/test.sh --suites parse_coverage,extraction` | Existing coverage outputs stay stable on fixtures while many error regions do not multiply source scans. **M** |
| 4. `76e54f75`, `4fc5999a`, `61ccd905`, `08642202` — watcher hard-failure backoff and QA | `src/watcher/watcher.c/.h`: port the hard-failure backoff with its QA follow-ups, especially `4fc5999a`'s monotonic backoff guard; `61ccd905` and `08642202` tighten tests and contract. | Preserve watcher lifecycle/cancellation semantics and monotonic timing. `scripts/test.sh --suites watcher` | Sustained hard failure does not fork-loop, elapsed-time checks use monotonic time, and the tightened contract tests pass. **M** |
| 5. `77ab4bea` then `7707c9d7` — dynamic AST/definitions walker stacks | `internal/cbm/extract_defs.c` and `internal/cbm/helpers.c`: grow pending-node stacks instead of silently dropping children at fixed limits; preserve source traversal order and non-truncated failure behavior on allocation failure. | The definitions walker change precedes broad metrics/token walkers; reuse memory-core allocation and logging. `scripts/test.sh --suites extraction,complexity,stack_overflow_a,stack_overflow_b,stack_overflow_c` | Wide/deep fixtures produce complete metrics and leading token samples; allocation failure does not return truncated content. **M** |
| 6. `c8184ed9` then `282fbfcd` — inherited `.gitignore` base | `src/discover/discover.c`: honor an enclosing repository’s ignore file for a git-less subfolder, resolving each ignore rule relative to its owning directory. | Apply parent discovery first, then correct relative-base handling; preserve local ignore precedence. `scripts/test.sh --suites gitignore,discover` | Nested invocation matches the enclosing repo’s ignore decisions, including nested `.gitignore` bases. **M** |
| 7. `8093d10b` — binary `.res` / ReScript hang | `src/discover` language admission and `internal/cbm/extract_usages.c`/vendored ReScript scanner: keep binary resource files out of text parsing or safely reject them before scanner work. | Match CLI’s vendored grammar and language contract; no new dependency. `scripts/test.sh --suites language,extraction,complexity` | Binary `.res` indexing terminates promptly without changing valid ReScript source handling. **S–M** |
| 8. `f339aa62` then `df8dd101` — Python scope-chain lookup | `internal/cbm/lsp/scope.c/.h` and `py_lsp.c`: first collapse repeated Python chain scans, then expose/use one lookup returning the nearest complete binding (presence, type, callable identity). | Preserve lexical shadowing, including unknown bindings; second change depends on the first lookup refactor. `scripts/test.sh --suites scope,py_lsp` | Existing binding/call resolution remains stable; chained lookups become one walk and shadowed names cannot resolve to an outer binding. **M** |
| 9. `460ebefc` — graph publish failure errno/log | `src/graph_buffer/graph_buffer.c` and `internal/cbm/sqlite_writer.c`: propagate/log the actual atomic publish failure so a failed publish cannot be reported as a successful dump. | Coordinate graph-buffer and SQLite-writer error contracts. `scripts/test.sh --suites graph_buffer,sqlite_writer` | Forced publish failure reports failure and its cause; successful publish still reports success. **M** |
| 10. `343d7dd5` — C-family return pointer/qualifiers | `internal/cbm/extract_defs.c`: retain pointer depth and qualifiers when extracting C-family function return types. | Keep language-specific type normalization and existing serialized schema. `scripts/test.sh --suites extraction` | Pointer and qualified return-type fixtures preserve their exact extracted type; ordinary return types remain unchanged. **M** |

### Deferred or lower-return candidates

- `c0a06b3d` adds a large SQL literal-values scanner; defer until a CLI
  benchmark establishes the value of its larger parser and test surface.
- `1a342553` is a broad graph-quality bundle; split and evidence individual
  defects before considering any port.
- `19d14447` daemon conflict UX is outside this pipeline/extraction plan; defer
  unless a CLI daemon-conflict workflow is selected.
- Windows-only temporary-file and race fixes need native Windows applicability
  and validation, so they are not in this ten-slice plan.
- MCP-only, CI-only, documentation-only, style-only, and test-only commits are
  excluded absent a CLI behavior change.

### Complete disposition of the 87 non-merge commits

The accounting below covers every non-merge hash in `92abefa3..80eb92a7`
exactly once: **16 selected source hashes** across the ten verticals above,
**24 applicable but deferred hashes**, and **47 excluded hashes** grouped by
scope. The source log was checked against these sets; the exclusion grouping is
for disposition accounting, not a claim that those changes are defective.

**Selected — 16 hashes:** `99c3bb4c`, `d7eba5a7`, `e4780c20`, `76e54f75`,
`4fc5999a`, `61ccd905`, `08642202`, `77ab4bea`, `7707c9d7`, `c8184ed9`,
`282fbfcd`, `8093d10b`, `f339aa62`, `df8dd101`, `460ebefc`, `343d7dd5`.

**Applicable but deferred — 24 hashes:** `dc735c01`, `19d14447`, `e9058e11`,
`075f40e9`, `8f803f72`, `c0a06b3d`, `246e4190`, `6ae6cd4c`, `f3bdf11a`,
`aaa72fbe`, `c314dea4`, `df83d25a`, `24afb562`, `1a342553`, `1d2bbc42`,
`ed991154`, `a041794f`, `d4931241`, `452485f7`, `25d08258`, `fcf451c6`,
`eb80a4ec`, `2ea73059`, `5cb90f64`.

**Excluded — 47 hashes:**

- MCP server/tool behavior and MCP-specific docs/style (9): `fa229211`,
  `629e9c2c`, `cda5e3c9`, `dadc57db`, `af3e003a`, `c067c771`, `58c1c0ca`,
  `60449f89`, `73c509f4`.
- Client, package, extension, and hook integration (12): `ff8dd470`,
  `e171bd48`, `8f3c1504`, `e8dcb28b`, `7883507d`, `88269ac2`, `e026a60b`,
  `004c5554`, `a378d47e`, `8c30c785`, `7a909466`, `f0589375`.
- Maintenance, tests, CI, vendor/security metadata, and docs/style (26):
  `259dbb67`, `ea6fa49f`, `07eb426c`, `bc960d87`, `efb23128`, `5a9c72d8`,
  `6f91a16f`, `64297ff3`, `9a9534cb`, `2f557f53`, `da4bd25a`, `65d60e72`,
  `a262a357`, `f428d583`, `bc5b3dd2`, `fa138acf`, `183cd727`, `95fa6344`,
  `f50434ed`, `3ab2994f`, `eccc6153`, `c1128db8`, `aec9bfe3`, `5abd77f5`,
  `d402623c`, `3604d6cf`.

### Per-slice workflow and evidence boundary

For each slice: prepare an isolated worktree from a refreshed CLI integration
base after reconciling the nine docs-only commits on `origin/main` and the dirty
main checkout; inspect the exact upstream source diff and reverse applicability;
make only that slice's CLI adaptation; run the listed focused suite plus the
incremental build/diff check; obtain independent review; record acceptance; and
integrate separately. Do not bulk-merge upstream or combine these ten changes
into one implementation branch. No tests, builds, reviews, acceptances,
integrations, or pushes for these planned slices are claimed here.


## 2026-10-01 refresh limitations and next review boundary

The census enumerates all 348 ancestry-only non-merge commits exactly once and
records full hashes, subjects, paths, patch-ID status, and provisional categories.
Those categories are triage only: per-commit source/reverse-applicability review
was not completed, so the census does not claim final semantic disposition or
rank every new vertical. The 298 ancestry-only merge commits are included in the
reconciliation count; they are not independently assigned implementation slices.

The MCP pull succeeded by fast-forward from `80eb92a7017dab9a0773430433660a966b80cc15`
to `0f52d30c2964a5a538149f8b39846ba52517a020`, preserving its existing dirty
`graph-ui/tsconfig.tsbuildinfo`. Both requested full, non-persistent graph
indexing commands were attempted but stopped before project selection/indexing
because secure cache initialization returned `cache-resolve`; therefore there
is no new generation, coverage, or before/after graph status to report. The
independent parity report was not available in this run, so manage-adr exposure
and parity findings remain unverified. No code, tests, builds, pull/merge of CLI,
or changes outside this canonical document and run evidence were made.


## 2026-10-01 bounded slice dispatch

The ledger/parity workers exited with reports, but their Agent Runs have
`completion_missing`: neither has valid host-verified completion, evaluation,
review, or acceptance. The parity report is now available in Agent-Workflow
run `CBM-MCP-CLI-PARITY-20260930`; it identifies the missing public ADR alias and
explicit `outline` behavior. Full upstream semantic sorting and both requested
reindexes remain incomplete; the census remains provisional. These two bounded
assignments are based on current source and specific existing adaptations, not
on a claim that the full census is ranked or accepted.

| Agent Run | Slice | Source / reuse | State |
|---|---|---|---|
| `CBM-ADR-SLICE-20260930` | Public `manage-adr` command/help and explicit read-only ADR `outline`; retain existing omitted-mode `get` | Parity report plus MCP `0f52d30c`; reuse the current ADR operation | Queued: isolated `impl/cbm-adr-slice-20260930` at `38153854`; baseline acceptance preparation precedes automatic launch |
| `CBM-COUNT-SLICE-20260930` | Distinguish failed node/edge COUNT reads from successful zero | Reuse local `5ef42e21`, adapting upstream `99c3bb4c`; inspect callers/scoped helpers | Queued: isolated `impl/cbm-count-slice-20260930` at `38153854`; baseline acceptance preparation precedes automatic launch |

Both runs explicitly select GPT-6-Luna and bind incremental production build,
focused tests (`cli,incremental` for ADR; `store_nodes` for COUNT), and
`git diff --check` as acceptance commands. Run evidence and launch scheduling
live under Agent-Workflow authority/task inputs. Workers may make scoped local
commits; no integration, push, PR, review, or acceptance is authorized/claimed
by this dispatch entry. Existing branches/worktrees and dirty primary content
remain preserved. The release-readiness worker also exited without valid
completion; its documentation commit `fd2aca2a` is separate from these slices.

## 2026-10-01 recovery evidence and outstanding work

The independent parity audit is available at
`/home/nate/.local/state/agent-workflow/runs/CBM-MCP-CLI-PARITY-20260930/evidence/parity-report.md`.
It found no operation-name absence, but `manage_adr` is compatibility-only:
there is no top-level `manage-adr` alias/help route, and CLI omits MCP's
`outline` mode while using a different default. The report prioritizes adding
the alias and resolving mode parity, followed by registry parity checks.

The recovery retry re-confirmed CLI `38153854c46f237b700d68b60a65c1a1078c78c5`,
upstream/MCP `0f52d30c2964a5a538149f8b39846ba52517a020`, and merge base
`17786374dfc39b8c751933d34d269501f2b4d6b7`. The prior provisional census is
not a complete semantic review. Current-run evidence has a fresh exhaustive
hash/subject/path enumeration, but its 348 rows remain explicitly
unclassified; ranked vertical slices and source-informed dispositions have not
been completed. See `evidence/census.tsv` in Agent-Workflow run
`CBM-UPSTREAM-LEDGER-20261001-R2`.

Both required full in-memory indexes were attempted with isolated run-owned
`CBM_CACHE_DIR` values and failed before index access: the active account daemon
is bound to a different cache root. The CLI directs operators to close all CBM
sessions before switching roots. No daemon was stopped, and no graph freshness,
generation, or coverage is claimed. MCP's dirty `graph-ui/tsconfig.tsbuildinfo`
remains preserved. Detailed command results and the deterministic verifier are
in the current run's `evidence/` directory. The verification gate is expected
to fail until source dispositions, ranked slices, and both indexes are complete.


## 2026-10-01 candidate verification and next-three dispatch

The `d7eba5a7` persisted-LSP-array optimization is already represented on the
frozen `release-tooling` base: `fcc51bc9` is an ancestor of
`637678ec43a051194bd41c099bcaae1b15f057e6`, and the base decoder iterates both
the `lsp` object array and nested string arrays with `yyjson_arr_iter`. It is
excluded from this next-three set as already represented, independent of the
Jev ranking.

The remaining three selected verticals were source-checked against the frozen
base and exact upstream commits. A bounded Jev selection call returned
`macro_watcher_ast` (`jev-1.13.0`, confidence 0.96; probability 0.97, with 0.03
for insufficient evidence). Jev advised the ranking; source and deterministic
validation remain authoritative.

| Rank | Upstream source | CLI applicability on frozen base | Candidate branch / Agent Run | State |
|---|---|---|---|---|
| 1 | `e4780c208632b3c42488a3c9e315014ef4da157e` | `internal/cbm/cbm.c` scanned from byte zero per parse-error region and asked the source-dependent macro predicate before the pure callable containment check. The port checks containment first, shares one line-offset table for callable regions, preserves the walk fallback on allocation failure, and adds deterministic scan-byte regressions. | `impl/jev-macro-scan-20261001` / `JEV-CBM-MACRO-SCAN-20261001` | Local commits `498b0375a2359c25e338997f254c30cabd20db42` and follow-up `04c0728fdb4c06b70b407b539fb256da9a9c8939`; source checks pass, tests/builds not run. Parent integration and Jenkins suites `parse_coverage,extraction` remain pending. |
| 2 | `76e54f7525983ecd8fdeef6fdc14896a72be7063`, `4fc5999a`, `61ccd905`, `08642202` | `src/watcher/watcher.c` left hard `index_fn` failures on the ordinary poll cadence. The port adds capped doubling for consecutive hard failures, resets the streak on success, schedules from a fresh monotonic reading, and keeps baselines unchanged on failure/busy. | `impl/jev-watcher-backoff-20261001` / `JEV-CBM-WATCHER-BACKOFF-20261001` | Commit `9ac0e3f8b900685ea657bac81659eeb24cc43df6`; source/diff/format checks passed, tests/builds not run. Parent integration and Jenkins suite `watcher` remain pending. |
| 3 | `77ab4bea87008547acee60435a8e3df29d90a5cb`, followed by `7707c9d7` | `internal/cbm/extract_defs.c` body-token walker has a fixed 512-entry pending stack and silently stops queuing children at capacity. Grow pending storage and report allocation failure without returning truncated token content; preserve the separate output token cap and traversal order. | `impl/jev-ast-walker-20261001` / `JEV-CBM-AST-WALKER-20261001` | Commit `cf7248a8` (`fix(extract): grow body token walker stack`) at frozen base; `git show --check` passes. Tests/builds and durable run-status verification remain pending (Agent-Workflow status calls errored). Focused Jenkins suites: `extraction,complexity`. |

All three candidate runs have separate Agent-Workflow authority and isolated
worktrees at `637678ec43a051194bd41c099bcaae1b15f057e6`. The parent integration
candidate has advanced to `c98416102331a1c3f68f16b8a21b148ab0de46eb` and includes
other concurrent fixes; the slice branches remain based on their original
frozen SHA. Macro commits are local implementation evidence only. Watcher and
AST implementation is locally committed; integration, independent review,
acceptance, and Jenkins union validation remain pending. Frozen candidate commits and integration
results are separate evidence.

### Existing local candidates — Jenkins build 2 evidence

| Candidate | Exact Jenkins checkout | Observed result | Lifecycle status |
|---|---|---|---|
| COUNT `784974f9e14be3d1b2d19389b8ff3a0774993a39` (`local-mr/count-20261001`) | Jenkins console confirms the exact SHA | `store_nodes`: 71 passed; build `SUCCESS` | Local implementation candidate with focused Jenkins validation; independent review, acceptance, integration, and push not established. |
| ADR `ee4a04e3c4c811e833ccd3803b35d8ec4aaf823e` (`local-mr/adr-20261001`) | Jenkins console confirms the exact SHA | Build `FAILURE` during lint: banned NOLINT comments at `src/main.c:1204` and `src/operations/adr.c:125,363`; test stages did not run | Local implementation candidate; fix lint and rerun. This is not a test failure or acceptance. |
| Properties `3ae5ab454b9a9188cea2e2f735e30b4704bdceeb` (`local-mr/jev-properties-20261001`) | Current local branch tip verified | Candidate only; no matching completed Jenkins checkout/result established in this record | Review/validation/acceptance/integration/push remain unverified. |

Exact COUNT and ADR console excerpts are retained in
`evidence/existing-candidate-jenkins-20261001.md` under Agent-Workflow run
`JEV-NEXT-THREE-SLICES-20261001`. No index reset is claimed; the previous
cache/daemon limitation remains in force.


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

Shared job #32 reported an include-formatting error before tests; correction
`23349698` is committed locally. Its rerun and publication remain pending.

Shared job #33 at `23349698` completed with 7,510 passed, one failed and
three platform skips across the shards. The failure was the new macro test's
upstream-specific expectation of 40 distinct regions; this fork's grammar
produced one. Commit `37dd63ba` preserves the test's meaningful guarantees
(nonempty parse gaps, zero scanned source bytes) and frees the result before
assertions so a failed check cannot obscure its diagnosis with leaked fixtures.
The complete AST wide-input regressions passed in that extraction run.
Job #34 passed the full gate at `37dd63ba`, which is now `main`.

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

## Consolidated full-hash disposition appendix — 2026-10-02

This appendix is the current canonical hash inventory for the frozen differing scope.
It is generated from verified exact assignment sets and the Sol R2 census.
R4 worker verdicts below are **checkpoint claims pending parent whole-hunk review**,
not accepted dispositions. Source-only findings and machine crosswalks are retained
in `CBM-UPSTREAM-CONSOLIDATE-20261002` evidence. The dated
[synthesis report](UPSTREAM_RECONCILIATION_REPORT_20261002.md) explains this snapshot;
this ledger and `BACKLOG.md` retain task authority.

### Adapter-only source-reviewed batch — 2026-10-03 UTC

`CBM-RECON-ADAPTER-LOW-20261003` source-reviewed the 16 previously unreviewed
runtime hashes identified below as adapter-only. That run released the exact
set from `CBM-RECON-RUNTIME-20261002-R4` with a worker-issued acknowledgement;
`CBM-UPSTREAM-CONSOLIDATE-20261002` then verified the run's 67-file hash manifest,
confirmed all 16 are single-parent non-merge commits reachable from the frozen
upstream tip and absent from production, original-QA and later-QA ancestry, and
confirmed every changed path is `src/mcp/**` or `tests/test_mcp.c`. The `src/mcp`
subtree exists in upstream with seven files and is absent from all three CLI
trees and from this audit worktree. The 16 hashes are disjoint from the 115
hashes handed to the three DeepSeek Phase1 workers, and the plan excludes exactly
this set, so primary ownership stays unique (111 extract / 86 history / 39
runtime / 39 runtime-Phase1 / 26 ops-Phase1 / 50 integration-Phase1 / 16 here).

Eight hashes close as adapter-only because every changed hunk is MCP request
routing, an adapter response envelope, an adapter-local comparator or allocator,
or an adapter test with no CLI target: `e65bc6b7`, `eeff3f19`, `96a2fc35`,
`60449f89`, `629e9c2c`, `b7bcb15c`, `b94afb44`, `364dd6a9`. The earlier
"represented" reading of `629e9c2c` was withdrawn: no shared nullable-ordering
behavior exists in the CLI target.

Eight hashes stay open with bounded remainders because the shared portion cannot
be closed from source alone: `b5f10c22`, `73c509f4`, `357d71e1`, `87471e1f`,
`fe3c285f`, `155d147e`, `1f0daaa9`, `e548d971`. `e548d971` keeps its adapter
leak hunk excluded while its shared zero-row allocation-ownership contract is
represented by exact target APIs; the absent MCP fallback caller leaves
applicability unresolved. Exact hunk, group and remainder text per hash is in
that run's `dispositions.jsonl` and `HANDOFF.md`; the consolidation checkpoint is
`evidence/adapter-low-batch-consolidation.json`.

The conservative whole-commit non-merge remainder therefore drops from 364 to
356. This is a bounded source finding only: parent whole-hunk review, acceptance,
implementation, runtime/tests/Jenkins validation and integration are all still
unestablished for these rows.

### All 367 differing non-merge commits

`Open` means the whole-commit semantic remainder is explicit even where a bounded
source behavior is represented or missing. `Empty` means exact parent-tree identity,
not acceptance or source coverage. Group containers are provisional unless a
source-checked vertical is described above. All listed hashes are reachable from
the frozen upstream tip and absent from original frozen QA ancestry.

| Full upstream hash | Parent bounded disposition / group | R4 checkpoint claim | Whole-commit remainder |
|---|---|---|---|
| `dc24d5813081ba7f4f1cb282cd9e10b8ac963eb7` | direct_adapter_import_excluded_parity_unresolved; native-merge:b44e58490f11e4ee895dd91c02067b6de59cdf96 | unresolved; no retry semantic review yet | Open |
| `0106ff5d9933b9bd65eba393a998586e219da7bd` | direct_adapter_import_excluded_parity_unresolved; native-merge:5fbab7bb7332bd06aaa880653ddfb2696e648f90 | unresolved; no retry semantic review yet | Open |
| `c537bf0f8b3b6a77e39df49fec78560b5e8e87b7` | semantic_unresolved; native-merge:1424934980d353f97c3664f07b2c1721792b7efa | missing | Open |
| `1e0aac527b016d519eb7f8fc7dd610dbeb66e988` | semantic_unresolved; native-merge:c895513f2d6821a3479ea4cbdcf8a39cee2f33ba | missing | Open |
| `1e25d962d69999438229b0562d44ea60e97f5456` | semantic_unresolved; native-merge:4d8ac9e1cea0d645a02f876f47536c54a5bfa406 | unresolved; no retry semantic review yet | Open |
| `76e54f7525983ecd8fdeef6fdc14896a72be7063` | represented_source_only; watcher-backoff | unresolved; no retry semantic review yet | Open |
| `4fc5999a85f50bbb131dd87c5f41949c08617c1a` | mixed_partial_representation; watcher-backoff | unresolved; no retry semantic review yet | Open |
| `61ccd905111e6b9e0f262a7636d4af399e9be192` | regression_followup_unresolved; watcher-backoff | unresolved; no retry semantic review yet | Open |
| `086422020f5fd9396b46384b10413cdca6994c96` | regression_followup_unresolved; watcher-backoff | unresolved; no retry semantic review yet | Open |
| `c36b4fbc446f9085704a6073b81ec743fd4180fc` | semantic_unresolved; route-decorator-paths | unresolved; no retry semantic review yet | Open |
| `3f831936b5f3012d13175e7c97326165efecefad` | candidate_not_represented; swift-shift-width | missing | Open |
| `a2ea711c29560511eb8467bea7ad309ebf500d6e` | candidate_not_represented; swift-shift-width | missing | Open |
| `e8204092333ee1c38cd681a81abb36f125288894` | required_candidate_followup; swift-shift-width | excluded | Open |
| `2f37f61e8848f31f308bce8623f66dc2fdb50619` | direct_adapter_import_excluded_parity_unresolved; native-merge:963980317538dec5674010f68238f3dff317a59e | unresolved; no retry semantic review yet | Open |
| `25a7aacc87250bff752ff61cd33e9236be5d1310` | represented_source_only; cypher-parser-consumption | represented | Open |
| `62b44519c68b403ea44d9a85ba8ee74cddee66e6` | semantic_unresolved; native-merge:7910de591b23c0e5d71b923f9e9b38933ce50aa9 | unresolved; no retry semantic review yet | Open |
| `9d4ac2b98b9c19633ce2cbf8d9c6bf9edc57bba6` | semantic_unresolved; native-merge:2a0e9b9b803cb0913b8bebeab12b0174f4c8e9cb | unresolved; no retry semantic review yet | Open |
| `44caa4c386c578e59c5919735e1e4d6094ac1af7` | semantic_unresolved; native-merge:e6a34fcea01611ad515b8bb9a6dcac107ab59b63 | represented | Open |
| `fd73c347fbcfd7f66c210577b4d713fc6ce7780b` | semantic_unresolved; native-merge:e6a34fcea01611ad515b8bb9a6dcac107ab59b63 | partially represented | Open |
| `592894a4387a50e1d128ff6a2695ff48f8cc32a5` | semantic_unresolved; native-merge:01bf5f23e3e61353c4bc21575ec09ac6f40e46bf | missing | Open |
| `349aecba9528427fa635975e246f5b8959b14f30` | semantic_unresolved; go-field-selector | partially represented | Open |
| `2bff501a62a3bba1aeb39f49f7c785d541ca209e` | semantic_unresolved; native-merge:c38aa35c277395e71fc8fd2f904231e6d909b5f0 | unresolved; no retry semantic review yet | Open |
| `e0785b353af1af3812d5d14cad55c839efc5749d` | semantic_unresolved; native-merge:0e9ed75991826d787d96c8abaf196ad0fdbf9f70 | missing | Open |
| `40f2722d6166c1f97fa1667c1e63e8c97e6f05f0` | semantic_unresolved; native-merge:db2f104a1f342fdcae6ac5abd0c08e9da36686ad | missing | Open |
| `95689b5c20101db7d92b69767bbea75e5bf0d4fc` | semantic_unresolved; python-bare-parameter-binding | missing | Open |
| `8bcfa2915fa02721c5b6d2a6c7082a26fef79d88` | semantic_unresolved; python-bare-parameter-binding | excluded | Open |
| `0b0d143c2d733e3313eef5b9d9745b0fb08cc2c0` | semantic_unresolved; python-bare-parameter-binding | superseded | Open |
| `97517a4652f376cb6bf096255e235edf456f86b1` | semantic_unresolved; python-bare-parameter-binding | missing | Open |
| `572725e61a00b9deda7407971dc9290477170515` | semantic_unresolved; native-merge:6cca768816ee8fe8c1ed591d27c659fd01635a3f | unresolved; no retry semantic review yet | Open |
| `93d2ee59eaec4999ca666a8be214de8d9f2f2bb7` | direct_adapter_import_excluded_parity_unresolved; native-merge:f37a47eb01dcf1fa7c5e15e57d99e388abdd5482 | unresolved; no retry semantic review yet | Open |
| `2d8721a60360df5804dfcce37640a2ca5fa0eac0` | direct_adapter_import_excluded_parity_unresolved; native-merge:f37a47eb01dcf1fa7c5e15e57d99e388abdd5482 | unresolved; no retry semantic review yet | Open |
| `240dc8156ba3ce9be92e2142e8506fef727710b4` | direct_adapter_import_excluded_parity_unresolved; native-merge:f37a47eb01dcf1fa7c5e15e57d99e388abdd5482 | unresolved; no retry semantic review yet | Open |
| `adb49b3a28826e6eef871b1777b63668199c0463` | direct_adapter_import_excluded_parity_unresolved; native-merge:f37a47eb01dcf1fa7c5e15e57d99e388abdd5482 | unresolved; no retry semantic review yet | Open |
| `ad04339030bbfea4e77139bcfa9a87356c1e0e5b` | semantic_unresolved; native-merge:bac8edc58dfc700d8e24dcd129fb7715e68febe7 | unresolved; no retry semantic review yet | Open |
| `c4e902849f8062e37a0f0ddaed1c6cc042780cb3` | semantic_unresolved; native-merge:5802ccdd77c9913f8643092839c698bad10b2c4a | missing | Open |
| `0ac290e2e243148b9e84bb682cf1101fe38bc6e3` | semantic_unresolved; native-merge:951e02e2f998f324b616d7acf6c6bcab8b97c956 | unresolved; no retry semantic review yet | Open |
| `9baa0a8dc5cbe7a9fa687e0a8811dfcbdcd0551e` | semantic_unresolved; native-merge:2f9828d6de76aac7a15fa3fb1da83a1ae0839660 | missing | Open |
| `957b27fca50d2c3b1ac86d771733f39612bae841` | semantic_unresolved; native-merge:2f9828d6de76aac7a15fa3fb1da83a1ae0839660 | superseded | Open |
| `57ef0a6d8662e9854dc5711922f801c3da381071` | semantic_unresolved; native-merge:9d0c6a1f1af852c54bba74e61401b21e4f345a51 | missing | Open |
| `2c76563a137876b110cec15b9b6fa08d35069f73` | semantic_unresolved; native-merge:91a1723175f7efb56679f9ef02170951fffce780 | represented | Open |
| `1770d68a2c934501b889e625dc94ef933e3a1815` | semantic_unresolved; native-merge:9ed01a478ccf5a451f88275737a1e209938bbff8 | represented | Open |
| `48cb94f65ce221e1494119d850d35019c86a59e2` | semantic_unresolved; native-merge:3bbabf187c405cefcfa855aa8ecfa11c5d32d807 | missing | Open |
| `73f4a432e4ac1d575635a490d96e0f5ea588344d` | semantic_unresolved; native-merge:44fa3aeb213c07ff75bcf4cc44f8ba9fbdfb04bf | missing | Open |
| `1d38be3331576f50ce555d99717974060a6a16a8` | semantic_unresolved; native-merge:c803525258e651f9ed3776aeade9cda4762b552a | unresolved | Open |
| `be66a2d2c6f1538a4a831eb2c91622997be5b5ec` | semantic_unresolved; native-merge:b95b33af863f9e711a0c82afb620f8760089c03f | missing | Open |
| `6b801eec68d6465aad2020de438e629cead99f5c` | semantic_unresolved; native-merge:54a434743ea61147a8c27fc8a80f1fa6da5b46e3 | unresolved; no retry semantic review yet | Open |
| `63b99976a21463697d6e980e32aa42f5913319da` | semantic_unresolved; native-merge:8f64c36d85754293895f929801bdffc58cead730 | represented | Open |
| `426e415f5427f2fb2e9c6f918cc7cc9abd9c109b` | semantic_unresolved; native-merge:8f64c36d85754293895f929801bdffc58cead730 | represented | Open |
| `f404dfbc7ae47f9912b18a0554a5e5d91f65be1a` | semantic_unresolved; native-merge:184a00a442bc87a888aaf3f39b801a561e03432e | missing | Open |
| `9104feb67f78959e64ddff78d236e0dea475a434` | semantic_unresolved; native-merge:13b7d067018ceafcb04519675ade885ade76815a | represented | Open |
| `4b1cdf57804afffbcb61e69fd484b42858fcc02f` | semantic_unresolved; native-merge:8e1de0403eddd33589fd00f2af5d8ab90b600e0a | missing | Open |
| `7c34adf8bb60f873bd525e3d45ab78a395874582` | excluded_empty_tree_change; native-merge:8e1de0403eddd33589fd00f2af5d8ab90b600e0a | excluded | Empty |
| `cd3ea5b35bfad379f3d76b5cc83f78826b57555c` | excluded_empty_tree_change; native-merge:8e1de0403eddd33589fd00f2af5d8ab90b600e0a | excluded | Empty |
| `dc0f8ae642bf39b4b11bec2a88a8cdc7a1e4ab5b` | semantic_unresolved; native-merge:ffb5ebe9e8ad2763fc6388c001ca8a16ca834d88 | missing | Open |
| `7e47043acce9bdfd87e7becc7fe7a348b02753ec` | semantic_unresolved; native-merge:5c8ce584eac30b26cb98f53d4bf8c3e7d6b4b9c2 | excluded | Open |
| `ffc29f73bb27dfbad778acbc48e1a3a54f17d1b5` | semantic_unresolved; native-merge:a1e1beb242e5c90a2f925688dee96d54b7cbb55c | missing | Open |
| `6b0c44afcfeae65eed82e2dbbd58f00b043f21f6` | semantic_unresolved; native-merge:d97ee2be28d678dd153327468e8b7527bec7a82a | missing | Open |
| `132e8fc32309e843ccd97138c8f032e317333673` | semantic_unresolved; native-merge:202c8e42b56abe2f55025e660e6042c449106823 | excluded | Open |
| `7d20ff9398cb494a61f6cd259a51b309dbe5bd54` | semantic_unresolved; native-merge:b6e4beb9b17ca12b65128f30d12e5bb051ea002c | missing | Open |
| `39c74bdd5c8b5dc3bcd43f962195add0ba794150` | semantic_unresolved; native-merge:b6e4beb9b17ca12b65128f30d12e5bb051ea002c | missing | Open |
| `fece72b049db1954eb38f2fce139cec5eb499b00` | semantic_unresolved; native-merge:b3d898e12fba86ce5582f87c05db729df2d71424 | missing | Open |
| `7b92d2a6218936767b112081b9aa4e79d5b81dd9` | semantic_unresolved; native-merge:b3d898e12fba86ce5582f87c05db729df2d71424 | partially represented | Open |
| `1a6f80c37baed2d4e670304f5b95e766289aeaa4` | semantic_unresolved; native-merge:ceba42728aa8e5a54acfe7dc8870a948c210badc | represented | Open |
| `3909e5a0e1d55236fccac9ab680a1c6103ce5daf` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | partially represented | Open |
| `e842fd78cf4027c7d9d2ffa7e54ec0b219827d0a` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `5f58e233dc5ba70acc039c7bc482d5399391ae69` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `dbadb736dde7f4beb0a55151632044e1399c6d8c` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `17d61ea9be09b004a3e64f330c82c2f716a10a4e` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | excluded | Open |
| `10ece1bb8690587d62c4f44bd9f275a6b6b987c4` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `240f252b707ff9702b0996718ba40f2edaac4efd` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `4a9fcd97d0d758c9ced85396ef973ca38e97cfc8` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `a31d2679c14c953e2c9d7de2a6b54b3ceefeafa3` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `14c229b158ba6cb05862c5b843ba48d3ea4b44c9` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `5630f11f21b19c9151f7f20161cdc22f3ed95c19` | direct_adapter_import_excluded_parity_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `f30ae39eb7f2e8ba9c918e2fde7473259d122e49` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `10af29d50c852f59d1a5e13273e717ce1f4b7816` | direct_adapter_import_excluded_parity_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `8a8c6839f50076815ea2aa83b66ed65641e3847a` | direct_adapter_import_excluded_parity_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `7be95be570027954e9c63c9ce65f95d21ba7d36c` | direct_adapter_import_excluded_parity_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `daa5dc824784765eddd2067eec9517885acfbdf8` | direct_adapter_import_excluded_parity_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `26650255ef865de00c4a62dbcfdc0743490c5256` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `56516eff2da40e8733217d92420ea7763195ad6c` | direct_adapter_import_excluded_parity_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `48149cac08a81d168a7e594e1a1843d121de873a` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `4cbc0536ac8d672482068375340382cc78a033e1` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | represented | Open |
| `0735fdccb7d45991b313935e1d61af5406cf6dd5` | direct_adapter_import_excluded_parity_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `cc9c2f4c0f51902ad7b9427ed1309bce3f463db4` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `e93db6a95a341986a915f5c382e1f15c8a0745ee` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `f6afd6138e605365a76d065d04521ec2037bed3d` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `f4c7d2010d8f77bc4dd1e5ae62e2e5aa84e44c42` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | excluded | Open |
| `7a1e069b29503f1beffda40ee5b459bea3e0e83f` | direct_adapter_import_excluded_parity_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `619c6834807532b69c419454fd53074d6d10050a` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | missing | Open |
| `c2b68d40b544b352981099c3445401f9d71b33b7` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `b1100a499ad4afe8b965615e4464a66c941a7a5c` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved | Open |
| `a15d99e39fd9ea04722f064ed02dc726b13d2943` | direct_adapter_import_excluded_parity_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `c25c16206be0d2abf9faf8a7069ef72dc554116d` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | superseded | Open |
| `b795203025596562fcfcce08c052ce782b411464` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | partially represented | Open |
| `9427f435158662074d6576053a6675a403696543` | semantic_unresolved; native-merge:3c7427efb740934bf66653413b484118652ce649 | unresolved; no retry semantic review yet | Open |
| `9b4a3bc91e2f4d50e00deccf236c6b0db177f4eb` | semantic_unresolved; native-merge:6df1607255a208409581ef7b18b1fffb0bfd7c56 | unresolved | Open |
| `3979f12a749232e1f934949f58f68c83a8b66014` | semantic_unresolved; native-merge:094c89548baf352fae3fb1312e4f3b8462853a1e | unresolved | Open |
| `f8132b42ac9ee1fde464866b5f3b08939528f267` | semantic_unresolved; native-merge:fe85a6b2360839eee98fc6316e0ea58d38f15d60 | unresolved | Open |
| `3b173288feafebac3f0845f53e481f69adcd70b1` | semantic_unresolved; native-merge:fe85a6b2360839eee98fc6316e0ea58d38f15d60 | unresolved | Open |
| `c4bec4a6b38ed66ccd97df06490e051d3d9d81cd` | direct_adapter_import_excluded_parity_unresolved; native-merge:86c031a6618fda670318469783f25f305d00a0f4 | unresolved; no retry semantic review yet | Open |
| `b7e18c8940d2bb58d85339d27da0754e5aba92e5` | semantic_unresolved; native-merge:7b0f553cbae565247aa858a4aba80b194305e7f5 | unresolved; no retry semantic review yet | Open |
| `0365ef94173f22677e31de98ea62dc0d00415669` | semantic_unresolved; native-merge:4efedf257b27f2e144937e0b1de929ea1d4ab34d | missing | Open |
| `3b9052c955a9aafbb3baff874a93bba19afeaace` | semantic_unresolved; native-merge:745794df80fa4a3fa816213b7e7a6d0fa3efe9eb | unresolved | Open |
| `a08b9ecbbda54793275cbd4e93fecf0ad4ff1c0a` | semantic_unresolved; native-merge:7d74337efd05c761026dc3f58a3d9f2ff1bc0bc2 | unresolved | Open |
| `24f54c0eb64c516941abee2ad0d7de758b34ac86` | semantic_unresolved; native-merge:2b030a5b60be18602fdca48863e0cfe5debc5f6f | represented | Open |
| `874a972fb7c4eb06e739dce0a390fffb85d31f83` | semantic_unresolved; native-merge:e2d538afd00ed66a400c9ebc9e8200ada50700e1 | unresolved; no retry semantic review yet | Open |
| `c1b9c451c7f124936e635835ca4dd0cf8bf7d5b3` | semantic_unresolved; windows-session-roots | unresolved; no retry semantic review yet | Open |
| `9846c3f1711d557c78426913efaff340e37feb4e` | semantic_unresolved; windows-session-roots | unresolved; no retry semantic review yet | Open |
| `3007dbf8426764a92ace4da111836b86bec6b508` | semantic_unresolved; native-merge:a6aea0bc0703237efb42df58fcf4b9ebd7cf7094 | unresolved; no retry semantic review yet | Open |
| `6db22aadb6e5a92b9b193608e2ac3f420e461ec3` | semantic_unresolved; native-merge:994990893c28a1db657fb7c558ab317d77b0d66f | represented | Open |
| `cb167b9dcc48f215ed2ffd29d9b4b296eaa1e281` | semantic_unresolved; coverage-ranges | unresolved; no retry semantic review yet | Open |
| `a953509275a4fa42a907f88852d62a613f27bc56` | semantic_unresolved; coverage-ranges | partially represented | Open |
| `219b348263cf973c30969269f967e293be5e080e` | semantic_unresolved; coverage-ranges | unresolved | Open |
| `f871178b8c92d208cc94e7e0b41c1f312ac65f2d` | semantic_unresolved; coverage-ranges | unresolved; no retry semantic review yet | Open |
| `b04f54506ab42eddcbd89232a0d47d8355edd596` | semantic_unresolved; coverage-ranges | unresolved; no retry semantic review yet | Open |
| `e5c17de6217ec5938d18d8c51e2c0d7ead116a19` | semantic_unresolved; coverage-ranges | unresolved; no retry semantic review yet | Open |
| `04ba2fa368db12a80091315bfa649626a36145e1` | represented_source_only; cypher-variable-capacity | unresolved | Open |
| `98899ba5d62c9a66d038b24f3857b137d03d44b0` | represented_source_only; cypher-variable-capacity | unresolved | Open |
| `77a0c7d94156a43f19047330c7356be4ee12cba5` | represented_source_only; semantic-empty-sort | unresolved | Open |
| `92725a5eeac0345bc0c4cb52ed88a38a7103ab37` | semantic_unresolved; native-merge:5e5405c6e65602f23fd2b7926a134d9e0389bc2e | unresolved | Open |
| `a3500040e69af8284e3cac1398e00ae3d00a0524` | semantic_unresolved; native-merge:5e5405c6e65602f23fd2b7926a134d9e0389bc2e | unresolved | Open |
| `8e70590d16314640f6440e6b87b802411c6c9d2a` | semantic_unresolved; native-merge:760c08f453aac6cac0f1bba42479864226172a2c | unresolved; no retry semantic review yet | Open |
| `088fedb2d25b869621fd93db05b2f3aa69e30e65` | semantic_unresolved; native-merge:760c08f453aac6cac0f1bba42479864226172a2c | unresolved; no retry semantic review yet | Open |
| `e410c86f270a013d79639577d82009698b207e8b` | semantic_unresolved; native-merge:29dbf0bfa1500a3947c141edb79dbfefa285bb76 | unresolved | Open |
| `4cafe8eb22d1363a73f7cd9d5921e88c693cd78b` | semantic_unresolved; native-merge:c43ea2ac9d7f3ce7c246789bc0f9f81bc1f78c85 | unresolved; no retry semantic review yet | Open |
| `4c1b23477b8e0de27feee74eac11c21a1fd64151` | semantic_unresolved; native-merge:c43ea2ac9d7f3ce7c246789bc0f9f81bc1f78c85 | unresolved; no retry semantic review yet | Open |
| `6b23078c9da97affd012c1f578c73b633ea18801` | semantic_unresolved; csharp-stacked-attributes | unresolved; no retry semantic review yet | Open |
| `c4224cf3ef5b5e31612f9088f9786d27727b5d71` | semantic_unresolved; csharp-stacked-attributes | unresolved | Open |
| `7219a24cc692500cdeb5cc192adbd47ac8bb6ee1` | semantic_unresolved; native-merge:161df2bdb1f4e28734f326a73f6e7989418f1288 | unresolved; no retry semantic review yet | Open |
| `9e1faa7327f8192101f16d8d6d81020937475166` | semantic_unresolved; native-merge:6741c29fc24d72ad4642a78335d05e32963df06e | unresolved; no retry semantic review yet | Open |
| `d3eb7e9823b43a8e2e5917d2d234367c1f88436d` | semantic_unresolved; native-merge:b5185d10c4122da345d969464d936fb26f591c72 | unresolved | Open |
| `699be6c2468801435091e8ab5ecff3375e5cce1d` | semantic_unresolved; native-merge:1d6653847499e76f45f8dfcb60192ee0f43c8ae7 | unresolved | Open |
| `d72250a9eda02c39749e10c7d4ba54fefba3da38` | semantic_unresolved; native-merge:1d6653847499e76f45f8dfcb60192ee0f43c8ae7 | unresolved; no retry semantic review yet | Open |
| `21591d51168d26f931f64d62edc36013cd1f6249` | semantic_unresolved; native-merge:1d6653847499e76f45f8dfcb60192ee0f43c8ae7 | partially represented | Open |
| `8135be28a764fad4ee55abe466762bbda7ebab74` | semantic_unresolved; native-merge:1db8bace03140f5793ff9205e5281732e77c2bea | unresolved; no retry semantic review yet | Open |
| `4fa11d1ab0b5807a7eaed645a6e3675f0663def8` | semantic_unresolved; native-merge:a0dc4183cbe523d343764e3b93b71ab7457d6733 | unresolved; no retry semantic review yet | Open |
| `8134712dc43b159fd863244ce53ba23641f20f8c` | semantic_unresolved; native-merge:48f2698f7664dc018884ee8c2874521f8e0d3bdf | unresolved | Open |
| `3a8b8e82120872a56ca4d0157260569d90abe85c` | semantic_unresolved; native-merge:cc6743e9f32a51399977d7e6cb6fb85a9a5db29d | unresolved; no retry semantic review yet | Open |
| `8622d6a0ed4935aedaa336d216d8b6b269732de5` | semantic_unresolved; native-merge:cc6743e9f32a51399977d7e6cb6fb85a9a5db29d | unresolved | Open |
| `3c854ae217c4c5d2f48d8aaeb136541bb0d8b52a` | semantic_unresolved; native-merge:cc6743e9f32a51399977d7e6cb6fb85a9a5db29d | unresolved | Open |
| `8d46d2586255fc85e35f00ff8b814424b140e623` | semantic_unresolved; native-merge:5b6a30dee6e9ddfd1ed304614b4b32fec7f3e05e | unresolved; no retry semantic review yet | Open |
| `557cb0102863e66cbd5cc622509a29a4964d1831` | semantic_unresolved; native-merge:cdff08f3138ce39f252d97eadc7c9890265d3f90 | unresolved | Open |
| `8116672a7e3a02aecb5cb0e6fd9e685cd5066fe1` | semantic_unresolved; native-merge:055fbb7d8213526bf86085a420110d1040132611 | unresolved; no retry semantic review yet | Open |
| `6091db7104bcaa0b61566272290e04ce448851a7` | semantic_unresolved; native-merge:c725453648d3ad482242e8902f44e9076be5734e | unresolved; no retry semantic review yet | Open |
| `208ee99c86631d2af810b212d69f8a2db5f1c93e` | direct_adapter_import_excluded_parity_unresolved; native-merge:713764bd0b0d202029c4c2823797b2e87c4934ac | unresolved; no retry semantic review yet | Open |
| `3a4160b4610f83ea26daf69f1ffe7cb898bf48ad` | semantic_unresolved; native-merge:713764bd0b0d202029c4c2823797b2e87c4934ac | unresolved; no retry semantic review yet | Open |
| `7e73b48ef155a0e14a6e058be56287232e3976ea` | semantic_unresolved; native-merge:713764bd0b0d202029c4c2823797b2e87c4934ac | unresolved; no retry semantic review yet | Open |
| `e45d705105a63f2fd9b35ab9e37ffbc640be8231` | direct_adapter_import_excluded_parity_unresolved; native-merge:713764bd0b0d202029c4c2823797b2e87c4934ac | unresolved; no retry semantic review yet | Open |
| `9ef7da8cbfba8de5526ccd05e7196f5fa73af8f1` | direct_adapter_import_excluded_parity_unresolved; native-merge:713764bd0b0d202029c4c2823797b2e87c4934ac | unresolved; no retry semantic review yet | Open |
| `f6a29270b72a5373e4d949ed3f4078a40830c57d` | direct_adapter_import_excluded_parity_unresolved; native-merge:713764bd0b0d202029c4c2823797b2e87c4934ac | unresolved; no retry semantic review yet | Open |
| `527016258e0da33ef883966f618609f4cf58abf8` | semantic_unresolved; native-merge:9b85dd3eafcbfd0e2106fed3ad025de3910c0a5b | unresolved; no retry semantic review yet | Open |
| `df579830f3941e5ebaef73dc87d183aee358fbcb` | semantic_unresolved; native-merge:1b067e3b042da824edb27bb173df41afb2d27beb | unresolved; no retry semantic review yet | Open |
| `618849512d01a3d14fcb704532911f9d02c23436` | semantic_unresolved; native-merge:1b067e3b042da824edb27bb173df41afb2d27beb | unresolved; no retry semantic review yet | Open |
| `33c4fd80ef02f380acc92d40fc83f0840a5a272f` | semantic_unresolved; native-merge:91bee4fb28c720c016cbadd5962817bf3f2cbd39 | unresolved; no retry semantic review yet | Open |
| `00060cec854dbd38d7864aeb95a2d8ba1baa8ab2` | semantic_unresolved; native-merge:91bee4fb28c720c016cbadd5962817bf3f2cbd39 | unresolved; no retry semantic review yet | Open |
| `0567dc70a097f36a953368978d6728b0a412b229` | semantic_unresolved; native-merge:91bee4fb28c720c016cbadd5962817bf3f2cbd39 | unresolved; no retry semantic review yet | Open |
| `ecfb9642e153ffc656d0bc2344b17e9d454ff916` | semantic_unresolved; native-merge:91bee4fb28c720c016cbadd5962817bf3f2cbd39 | unresolved; no retry semantic review yet | Open |
| `c572ddc4f3b619ce8725f5336feb49929a88a029` | semantic_unresolved; native-merge:b790be3d15d44f0d4629a2c97ddf76c104d80d22 | unresolved; no retry semantic review yet | Open |
| `e5aabdf86ea86ee27b92477b21aa8d6c754a674d` | semantic_unresolved; native-merge:b790be3d15d44f0d4629a2c97ddf76c104d80d22 | unresolved; no retry semantic review yet | Open |
| `7bf366f63b3fd5211cec19e483ee2ed27145ca14` | semantic_unresolved; native-merge:b790be3d15d44f0d4629a2c97ddf76c104d80d22 | unresolved; no retry semantic review yet | Open |
| `a3c24a71b4c28e805b912f9f928b6b0b151f9495` | semantic_unresolved; native-merge:b790be3d15d44f0d4629a2c97ddf76c104d80d22 | unresolved; no retry semantic review yet | Open |
| `91c63be5d4e41d44e0746ef533af4a0b1967d6b3` | semantic_unresolved; native-merge:b790be3d15d44f0d4629a2c97ddf76c104d80d22 | unresolved; no retry semantic review yet | Open |
| `4ce6b115956d51e7a0e115187c29424188170d7c` | semantic_unresolved; native-merge:dd454b2346f2ef2054d1799a8b0bca6c5daae0dc | unresolved | Open |
| `0c4831a3643ba1545d1cf709f056b70739aac04f` | semantic_unresolved; native-merge:dd454b2346f2ef2054d1799a8b0bca6c5daae0dc | unresolved | Open |
| `b6458f924da7ac1dc1fc2d7969bc7a3f2ca4d248` | semantic_unresolved; native-merge:dd454b2346f2ef2054d1799a8b0bca6c5daae0dc | unresolved | Open |
| `7b4eb4a00ac600060be4836ee4e1bbfa7bbedf24` | semantic_unresolved; native-merge:dd454b2346f2ef2054d1799a8b0bca6c5daae0dc | unresolved | Open |
| `91e31211df9dd61459617d857d46b71699e34df8` | semantic_unresolved; native-merge:dd454b2346f2ef2054d1799a8b0bca6c5daae0dc | unresolved | Open |
| `aacf96a20e3b9c450ba968c8aae663da25598992` | semantic_unresolved; native-merge:042a58bb1f34f725b4e5c64f1e1f966c1b1130fc | unresolved | Open |
| `734906379f235c07cce960ce6a57746ea3be48bb` | semantic_unresolved; native-merge:042a58bb1f34f725b4e5c64f1e1f966c1b1130fc | unresolved; no retry semantic review yet | Open |
| `013e1f7ec7d4b981a1cd28bf35d5767c195f8447` | semantic_unresolved; native-merge:9da4d66df287cf82bbe42c1d7044c10b090c9f61 | unresolved; no retry semantic review yet | Open |
| `92cb3b9a4bebbc79bd283e447104669ca068ea64` | semantic_unresolved; native-merge:9da4d66df287cf82bbe42c1d7044c10b090c9f61 | unresolved; no retry semantic review yet | Open |
| `9a460b389ce3002ac164605c3627bb86aa3b9be3` | semantic_unresolved; native-merge:9da4d66df287cf82bbe42c1d7044c10b090c9f61 | unresolved | Open |
| `fa99ff64eb6579fb15f3aae1aff7da349cb2999a` | semantic_unresolved; native-merge:9da4d66df287cf82bbe42c1d7044c10b090c9f61 | unresolved; no retry semantic review yet | Open |
| `8c2692c1ee32284c813c05746e833dea5dfedfe0` | semantic_unresolved; native-merge:8d1f4b4c5e574fcf39c3d974149ee96f407ea8f5 | unresolved | Open |
| `3507d9555d73aa746cde23139588081c6c02d2e2` | semantic_unresolved; native-merge:9c6f45bda7a420a4275b911034e18b51396834d7 | unresolved; no retry semantic review yet | Open |
| `262ab01217312938728d7176f587e45adca16f01` | semantic_unresolved; native-merge:9c6f45bda7a420a4275b911034e18b51396834d7 | unresolved; no retry semantic review yet | Open |
| `e65bc6b71e32d20aeafb42c00b6097a0e2ded2c0` | adapter_only_excluded_source_reviewed; native-merge:9c6f45bda7a420a4275b911034e18b51396834d7 | source-reviewed: adapter-only, no CLI target | Closed — adapter-only (exact hunk reason in ADAPTER-LOW evidence) |
| `eeff3f19ff97d60af4f34298aa38a652fe0705b2` | adapter_only_excluded_source_reviewed; native-merge:9c6f45bda7a420a4275b911034e18b51396834d7 | source-reviewed: adapter-only, no CLI target | Closed — adapter-only (exact hunk reason in ADAPTER-LOW evidence) |
| `1ad52f5fb1b3d96255756de01ce30912b1899e09` | semantic_unresolved; native-merge:93486bca2257d051f57340efda54dee6dbb7e0ed | unresolved | Open |
| `09c0e88a6484c5fb696d7fc7b5a5e3f32b8cba68` | semantic_unresolved; native-merge:0b51555402aa6aea9e5fd93d9ed53136dec4c6e8 | unresolved | Open |
| `469c3dd996f23cdb629f1bbd6a751bc0f29a99f2` | semantic_unresolved; native-merge:339b3f4097aa6ede22fc382ab7fd320d93c498b8 | unresolved; no retry semantic review yet | Open |
| `f9038b84eb3bf82c2dff4cc4ea7f1ec1d8d13f01` | semantic_unresolved; native-merge:339b3f4097aa6ede22fc382ab7fd320d93c498b8 | unresolved; no retry semantic review yet | Open |
| `d21e6cc762a1816d8c274174c9bba01601b888ee` | semantic_unresolved; native-merge:339b3f4097aa6ede22fc382ab7fd320d93c498b8 | unresolved; no retry semantic review yet | Open |
| `085017ab2172c2f3d4cd3e27251033ecbc5b814b` | semantic_unresolved; native-merge:339b3f4097aa6ede22fc382ab7fd320d93c498b8 | unresolved; no retry semantic review yet | Open |
| `ac8f5b8a8137f15e95478be71ee6742e928ba5bc` | semantic_unresolved; native-merge:339b3f4097aa6ede22fc382ab7fd320d93c498b8 | unresolved | Open |
| `da3258f3c7e4bdad6ec23f9ba194729364bb1254` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `7fedfe85c63446d3695b75d463a9b8767655730f` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `b5f10c2214f7b4d34e71187214d51d4cfa7ba759` | shared_semantics_bounded_source_reviewed; native-merge:2058d49a04b785315c9f5bb56b6e2365822b576b | source-reviewed partial: index-over-budget-reporting | Open — shared semantics pending exact CLI consumer/contract mapping |
| `7b77fd4834afe355cc4c7326ac3a55f0502f7264` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `a2f29b3fe73927d547fed0fe2e0f20d4738ad7c2` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `9724d903046626df0640ced218259cb58070e4e8` | semantic_unresolved; memory-core-budget-spill | unresolved; no retry semantic review yet | Open |
| `9e69cb7d9b7ad4b40fa5f8cbbb653dc70e108130` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `d89ccabef064661794269c0de726021736d72b51` | semantic_unresolved; memory-core-budget-spill | unresolved; no retry semantic review yet | Open |
| `d70b79417978a9f3ab0d1aa849879dbf049793cf` | semantic_unresolved; memory-core-budget-spill | unresolved; no retry semantic review yet | Open |
| `172787de9c18352bb29545da2426d9d61b260fc3` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `22ce867efd88dff9c219f789967ed2612329ee6d` | semantic_unresolved; native-merge:ce0b4b214e1e29f3ea642dc348f52a90d14abbc2 | unresolved | Open |
| `62fd80b842f00bd72e655d23c6afbfe12e199b6b` | semantic_unresolved; native-merge:59a05eb1bf9e11deb060d782cd7d3a29f2ae2866 | unresolved | Open |
| `81264814f162ebfbecfc609a951afa93aa43f8c9` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `632c19d904da691f33fce571d6879e34e59ce54e` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `959f4de50cca16bb0eb07308886f9600e6613130` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `f0a86ef2c2bc352a479162cca79d1aa64598f09e` | semantic_unresolved; native-merge:f1e51133e9ec0708756e0ca9311b6b0f3a8cca34 | unresolved | Open |
| `92e7e6a91374e9f89c74ca45e01f4e525e331837` | semantic_unresolved; native-merge:f1e51133e9ec0708756e0ca9311b6b0f3a8cca34 | unresolved | Open |
| `c3714f11339e7c312c5c313a065624e92c2eec56` | semantic_unresolved; memory-core-budget-spill | unresolved; no retry semantic review yet | Open |
| `9a2bdc8c1dfedd124db0076ebde7d2590ce78382` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `6b48097471e8d1a3e5cf16447304899a21e12d7a` | candidate_not_represented; route-order-and-service-completeness | unresolved | Open |
| `1d58385b513a414ead34544346e86549dda3c4b1` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `11cb476dff14194ba772f3dda6b9be640e3f6e0e` | semantic_unresolved; native-merge:f1e51133e9ec0708756e0ca9311b6b0f3a8cca34 | unresolved | Open |
| `2f33999044533b3e86d1d4de25e5b8d6ce58714d` | semantic_unresolved; native-merge:f1e51133e9ec0708756e0ca9311b6b0f3a8cca34 | unresolved | Open |
| `6f290e11d0f46c065b052db2ff70a7bbceede5dd` | semantic_unresolved; native-merge:f1e51133e9ec0708756e0ca9311b6b0f3a8cca34 | unresolved | Open |
| `c7ccb479b454680ce104e9bffdf143f68bd312c9` | represented_source_only; properties-parser-state | unresolved | Open |
| `8c1a9d61c27292bf7a52f10fb54186d4e24de28f` | semantic_unresolved; native-merge:f1e51133e9ec0708756e0ca9311b6b0f3a8cca34 | unresolved | Open |
| `dae1e7dff225d7e2a878c10cd0b00856da2bfd1a` | semantic_unresolved; native-merge:f1e51133e9ec0708756e0ca9311b6b0f3a8cca34 | unresolved | Open |
| `9db5217bdbb779212c5d066ed1a84f7ca6fc0282` | semantic_unresolved; native-merge:f1e51133e9ec0708756e0ca9311b6b0f3a8cca34 | unresolved | Open |
| `9db483402ea2fd4e0a5ff3bdd2b1fc50a7518ec9` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `f6281874bba9a78ac7543564ff8f76b5262daeef` | semantic_unresolved; native-merge:45c45be5be6ff7e47ef6074826e0560569981a88 | unresolved | Open |
| `243b405afeb3998707ea4d556f3178c8e8419cab` | semantic_unresolved; native-merge:c58eb61f04d5943b6a924ef19ff39de1099e09ee | unresolved | Open |
| `e8a46c16e233bff111092dff18985cca94890bcc` | represented_source_only; ts-awaited-generics | unresolved | Open |
| `6166934defd9bc5b687ab9c8de3779e71a844395` | represented_source_only; cypher-optional-count | unresolved | Open |
| `81a8a03c820cb8bccafd0ebac355eede21b5e8bf` | semantic_unresolved; native-merge:6cd1900643e883d0690fa2808a523c0bff3372c2 | unresolved | Open |
| `823e720982e1bce047a9243bd9b24d9688dadb72` | semantic_unresolved; native-merge:5aeebbac683b4dc6772c0a919f2f1cdd51674bd1 | unresolved | Open |
| `3d9a6cf5cb7fd39c6b5fb4dfa43011aa70f736ea` | semantic_unresolved; native-merge:d6082f2025d8f91366ec7a1fc8ddd427c8e9fa56 | unresolved | Open |
| `455e4fb42acfd18170695ad8b3d2c47d8239c7a5` | semantic_unresolved; native-merge:f347d59650acad3513d8b3a1d5993054e805c122 | unresolved | Open |
| `96a2fc35dfcf14662cee491430366f4e8d8b8149` | adapter_only_excluded_source_reviewed; native-merge:36f770842af8aa615e6c01e403643f6df62e7fbc | source-reviewed: adapter-only, no CLI target | Closed — adapter-only (exact hunk reason in ADAPTER-LOW evidence) |
| `00411ce7bc41995783bfb16aada673515585446d` | semantic_unresolved; native-merge:5b4be65b75cc701128b528eedf1120a7f5805842 | unresolved; no retry semantic review yet | Open |
| `581f0c74006beb3edd28babbebeda7e48b3fa1d6` | semantic_unresolved; native-merge:5b4be65b75cc701128b528eedf1120a7f5805842 | unresolved; no retry semantic review yet | Open |
| `3b7f2559213ee80f5cfe0e2329c418211e09749e` | semantic_unresolved; native-merge:5b4be65b75cc701128b528eedf1120a7f5805842 | unresolved; no retry semantic review yet | Open |
| `e0becdeb094dd5e898997a2add826a9f84befbd7` | represented_source_only; dotnet-xml-admission | unresolved | Open |
| `21ae02154a198265d7dbed13af4280aa2b1e5690` | semantic_unresolved; native-merge:92abefa3f57a94591a92bcecd6a6f102da373575 | unresolved | Open |
| `af3e003ad8e67cd1fe3b20dafaf4d33a4b351852` | semantic_unresolved; native-merge:def38f3ebcd6c00978373315c9785ec56270a2bf | unresolved; no retry semantic review yet | Open |
| `c1128db8187db2dac27a7ad14120fc09802c8018` | semantic_unresolved; native-merge:def38f3ebcd6c00978373315c9785ec56270a2bf | unresolved | Open |
| `f05893757bed0fe83e450b6dbc62cf1c0dc22045` | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef | unresolved; no retry semantic review yet | Open |
| `7a909466a7f0bab98af0004f311c8668f3cae44c` | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef | unresolved; no retry semantic review yet | Open |
| `8c30c7850975ed6a4cd0009e60d2679727ce9905` | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef | unresolved; no retry semantic review yet | Open |
| `a378d47e7b4b14ad679b57d892ff98e9705d4f23` | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef | unresolved; no retry semantic review yet | Open |
| `004c555448e455172fd328ca9bdb1cdfef995ed2` | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef | unresolved; no retry semantic review yet | Open |
| `e026a60bb213b215caf2588455a8c885f0d82bc0` | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef | unresolved; no retry semantic review yet | Open |
| `88269ac2ee70ab88f3ee8d84bd509399e3a8f3d6` | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef | unresolved; no retry semantic review yet | Open |
| `d402623c20e1dbff62e560a865fd4955798dfbfe` | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef | unresolved | Open |
| `fcf451c6a6dda768dfec5a8f61a01ac1e1cfa6bf` | semantic_unresolved; native-merge:c7488b031ce95a517ca91dac1df6ddab30718189 | unresolved | Open |
| `25d08258e567ec417bf7b18b14a63e30162fc7da` | semantic_unresolved; native-merge:b82cf7b1a305b043839e5321e7ffa8f2400ded21 | unresolved | Open |
| `460ebefc87df4dd5142bdc339702edd4feec4696` | candidate_not_represented; atomic-publish-diagnostics | unresolved; no retry semantic review yet | Open |
| `1a3425536be95f0d2db7b932edd6d5f19143aa09` | semantic_unresolved; graph-quality-omnibus-needs-split | unresolved | Open |
| `24afb562dc6551072a611fc7ab7922ab2a8f6ab3` | semantic_unresolved; graph-quality-omnibus-needs-split | unresolved | Open |
| `df83d25ace9885e252363cb04be25a17da85c9e8` | semantic_unresolved; native-merge:bf4c476d5e18621d4424bc3eff322b1c7a678eeb | unresolved; no retry semantic review yet | Open |
| `da4bd25a01b612517512ee628bf0ec652000e80a` | semantic_unresolved; native-merge:5c69c149f9218ea3412945e82454792d0d016425 | unresolved; no retry semantic review yet | Open |
| `eb80a4ecff04d8bfacef20e1aa2ee29445a782ff` | semantic_unresolved; native-merge:28b4093c999c932b44a63b6b2f4036ccac719d60 | unresolved | Open |
| `5cb90f64adaf9c5a38b591971aa7c09e3d9e958c` | semantic_unresolved; native-merge:9b7d63f5796c0e39358c724b814f8fdbeef204b7 | unresolved | Open |
| `f428d583008ce87281bac138c02bd4375c56d735` | semantic_unresolved; native-merge:d1b2548ee81a3e335d7b54557e523090d557ea47 | unresolved | Open |
| `58c1c0caa7061ae868305dbe59a6c5e67004849d` | semantic_unresolved; native-merge:8b57fe2a2b141df72228fb2d3a66bc7a190a23c2 | unresolved | Open |
| `c067c771b9f93d08a3e614e68aee7d710d2f256c` | semantic_unresolved; native-merge:8b57fe2a2b141df72228fb2d3a66bc7a190a23c2 | unresolved | Open |
| `a262a35724b3246353f07b5de96ec4731681dd4b` | semantic_unresolved; native-merge:e783f73d752f83b689451e3a7e48061cda1202f7 | unresolved | Open |
| `65d60e72f5b86f5982b15f0d88826ba79a085aa2` | semantic_unresolved; native-merge:e783f73d752f83b689451e3a7e48061cda1202f7 | unresolved | Open |
| `aaa72fbe60ad341325e9c944e32cf67d98eca387` | semantic_unresolved; native-merge:95c91b8d8d0fc11a01b9109b43d0eea94393f538 | unresolved; no retry semantic review yet | Open |
| `2f557f53986976b2df1eb2a1a540c9d48cf91a8e` | semantic_unresolved; native-merge:95c91b8d8d0fc11a01b9109b43d0eea94393f538 | unresolved; no retry semantic review yet | Open |
| `7883507db6779a20bcfad398c8d6594f4206aa9b` | semantic_unresolved; native-merge:181c03dc188082ef67e1f27372afbf1117d44edb | unresolved | Open |
| `3604d6cf66c3d3099cec0b0b9a2f6da7f15e9e88` | semantic_unresolved; native-merge:b0691e0d2fd281b272f69eff970e157412e95bac | unresolved; no retry semantic review yet | Open |
| `343d7dd5e47b817999098605370f0eab706675f0` | candidate_not_represented; c-declared-return-type | unresolved | Open |
| `a041794f8e51ddf42342899be7b2ac26f951d777` | semantic_unresolved; native-merge:5c0b2418779ea74baacb8a74374d8dbd4f8beff3 | unresolved; no retry semantic review yet | Open |
| `c314dea4c1409a3a68fb33706b49e272809b5079` | semantic_unresolved; native-merge:5958f546dc0f9af9b856f4bbc1e709382ba8425f | unresolved; no retry semantic review yet | Open |
| `e8dcb28ba719e4272f605f3b961422bb1f3c3fad` | semantic_unresolved; native-merge:a5550e1a9eaa7487b79d02a048eb82b079771c60 | unresolved; no retry semantic review yet | Open |
| `183cd727be5968fec3ce43176ebb1fa255f8976b` | semantic_unresolved; native-merge:c98323bff7e92a3bbefbffd065589a0bf7a4b118 | unresolved | Open |
| `fa138acf44525ef94cfd6e16772d601da5623aef` | semantic_unresolved; native-merge:c98323bff7e92a3bbefbffd065589a0bf7a4b118 | unresolved | Open |
| `dadc57dbfa8615ce897c338f6af8d6c6eb1674b1` | semantic_unresolved; native-merge:8ab13c94f6030cf4bb7eb2c7280fbe85ee740a83 | unresolved | Open |
| `cda5e3c9e5c4d2251467ced00ff305205c368d17` | semantic_unresolved; native-merge:8ab13c94f6030cf4bb7eb2c7280fbe85ee740a83 | unresolved | Open |
| `f3bdf11a039b8ed8d685b04ddecedaac1e409167` | semantic_unresolved; doclinks | unresolved | Open |
| `6ae6cd4c3f280cb8e3ffec2858269055e390ffec` | semantic_unresolved; doclinks | unresolved | Open |
| `9a9534cbc45d40afa4e8bf070030a806acea1f5f` | semantic_unresolved; doclinks | unresolved | Open |
| `1d2bbc425988eb78fcd966dc8c9cfbdb814f0685` | semantic_unresolved; native-merge:64c23fab6c3eba3de226355c0ca1ff750116edab | unresolved; no retry semantic review yet | Open |
| `452485f73e488c86e01c90b6cf3935971e723792` | semantic_unresolved; route-decorator-paths | unresolved | Open |
| `d4931241adaef732066afb5ebf1cb225e2474403` | semantic_unresolved; route-decorator-paths | unresolved; no retry semantic review yet | Open |
| `246e4190c3086d1074adbe8afe39bf4261db59b8` | semantic_unresolved; route-decorator-paths | unresolved | Open |
| `eccc615398a0fa7fa29fd4494a5db4b0be77ebb8` | semantic_unresolved; native-merge:11b662f9f7fba92012b872dd4fcaef7ee0c1300d | unresolved | Open |
| `3ab2994f3026a0bed2bb778cb52f58e82049f665` | semantic_unresolved; native-merge:11b662f9f7fba92012b872dd4fcaef7ee0c1300d | unresolved | Open |
| `f50434eddfde4bbec93b4cd4716e60e10d1006e4` | semantic_unresolved; native-merge:11b662f9f7fba92012b872dd4fcaef7ee0c1300d | unresolved | Open |
| `95fa63441bf80c9ed871fa090ba925b8b6106d83` | semantic_unresolved; native-merge:11b662f9f7fba92012b872dd4fcaef7ee0c1300d | unresolved | Open |
| `bc5b3dd216568c864b41303a674020a2d456ced9` | excluded_empty_tree_change; native-merge:11b662f9f7fba92012b872dd4fcaef7ee0c1300d | unresolved | Empty |
| `efb231285d188ffda16a58f7a5ce8e0e6448638f` | semantic_unresolved; native-merge:4f7fa84e3210fb86c5268efe24f30bb58afab96f | unresolved | Open |
| `bc960d87d663b7b4fc4c3ba558013c4609073762` | semantic_unresolved; native-merge:f7cfccfd0cce3f18347b8f7c498802adab452d85 | unresolved; no retry semantic review yet | Open |
| `2ea73059f894df564e21be409908c2b5d3d53b37` | semantic_unresolved; ui-unicode-git-paths | unresolved; no retry semantic review yet | Open |
| `64297ff3e8e142887fc36c933d57fd4d32924cc2` | semantic_unresolved; native-merge:5df8b0442c48fed654c6f11d94cb6f64f00fc368 | unresolved | Open |
| `ff8dd47011c43c622d3dc82b33be32b4493e4ea9` | semantic_unresolved; native-merge:5df8b0442c48fed654c6f11d94cb6f64f00fc368 | unresolved | Open |
| `5a9c72d8f8c65b38c66a29a34184b49edf3378ff` | regression_followup_unresolved; watcher-backoff | unresolved; no retry semantic review yet | Open |
| `07eb426c95386e299a6aa6adbf2b4eb9f272ff3b` | regression_followup_unresolved; watcher-backoff | unresolved; no retry semantic review yet | Open |
| `77ab4bea87008547acee60435a8e3df29d90a5cb` | represented_source_only; ast-walker-growth | unresolved | Open |
| `ed991154f212928df63ce5a5cd1159cfc8196f00` | semantic_unresolved; native-merge:9d65f4cb6257d6b55c6052212a55caa555e8297d | unresolved | Open |
| `8f803f7262d1d1e58068f677f3e365048050b728` | semantic_unresolved; python-iris-class-calls | unresolved | Open |
| `075f40e9d4ccbfead4df0e199c7730f94c2154ff` | semantic_unresolved; python-iris-class-calls | unresolved | Open |
| `6f91a16fc2a753db248f122dea7e4b50763972d1` | semantic_unresolved; native-merge:66d9c2be8895e935f63f22979fe019562745e52b | unresolved; no retry semantic review yet | Open |
| `d7eba5a71d2a99d7b7bc588c666d22516ac0b563` | represented_source_only; persisted-lsp-linear-decode | unresolved | Open |
| `8093d10b812d9a7840ae99d8f9d24a04cb0f9559` | integration_in_progress; rescript-admission-termination | unresolved | Open |
| `ea6fa49fc2c209de69c1b0f393d9d2cc25b61419` | integration_in_progress; rescript-admission-termination | unresolved | Open |
| `e4780c208632b3c42488a3c9e315014ef4da157e` | represented_source_only; macro-coverage-scans | unresolved | Open |
| `c0a06b3d0a90513e0d80f79514b869f91cb958f1` | semantic_unresolved; sql-literal-values | unresolved | Open |
| `259dbb67bee1d3547ae61a98c9d0c1cf4e491d44` | required_deferred_followup; sql-literal-values | unresolved | Open |
| `19d144477b9558dad148d3f555ec6ede16caa3f5` | semantic_unresolved; native-merge:cb536768bedb28b490a8190e8b1296aacf197a69 | unresolved; no retry semantic review yet | Open |
| `dc735c01fbbb4eddbbb73ff4df6007ab68413a4d` | semantic_unresolved; native-merge:2278498371650c89aa16305ec06390b93d581d55 | unresolved; no retry semantic review yet | Open |
| `e9058e11db9ed94e401ad711b2016249970a9040` | semantic_unresolved; native-merge:8fa5eefcca9c22e556aa139f84aa30869fca5a76 | unresolved | Open |
| `5abd77f5d9580d75f8600914b46495be21e9a4a5` | semantic_unresolved; native-merge:13ce2c0cd5838c1f550d2a34af53ece6b82fb47e | unresolved | Open |
| `aec9bfe3751d8c1f2eb2cb48d8fe584b9d3fd973` | semantic_unresolved; native-merge:13ce2c0cd5838c1f550d2a34af53ece6b82fb47e | unresolved | Open |
| `fa2292111802b5f3b66db3e4dd168280b53ac17e` | mixed_shared_scope_unresolved; artifact-export-attribution | unresolved | Open |
| `7707c9d7918263cd446a4b54d4eb48c3fea91048` | represented_source_only; ast-walker-growth | unresolved | Open |
| `99c3bb4cbe4c0f9542bf722d29cbb5dd01397455` | represented_source_only; store-count-errors | unresolved; no retry semantic review yet | Open |
| `73c509f4246b0f97ca772b6d9a3159181a7fa6b6` | shared_semantics_bounded_source_reviewed; native-merge:5621a1c9c47fd4b9446cc88a9c6e13f503e42a44 | source-reviewed partial: unreadable-count-status | Open — shared semantics pending exact CLI consumer/contract mapping |
| `60449f895dbaedb4e74c4f155a8e214d78486e12` | adapter_only_excluded_source_reviewed; native-merge:5621a1c9c47fd4b9446cc88a9c6e13f503e42a44 | source-reviewed: adapter-only, no CLI target | Closed — adapter-only (exact hunk reason in ADAPTER-LOW evidence) |
| `629e9c2c1f87e2ac8b5d644d38ff4e935928752a` | adapter_only_excluded_source_reviewed; native-merge:2b34ebcbf137101dbb6433c699d262a076f3b105 | source-reviewed: adapter-only, no CLI target | Closed — adapter-only (exact hunk reason in ADAPTER-LOW evidence) |
| `8f3c15044b8200a15ee1895c5127f4bf117da493` | semantic_unresolved; native-merge:69fb951cd0c7f38808fc2b8b89bf3a9a4c5dc417 | unresolved; no retry semantic review yet | Open |
| `e171bd48b5e5845ec6f00db65ff300caac4f7304` | semantic_unresolved; native-merge:69fb951cd0c7f38808fc2b8b89bf3a9a4c5dc417 | unresolved; no retry semantic review yet | Open |
| `c8184ed94887a6b5e130726885b44a07c63a47b7` | candidate_not_represented; ancestor-ignore-chain | unresolved | Open |
| `282fbfcdabfec79c19c36f5bec5f75c22eb36df5` | required_candidate_followup; ancestor-ignore-chain | unresolved | Open |
| `f339aa625ffd71b936fe386250858e281e5b87ff` | semantic_unresolved; python-complete-binding-lookup | unresolved; no retry semantic review yet | Open |
| `df8dd1011bd1987d0ca5a45f324775aad2411f54` | semantic_unresolved; python-complete-binding-lookup | unresolved; no retry semantic review yet | Open |
| `6d508350c7713e10e0e935625ad74d8401fe89e2` | semantic_unresolved; native-merge:aa53278b29b5abd32e4c326fafa68db7495c555f | unresolved | Open |
| `b7bcb15c219761f27ff146063a50481303527563` | adapter_only_excluded_source_reviewed; native-merge:aa53278b29b5abd32e4c326fafa68db7495c555f | source-reviewed: adapter-only, no CLI target | Closed — adapter-only (exact hunk reason in ADAPTER-LOW evidence) |
| `3faecc78144175a81acf76721c4f695cf80b5676` | semantic_unresolved; native-merge:2bed1c8c4a5590c0ed1239f934101a81791bcffe | unresolved | Open |
| `c6ccdd9395dbbafc6f66a49040b478b445387946` | semantic_unresolved; native-merge:f5ecb5f739c7c9d5bb7de209553e3bfa9fd1041c | unresolved | Open |
| `19f50de3d2e2bdea5f9054caf07af70f46d74244` | semantic_unresolved; native-merge:e9a3d18fbdf4e4f91f06561dcc45606aab838359 | unresolved; no retry semantic review yet | Open |
| `d56e046ddd08ac6bf8b14f5049a88382a10a1636` | semantic_unresolved; native-merge:0cff5dbe7c396e9a52355f69aaec3e38f626a323 | unresolved | Open |
| `a8c4040e8a92dcf831c64f57458694ff8fa6e9f9` | semantic_unresolved; native-merge:09819ba144c6eaa178705681e76378be7e738745 | unresolved; no retry semantic review yet | Open |
| `e7324bb6904c6d0a295b956e83c8585219198358` | candidate_not_represented; sqlite-interior-pages | unresolved; no retry semantic review yet | Open |
| `a0c71225f7c0c5e4bee36ef235f966866abffb24` | semantic_unresolved; native-merge:7821a1babbdc29dd73e114a98b7f0a1a80cdb336 | unresolved | Open |
| `6b7628bb6bbf041a8bdf069dd9d6bc8861d84e73` | semantic_unresolved; native-merge:7821a1babbdc29dd73e114a98b7f0a1a80cdb336 | unresolved | Open |
| `357d71e198ae3cd4614b9e06e54957423291e5c4` | shared_semantics_bounded_source_reviewed; native-merge:e2f9fcc399e431288ad9ff0f00a8aac2560a81a8 | source-reviewed partial: detect-changes-project-relative-path | Open — shared semantics pending exact CLI consumer/contract mapping |
| `87471e1f1d6a01ca8be6ca45b968145d0de95362` | shared_semantics_bounded_source_reviewed; native-merge:e2f9fcc399e431288ad9ff0f00a8aac2560a81a8 | source-reviewed partial: detect-changes-project-relative-path | Open — shared semantics pending exact CLI consumer/contract mapping |
| `b9ad9ed1449540e06e9f814ded548d9f2bf52f49` | semantic_unresolved; axios-client-base-url | unresolved | Open |
| `1d5ff68e508e78b149589396d547166ad41362e0` | semantic_unresolved; axios-client-base-url | unresolved | Open |
| `793e0b6127db3c8333b1dc29faf0b35167e3ceda` | semantic_unresolved; native-merge:78795ad76a5a1fbd16ce50145da05988ef3b7590 | unresolved | Open |
| `1c73f8676c31e227c6ed478ccbb55662b34d6db2` | semantic_unresolved; native-merge:30a410b3985f120b9d6a1c0ccb7b97fa19283b30 | unresolved; no retry semantic review yet | Open |
| `dad40ccdb633c10557baba4049aec35049fa3b15` | semantic_unresolved; native-merge:9df483f0aff0efa93a61443a6731af957c4049d9 | unresolved; no retry semantic review yet | Open |
| `40dad610eec783fe243e6b6141c1bebc75c23f0d` | semantic_unresolved; git-child-environment | unresolved | Open |
| `45f0c430d28cf188753f44536d2cac26564a04ba` | semantic_unresolved; native-merge:6746f435497bca057444c908e5158f9790340073 | unresolved | Open |
| `951b2fddf68a6e6605b1664bd3252a2a6578ef35` | semantic_unresolved; native-merge:372aff2b5ceb229be3983a6432424ba7c120aca6 | unresolved | Open |
| `c64ea20feae160d1eeddadf09e7e0df36b08e818` | semantic_unresolved; native-merge:368f33f9e0a2c2793587dd9f16536f27a5bfc0ed | unresolved; no retry semantic review yet | Open |
| `3ef7d8cdc201115dd76cc373ea2c64a567a2defe` | semantic_unresolved; native-merge:1ff6299976220bcbb064d869173661d4f7ee7eb9 | unresolved | Open |
| `ae578bc1cc32bb8f159a69c486f938ad745c643f` | semantic_unresolved; native-merge:029919c6603e323d58c5caef731fa533cf917bd6 | unresolved; no retry semantic review yet | Open |
| `28b41c35b71cb039988f39f72689dd657e30522d` | semantic_unresolved; native-merge:cb251ed4725652e2574ada680889fb38a69e0f4e | unresolved; no retry semantic review yet | Open |
| `f3754d6197b626d5432ac84b0363b0d3a5544c5c` | semantic_unresolved; native-merge:6d19350e3722995069e3d73fc3cf51271ceb70c6 | unresolved | Open |
| `26cbccd998819c428589156a82e167db061522ae` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `78a2ec399fec3dc28e61439a4bcd900b7c64c23f` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `95eaf01755d18c501fa8d1c5dae2255cf460b9a2` | semantic_unresolved; native-merge:39da5de00cce7ceb327acb2d41ee1da521f6a9ba | unresolved | Open |
| `fe3c285fdc63bd9abe027026edd7adffe5f62677` | shared_semantics_bounded_source_reviewed; native-merge:083bc99d806b68f3837b4554ad27e4eb52d59369 | source-reviewed partial: read-only-tool-metadata | Open — shared semantics pending exact CLI consumer/contract mapping |
| `460153204ba9833913e5245ca894f783c1d7229d` | integration_in_progress; internal-cache-stores | unresolved | Open |
| `a1b7c05685e18597f3461783d4bc98661f05427d` | semantic_unresolved; memory-core-budget-spill | unresolved | Open |
| `23d48336d3367fb27cbde54049b1414a2aea1b48` | semantic_unresolved; native-merge:218f0e5b78214ea9ce5753aef253c36cc67c71c4 | unresolved; no retry semantic review yet | Open |
| `4c6ecffdcdb8d78c0704f0d7e825f676ad77c240` | semantic_unresolved; native-merge:8b8cafb616b9e9fff06fdb521547ffd874013eb3 | unresolved; no retry semantic review yet | Open |
| `533eee07ece1d01ff2c88da55972c7720cd1604b` | semantic_unresolved; test-runtime-isolation | unresolved; no retry semantic review yet | Open |
| `0d783bcca4edde896d5716e8a4060c3100595973` | semantic_unresolved; test-runtime-isolation | unresolved | Open |
| `ed76cf8e88cfb1f26c82110e46a518c13577f788` | integration_in_progress; daemon-cache-ownership | unresolved; no retry semantic review yet | Open |
| `3f99b2ddd323a119a443eb4477cc762764a89207` | semantic_unresolved; native-merge:0be78a53a56e51506f0db3d0d6d9d0dbbccad621 | unresolved | Open |
| `155d147e789913c1786debeeef6ff48f2c41470e` | shared_semantics_bounded_source_reviewed; native-merge:3f0f49cd6caa84f68bae84a14151ff9ac0c18178 | source-reviewed partial: unnamed-root-owner | Open — shared semantics pending exact CLI consumer/contract mapping |
| `b94afb44217f24d3764d1f50e4366619fd8ab1a4` | adapter_only_excluded_source_reviewed; native-merge:3f0f49cd6caa84f68bae84a14151ff9ac0c18178 | source-reviewed: adapter-only, no CLI target | Closed — adapter-only (exact hunk reason in ADAPTER-LOW evidence) |
| `1f0daaa915251d44f5e41a0672f793203c6695dc` | shared_semantics_bounded_source_reviewed; native-merge:3f0f49cd6caa84f68bae84a14151ff9ac0c18178 | source-reviewed partial: unnamed-root-owner | Open — shared semantics pending exact CLI consumer/contract mapping |
| `364dd6a99b4323eda0fe6d0dec187bb8d586cfcb` | adapter_only_excluded_source_reviewed; native-merge:3f0f49cd6caa84f68bae84a14151ff9ac0c18178 | source-reviewed: adapter-only, no CLI target | Closed — adapter-only (exact hunk reason in ADAPTER-LOW evidence) |
| `41b1aeacfe128ccc9987fdb8f71c8bf1abf55678` | semantic_unresolved; native-merge:3f0f49cd6caa84f68bae84a14151ff9ac0c18178 | unresolved; no retry semantic review yet | Open |
| `39847957699ee56900cf62f6e9336c06989e49e3` | candidate_not_represented; ts-imported-class-receivers | unresolved | Open |
| `2fabaa2b877986bdededb4f4f78e9a31014c6fdf` | candidate_not_represented; cypher-optional-rel-buffer | unresolved | Open |
| `88d4f29d18d6f5b231c377196e499af61d2c0c4a` | semantic_unresolved; native-merge:f67243ff0e6e32d116e01a54398c48492edecf18 | unresolved | Open |
| `e81fd499f8622e03f181db8f8637bd87d24f7203` | semantic_unresolved; native-merge:28e24c6e5153a431c0835266aa802fa3fc60f76a | unresolved; no retry semantic review yet | Open |
| `1b79558975ba24dbcb4e8d0efa1ceac6965044ed` | semantic_unresolved; jsx-ampersand-grammar | unresolved | Open |
| `c5c13a878d080e5ad086993710fd1b6548464ff7` | semantic_unresolved; jsx-ampersand-grammar | unresolved | Open |
| `4c466fc4f95f5e13cb6f93724ba19a02b80ddc4d` | mixed_partial_representation; build-config-seams | unresolved | Open |
| `c86be38954cf8b01a0d6211cb0f48ee6e48ca8d8` | semantic_unresolved; native-merge:7d4a12cb0af73f33b012f9f8e887853d1cfe0142 | unresolved | Open |
| `5acd91a39e548c94fa4546684998b543b844c1a7` | candidate_not_represented; python-external-import-binding | unresolved | Open |
| `492ae135575aeb7a9ff10c8ddbb987a389c54e27` | required_candidate_followup; python-external-import-binding | unresolved | Open |
| `e548d971b3e471ca5a149201c39ba62ab1a72261` | shared_semantics_bounded_source_reviewed; native-merge:fc70534e81453db793a7250c25e3411a58df166f | source-reviewed partial: trace-path-qn-fallback-cleanup | Open — shared semantics pending exact CLI consumer/contract mapping |
| `a45a99c9c73dd54a4625d680da700f7cb0387eff` | semantic_unresolved; native-merge:cb7d937638f77fb67aacc39444f6a6709d217d51 | unresolved | Open |
| `cf72f1a657e64b7298946da015acba5e8a92dbe7` | semantic_unresolved; git-child-environment | unresolved | Open |

### All 311 differing actual merge commits

First-parent membership and constituent counts are inventory facts. Complete full-hash
parents, nested merges and constituent lists for **all 923 merges** are retained in
`merge-crosswalk.jsonl`; all **2,429 non-merges** are in `nonmerge-crosswalk.jsonl`.
Every merge below remains open for whole-result semantics. Empty combined diffs
never close that requirement. QA integration of a constituent does not close its
merge container.

| Full upstream merge hash | On upstream first-parent chain | Constituent non-merges / nested merges | Whole-result disposition / combined-diff signal |
|---|---|---:|---|
| `3a500bc8450ab78969d1d8cb4b12e55a479b68d7` | no | 11 / 10 | unresolved |
| `359f7360ddd39dc155637076e76a4e855c27611f` | no | 152 / 117 | unresolved |
| `5fbab7bb7332bd06aaa880653ddfb2696e648f90` | yes | 1 / 1 | unresolved |
| `98e386e58b753bcf63f67aeeacbfcb94961d035c` | no | 117 / 88 | unresolved; nonempty combined diff |
| `b44e58490f11e4ee895dd91c02067b6de59cdf96` | yes | 1 / 2 | unresolved |
| `1424934980d353f97c3664f07b2c1721792b7efa` | yes | 1 / 0 | unresolved; nonempty combined diff |
| `c895513f2d6821a3479ea4cbdcf8a39cee2f33ba` | yes | 1 / 0 | unresolved |
| `4d8ac9e1cea0d645a02f876f47536c54a5bfa406` | yes | 1 / 0 | unresolved; nonempty combined diff |
| `aefd2a1838db8e5c41750b0d4ddaf46a45255e85` | yes | 1 / 0 | unresolved |
| `620613ede6a983f04b7c1539a2ea4c65b34d42f6` | yes | 3 / 0 | unresolved |
| `963980317538dec5674010f68238f3dff317a59e` | yes | 1 / 0 | unresolved |
| `4bae9bd67f54d91557127449e2ae2adcc04e46e1` | yes | 1 / 0 | unresolved |
| `7910de591b23c0e5d71b923f9e9b38933ce50aa9` | yes | 1 / 0 | unresolved |
| `2a0e9b9b803cb0913b8bebeab12b0174f4c8e9cb` | yes | 1 / 0 | unresolved |
| `e6a34fcea01611ad515b8bb9a6dcac107ab59b63` | yes | 2 / 0 | unresolved |
| `01bf5f23e3e61353c4bc21575ec09ac6f40e46bf` | yes | 1 / 0 | unresolved |
| `192f29609a7e4ed6fc10166419b590645950589b` | yes | 1 / 0 | unresolved |
| `7fedaba331d8fbd241582f3ceeccd70af0ade149` | no | 128 / 98 | unresolved |
| `c38aa35c277395e71fc8fd2f904231e6d909b5f0` | yes | 1 / 1 | unresolved |
| `0e9ed75991826d787d96c8abaf196ad0fdbf9f70` | yes | 1 / 0 | unresolved |
| `db2f104a1f342fdcae6ac5abd0c08e9da36686ad` | yes | 1 / 0 | unresolved |
| `63f79365e3217d2ef7a3615567bb96627f3fd6b3` | no | 44 / 42 | unresolved; nonempty combined diff |
| `cc3e2638da372f70e53aba2e5bcd8d1555dd4f2b` | yes | 4 / 1 | unresolved |
| `6cca768816ee8fe8c1ed591d27c659fd01635a3f` | yes | 1 / 0 | unresolved |
| `f37a47eb01dcf1fa7c5e15e57d99e388abdd5482` | yes | 4 / 0 | unresolved |
| `bac8edc58dfc700d8e24dcd129fb7715e68febe7` | yes | 1 / 0 | unresolved |
| `5802ccdd77c9913f8643092839c698bad10b2c4a` | yes | 1 / 0 | unresolved |
| `951e02e2f998f324b616d7acf6c6bcab8b97c956` | yes | 1 / 0 | unresolved |
| `4f3412b78a5fa64ad01cb9ffedea603d101262ec` | no | 1 / 1 | unresolved |
| `40e5e373ab27bba3129405f346aa5937f9c93850` | no | 1 / 2 | unresolved |
| `2f9828d6de76aac7a15fa3fb1da83a1ae0839660` | yes | 2 / 2 | unresolved |
| `4d4af28672729a5c1326974684728c9ee150926a` | no | 14 / 9 | unresolved |
| `9d0c6a1f1af852c54bba74e61401b21e4f345a51` | yes | 1 / 1 | unresolved |
| `429141261daefc04fc94f35300f7dbd899ed28f2` | no | 14 / 9 | unresolved |
| `91a1723175f7efb56679f9ef02170951fffce780` | yes | 1 / 1 | unresolved |
| `380b1c5af1d5f3e94248752266031b3035080793` | no | 7 / 4 | unresolved |
| `9ed01a478ccf5a451f88275737a1e209938bbff8` | yes | 1 / 1 | unresolved |
| `2b6266bd3a1be28f2b85f0f44a27ef9e84d0e9d3` | no | 14 / 4 | unresolved |
| `3643ac36be08237dffa4d72b17ed9f31d0f4e3b4` | no | 1 / 1 | unresolved |
| `fc8da75993458c610fa9a44314e2e486593dc1e4` | no | 16 / 13 | unresolved; nonempty combined diff |
| `2a7f4427dc6680dd6365136f2204aaf5867d6603` | no | 18 / 12 | unresolved |
| `28d9892f951817b970f46ca081a60c9a45a502f7` | no | 1 / 1 | unresolved |
| `834e4f023115e685c39f0eed0258d8acd35dcfec` | no | 31 / 18 | unresolved |
| `dd74440200451ecf6559ac84ff3612dfde0244ff` | no | 6 / 4 | unresolved |
| `b3d7940f4ac770c01069fb48b5c81f922d8c79e4` | no | 11 / 10 | unresolved |
| `fb2e38f5bbd45a2504d9daee09fc4c57a8d7db7a` | no | 17 / 22 | unresolved |
| `b149247a0fd78d912ec906f44c2cf30bae7e5209` | no | 1 / 1 | unresolved |
| `cd8a5ed0d7c5674072f5d422b12302aa6424fda3` | no | 1 / 2 | unresolved |
| `3f08fac5139a4acd3f393e9ca78538c1ba8b94d7` | no | 1 / 3 | unresolved |
| `da09b73bc0c03f3909a0153d75b8479220c31e2c` | no | 22 / 18 | unresolved |
| `e79a7d73b45644d86ab3ca3c7af84aa04aebe75f` | no | 1 / 1 | unresolved |
| `bbecc9b9e92af203a03433e8cb6968e1e8d21cb2` | no | 4 / 1 | unresolved |
| `b64e2170009159a5111b1339312512f4e66b1b57` | no | 1 / 1 | unresolved |
| `f47cd47afe9e7e144f3c6d49df37c5b53d3d1cdb` | no | 2 / 2 | unresolved |
| `3bbabf187c405cefcfa855aa8ecfa11c5d32d807` | yes | 1 / 17 | unresolved |
| `44fa3aeb213c07ff75bcf4cc44f8ba9fbdfb04bf` | yes | 1 / 0 | unresolved |
| `c803525258e651f9ed3776aeade9cda4762b552a` | yes | 1 / 0 | unresolved |
| `b95b33af863f9e711a0c82afb620f8760089c03f` | yes | 1 / 0 | unresolved |
| `e63292a8d4036fc2618ecbee973bd6cccaaaffc4` | no | 14 / 9 | unresolved |
| `54a434743ea61147a8c27fc8a80f1fa6da5b46e3` | yes | 1 / 1 | unresolved |
| `74700067c48b967edc4ef723b658a0fbb04f291c` | no | 14 / 9 | unresolved |
| `8f64c36d85754293895f929801bdffc58cead730` | yes | 2 / 1 | unresolved |
| `184a00a442bc87a888aaf3f39b801a561e03432e` | yes | 1 / 0 | unresolved |
| `13b7d067018ceafcb04519675ade885ade76815a` | yes | 1 / 0 | unresolved |
| `89d259935278a7f24750ca4411334aec9071ea6c` | no | 15 / 11 | unresolved |
| `a215dac387cf778d72fc7f47795370034ef51a67` | no | 1 / 1 | unresolved |
| `d7163945b6f964a207ef6a034bd115006d66e996` | no | 15 / 13 | unresolved |
| `2efb7a6323f992d5c407cc85e37b6d167c184ca1` | no | 159 / 147 | unresolved |
| `8e1de0403eddd33589fd00f2af5d8ab90b600e0a` | yes | 3 / 4 | unresolved |
| `dc703dad4a706a0f2c8130a869922677dba09ae3` | no | 175 / 161 | unresolved |
| `ffb5ebe9e8ad2763fc6388c001ca8a16ca834d88` | yes | 1 / 1 | unresolved |
| `5c8ce584eac30b26cb98f53d4bf8c3e7d6b4b9c2` | yes | 1 / 0 | unresolved |
| `20c7dd3dbbba55770b82a99f6fcec6120f362873` | no | 156 / 155 | unresolved |
| `a1e1beb242e5c90a2f925688dee96d54b7cbb55c` | yes | 1 / 1 | unresolved |
| `d97ee2be28d678dd153327468e8b7527bec7a82a` | yes | 1 / 0 | unresolved |
| `202c8e42b56abe2f55025e660e6042c449106823` | yes | 1 / 0 | unresolved |
| `b6e4beb9b17ca12b65128f30d12e5bb051ea002c` | yes | 2 / 0 | unresolved |
| `b3d898e12fba86ce5582f87c05db729df2d71424` | yes | 2 / 0 | unresolved |
| `ad479b61e9f23c01606f42c7d44ca174f104a2c8` | no | 166 / 157 | unresolved |
| `ceba42728aa8e5a54acfe7dc8870a948c210badc` | yes | 1 / 1 | unresolved |
| `3c7427efb740934bf66653413b484118652ce649` | yes | 33 / 0 | unresolved |
| `6df1607255a208409581ef7b18b1fffb0bfd7c56` | yes | 1 / 0 | unresolved |
| `094c89548baf352fae3fb1312e4f3b8462853a1e` | yes | 1 / 0 | unresolved |
| `fe85a6b2360839eee98fc6316e0ea58d38f15d60` | yes | 2 / 0 | unresolved |
| `7b0f553cbae565247aa858a4aba80b194305e7f5` | yes | 1 / 0 | unresolved |
| `7da8bf15838c42f8e3f366fc2188ca63f2b61942` | no | 1 / 1 | unresolved |
| `86c031a6618fda670318469783f25f305d00a0f4` | yes | 1 / 1 | unresolved |
| `7fbaf02904363d38c88f9b749b4523a614feecd5` | no | 212 / 170 | unresolved |
| `eb3bd818b597b9c1f317906b350cbf323f2a4995` | no | 1 / 1 | unresolved |
| `4efedf257b27f2e144937e0b1de929ea1d4ab34d` | yes | 1 / 2 | unresolved |
| `f8951524252b71be8d8d3ea1f86d03eb97d6e9d5` | no | 1 / 1 | unresolved |
| `745794df80fa4a3fa816213b7e7a6d0fa3efe9eb` | yes | 1 / 1 | unresolved |
| `4f82a1e56f0eef435fed0eac5c8dd06b5b30c2fc` | no | 65 / 57 | unresolved |
| `e384f88f90d1f13ac95220f15de72e888a654c98` | no | 1 / 1 | unresolved |
| `9cb2e9dd1c9fd4e6b3cb9f939ec46264b05c75ab` | no | 3 / 7 | unresolved |
| `2b030a5b60be18602fdca48863e0cfe5debc5f6f` | yes | 1 / 3 | unresolved |
| `99fa463023fe63520d5a23c29f777b9d91d553b7` | no | 3 / 7 | unresolved |
| `e2d538afd00ed66a400c9ebc9e8200ada50700e1` | yes | 1 / 1 | unresolved |
| `971111c7a98c31a134ad03e616a02810cd83568b` | yes | 2 / 0 | unresolved |
| `306c02d44e9a464faf68640f183fa6e82fea0907` | no | 4 / 8 | unresolved |
| `a6aea0bc0703237efb42df58fcf4b9ebd7cf7094` | yes | 1 / 1 | unresolved |
| `994990893c28a1db657fb7c558ab317d77b0d66f` | yes | 1 / 0 | unresolved |
| `ddb6533cd9c16e8e2be12aa791521420fb427881` | no | 4 / 8 | unresolved |
| `aa44c28ea5ea82a5f811f0bace4f7857a68cac80` | yes | 5 / 1 | unresolved |
| `1b89228d90b32ddd46f494df8755f496f6aba030` | no | 11 / 12 | unresolved |
| `98b94a24a3c8e1cb8ae6b9b693c522c6a02a89c2` | yes | 1 / 0 | unresolved |
| `c8ea1cad0dd34e6f6f8c542014a1a22885f67ee3` | no | 3 / 7 | unresolved |
| `a143b0882d0457f2f56b2d734128f914325d555f` | no | 11 / 12 | unresolved |
| `421d9d2299c31f05a2f91ab34cce6e7347346946` | yes | 2 / 2 | unresolved |
| `8ec859bb89752587bb93138a9af69a361b79bf01` | yes | 1 / 0 | unresolved |
| `4ef5672c53777f9dd316510dacfa3b88e2166fe2` | no | 14 / 19 | unresolved |
| `5e5405c6e65602f23fd2b7926a134d9e0389bc2e` | yes | 2 / 1 | unresolved |
| `b49c7a736c0789069e5c2d07a301143e382af53c` | no | 153 / 131 | unresolved |
| `4d3d06ac2aba832ba774cad396a3f445b290f0b4` | no | 15 / 20 | unresolved |
| `760c08f453aac6cac0f1bba42479864226172a2c` | yes | 2 / 2 | unresolved |
| `dab58c0701d8806ddac2783949f1a478225f80bb` | no | 11 / 12 | unresolved; nonempty combined diff |
| `29dbf0bfa1500a3947c141edb79dbfefa285bb76` | yes | 1 / 1 | unresolved |
| `b51411753e9423f1c67f810ecf54f9fce903cefc` | no | 95 / 82 | unresolved; nonempty combined diff |
| `9a5e3f36a9580f76211300faa8b7ed5942169b47` | no | 1 / 1 | unresolved |
| `53f55ebe72870695c15ace98101df5af6eeef38e` | no | 3 / 7 | unresolved |
| `1d35def402f586b4c18bba41de33c881eda8cfa2` | no | 2 / 6 | unresolved |
| `76d0382f8f958238119c8e36bbdd204838d15db5` | no | 9 / 6 | unresolved |
| `c43ea2ac9d7f3ce7c246789bc0f9f81bc1f78c85` | yes | 2 / 5 | unresolved |
| `683d4bc312625a3d26819635673529ab02a0d0e1` | no | 233 / 190 | unresolved; nonempty combined diff |
| `b35f73123302d8d101323c3621e6fe755d4970c6` | no | 1 / 1 | unresolved |
| `bb21edbbbba442364b88598af97478a97e62243a` | no | 3 / 7 | unresolved |
| `fbe9eae11c237459da50a06ebd83f709d655d491` | no | 11 / 12 | unresolved |
| `5b8217d43f60c629cad2aa08708a575ec4a0962a` | yes | 2 / 4 | unresolved |
| `161df2bdb1f4e28734f326a73f6e7989418f1288` | yes | 1 / 0 | unresolved |
| `6741c29fc24d72ad4642a78335d05e32963df06e` | yes | 1 / 0 | unresolved |
| `b5185d10c4122da345d969464d936fb26f591c72` | yes | 1 / 0 | unresolved |
| `1d6653847499e76f45f8dfcb60192ee0f43c8ae7` | yes | 3 / 0 | unresolved |
| `1db8bace03140f5793ff9205e5281732e77c2bea` | yes | 1 / 0 | unresolved |
| `48f2698f7664dc018884ee8c2874521f8e0d3bdf` | yes | 1 / 0 | unresolved |
| `cc6743e9f32a51399977d7e6cb6fb85a9a5db29d` | yes | 3 / 0 | unresolved; nonempty combined diff |
| `5b6a30dee6e9ddfd1ed304614b4b32fec7f3e05e` | yes | 1 / 0 | unresolved |
| `cdff08f3138ce39f252d97eadc7c9890265d3f90` | yes | 1 / 0 | unresolved |
| `055fbb7d8213526bf86085a420110d1040132611` | yes | 1 / 0 | unresolved |
| `c725453648d3ad482242e8902f44e9076be5734e` | yes | 1 / 0 | unresolved |
| `713764bd0b0d202029c4c2823797b2e87c4934ac` | yes | 6 / 0 | unresolved |
| `9b85dd3eafcbfd0e2106fed3ad025de3910c0a5b` | yes | 1 / 0 | unresolved |
| `ca36316c6b34a6081de24d69699665a20b16de0e` | no | 15 / 8 | unresolved |
| `a0dc4183cbe523d343764e3b93b71ab7457d6733` | yes | 1 / 1 | unresolved |
| `62bafb76b380d3af1cd1d6228b4afea0082c2a7b` | no | 1 / 1 | unresolved; nonempty combined diff |
| `53700ee1c72f1a5c2551a83c74d20278e4ac664b` | no | 8 / 3 | unresolved |
| `1b067e3b042da824edb27bb173df41afb2d27beb` | yes | 2 / 2 | unresolved |
| `ca86fdb057b3d7f6d31e179ccbe64629fb1b0921` | no | 26 / 24 | unresolved; nonempty combined diff |
| `91bee4fb28c720c016cbadd5962817bf3f2cbd39` | yes | 4 / 1 | unresolved |
| `d40ed1a6bbc968bbddf90abd3b38e74a131a1b5a` | no | 5 / 12 | unresolved; nonempty combined diff |
| `33534653f10c052f2a5c7f1953756aeb10499be9` | no | 21 / 12 | unresolved; nonempty combined diff |
| `b790be3d15d44f0d4629a2c97ddf76c104d80d22` | yes | 5 / 2 | unresolved |
| `951b9ab022df391410528fe98769d091c203fc60` | no | 26 / 24 | unresolved |
| `dd454b2346f2ef2054d1799a8b0bca6c5daae0dc` | yes | 5 / 1 | unresolved |
| `042a58bb1f34f725b4e5c64f1e1f966c1b1130fc` | yes | 2 / 0 | unresolved |
| `78a6d19f1eaaa73054639ab0f14c372c53cd1bb3` | no | 21 / 12 | unresolved; nonempty combined diff |
| `9da4d66df287cf82bbe42c1d7044c10b090c9f61` | yes | 4 / 1 | unresolved; nonempty combined diff |
| `2b8bd0c065c1d2728994fddc1683a66f70745748` | no | 100 / 105 | unresolved; nonempty combined diff |
| `93111777ad028dcc1efb698d68e95c1dac4f6ed0` | no | 7 / 5 | unresolved |
| `9c6f45bda7a420a4275b911034e18b51396834d7` | yes | 4 / 2 | unresolved |
| `044fef7957d48a097b48f4ac3c505aa67de8164f` | no | 4 / 3 | unresolved |
| `8d1f4b4c5e574fcf39c3d974149ee96f407ea8f5` | yes | 1 / 1 | unresolved |
| `93486bca2257d051f57340efda54dee6dbb7e0ed` | yes | 1 / 0 | unresolved |
| `2b80b31306b3dbb9ac8bc0ad50372f724f1dd243` | no | 64 / 57 | unresolved |
| `7d74337efd05c761026dc3f58a3d9f2ff1bc0bc2` | yes | 1 / 2 | unresolved |
| `0b51555402aa6aea9e5fd93d9ed53136dec4c6e8` | yes | 1 / 0 | unresolved |
| `339b3f4097aa6ede22fc382ab7fd320d93c498b8` | yes | 5 / 0 | unresolved |
| `2058d49a04b785315c9f5bb56b6e2365822b576b` | yes | 9 / 0 | unresolved |
| `8972ea69c6ad94b1ef1d4ffbf0a92d78d2db1798` | yes | 1 / 0 | unresolved |
| `ce0b4b214e1e29f3ea642dc348f52a90d14abbc2` | yes | 1 / 0 | unresolved |
| `59a05eb1bf9e11deb060d782cd7d3a29f2ae2866` | yes | 1 / 0 | unresolved |
| `f1e51133e9ec0708756e0ca9311b6b0f3a8cca34` | yes | 17 / 0 | unresolved |
| `45c45be5be6ff7e47ef6074826e0560569981a88` | yes | 1 / 0 | unresolved |
| `c58eb61f04d5943b6a924ef19ff39de1099e09ee` | yes | 1 / 0 | unresolved |
| `60c307e8d53a9bcd5d188e8f31baf83c3a25f6a5` | yes | 1 / 0 | unresolved |
| `d1be39287dedcdbf31be107fe1dc1b17aa389721` | yes | 1 / 0 | unresolved |
| `6cd1900643e883d0690fa2808a523c0bff3372c2` | yes | 1 / 0 | unresolved |
| `5aeebbac683b4dc6772c0a919f2f1cdd51674bd1` | yes | 1 / 0 | unresolved; nonempty combined diff |
| `c6ff57df0c216dc0fc86442f111607d8b3bd7ef8` | no | 49 / 69 | unresolved |
| `d6082f2025d8f91366ec7a1fc8ddd427c8e9fa56` | yes | 1 / 1 | unresolved |
| `f347d59650acad3513d8b3a1d5993054e805c122` | yes | 1 / 0 | unresolved; nonempty combined diff |
| `36f770842af8aa615e6c01e403643f6df62e7fbc` | yes | 1 / 0 | unresolved |
| `d64c9f2afe6b2add648ee1e3eead48acda025579` | no | 5 / 1 | unresolved |
| `e176ae552b99f0fd08a2dde8bdf52025a3e20059` | no | 12 / 4 | unresolved |
| `5b4be65b75cc701128b528eedf1120a7f5805842` | yes | 3 / 2 | unresolved |
| `9b6c99e794487fec973bc1e64e1baebafdf42e4b` | yes | 1 / 0 | unresolved |
| `92abefa3f57a94591a92bcecd6a6f102da373575` | yes | 1 / 0 | unresolved |
| `def38f3ebcd6c00978373315c9785ec56270a2bf` | yes | 2 / 0 | unresolved |
| `e321b1fceaaf4c9f03fbe82547f02c1079e3b760` | no | 222 / 179 | unresolved; nonempty combined diff |
| `a7187ad25001edf1d6731973941a82da6f193846` | yes | 4 / 1 | unresolved |
| `b026a6907807f7a411e7da4de45d0cedc2e93dc9` | no | 117 / 88 | unresolved |
| `b435bd87b6d0e6a8da43c2964829927fff331f5c` | no | 49 / 69 | unresolved |
| `19d9e4ac4d0a8c6839b627c0cd5890188121fc2b` | no | 177 / 116 | unresolved |
| `9b1033790779aa6662d90b98c0c56d23272ae6ef` | yes | 8 / 3 | unresolved |
| `7d01d3ec031dc26759283fff96e979c3e0ce270e` | no | 2 / 1 | unresolved |
| `c7488b031ce95a517ca91dac1df6ddab30718189` | yes | 1 / 1 | unresolved |
| `2dba3cf81750f245d8c1d912997f04be5615df99` | no | 2 / 1 | unresolved |
| `b82cf7b1a305b043839e5321e7ffa8f2400ded21` | yes | 1 / 1 | unresolved |
| `6f2f83618134bee208c98c47931579f27a712809` | no | 2 / 1 | unresolved |
| `d615088a3cc74d0cc320203a622951333dd197a0` | yes | 1 / 1 | unresolved |
| `bf4c476d5e18621d4424bc3eff322b1c7a678eeb` | yes | 3 / 0 | unresolved |
| `28b4093c999c932b44a63b6b2f4036ccac719d60` | yes | 1 / 0 | unresolved |
| `9b7d63f5796c0e39358c724b814f8fdbeef204b7` | yes | 1 / 0 | unresolved |
| `d1b2548ee81a3e335d7b54557e523090d557ea47` | yes | 1 / 0 | unresolved |
| `8b57fe2a2b141df72228fb2d3a66bc7a190a23c2` | yes | 2 / 0 | unresolved |
| `e783f73d752f83b689451e3a7e48061cda1202f7` | yes | 2 / 0 | unresolved |
| `95c91b8d8d0fc11a01b9109b43d0eea94393f538` | yes | 2 / 0 | unresolved |
| `fbda6bacf78b47e4c498dfb44ae653f9d10a4add` | no | 187 / 177 | unresolved |
| `c0bfd073ff25484c2d7b3b59dea937be6c92ed08` | no | 195 / 129 | unresolved |
| `181c03dc188082ef67e1f27372afbf1117d44edb` | yes | 1 / 2 | unresolved |
| `e63bc6f06a03e93fad9e962c9b9aaf4b0309306e` | no | 187 / 177 | unresolved |
| `dfae618b0e9508cd4998b583ebbeb36cf458278e` | no | 195 / 129 | unresolved |
| `b0691e0d2fd281b272f69eff970e157412e95bac` | yes | 1 / 2 | unresolved |
| `1160fa3591ea2bbc20ab1f7b7f547466bb005a7d` | yes | 1 / 0 | unresolved |
| `5c0b2418779ea74baacb8a74374d8dbd4f8beff3` | yes | 1 / 0 | unresolved |
| `5958f546dc0f9af9b856f4bbc1e709382ba8425f` | yes | 1 / 0 | unresolved |
| `9b5bf70dc400fdfe989ed2bf1b623b2103bbf2f0` | no | 14 / 15 | unresolved |
| `5c69c149f9218ea3412945e82454792d0d016425` | yes | 1 / 1 | unresolved |
| `6115e5785dc01f5b403cbfe7f184ffbfba058e45` | no | 100 / 57 | unresolved |
| `92cafd1ec888362ef3223ad669a5aa4f378a54d5` | no | 32 / 28 | unresolved |
| `a5550e1a9eaa7487b79d02a048eb82b079771c60` | yes | 1 / 2 | unresolved |
| `f07c18702ac82957d0eb73f96f37d30184b7e6b5` | no | 32 / 28 | unresolved |
| `c98323bff7e92a3bbefbffd065589a0bf7a4b118` | yes | 2 / 1 | unresolved |
| `fcdc6bedb7d05489462639da38c4ac755137734f` | no | 34 / 29 | unresolved |
| `8ab13c94f6030cf4bb7eb2c7280fbe85ee740a83` | yes | 2 / 1 | unresolved |
| `da3ea3633911b5ac1240cfd7c73c5056db2c52ff` | yes | 3 / 0 | unresolved |
| `64c23fab6c3eba3de226355c0ca1ff750116edab` | yes | 1 / 0 | unresolved |
| `61cfb09b52ba40e8fb3ee9703c618b28739e3298` | no | 60 / 33 | unresolved |
| `cf7eecd7d0b1288c7f2e290a1d1bfc1b1b6f8698` | no | 17 / 16 | unresolved |
| `d772b1e5af0ce80d08aa6603ea47d1f51f324193` | no | 9 / 10 | unresolved |
| `4b0b5f258d047d08a107c16399a1cc0e98ac89a9` | yes | 3 / 3 | unresolved |
| `a6d3d5d820b07ba8544ddae1668e8107d7961532` | no | 32 / 28 | unresolved |
| `ff127ea44c56ce21bd5a4f9cf9f51bb27523d0aa` | no | 9 / 10 | unresolved |
| `11b662f9f7fba92012b872dd4fcaef7ee0c1300d` | yes | 5 / 2 | unresolved |
| `4f7fa84e3210fb86c5268efe24f30bb58afab96f` | yes | 1 / 0 | unresolved |
| `f7cfccfd0cce3f18347b8f7c498802adab452d85` | yes | 1 / 0 | unresolved |
| `3e67838b05e67915d472ec7698c80649ab8ad03a` | no | 225 / 164 | unresolved; nonempty combined diff |
| `6935df23a95ef3d85db1dd1670d45cb6bb562b20` | yes | 1 / 1 | unresolved |
| `5df8b0442c48fed654c6f11d94cb6f64f00fc368` | yes | 2 / 0 | unresolved |
| `b0fe8aef949e9e9a9e8bf3114d0ff33486695f69` | yes | 2 / 0 | unresolved |
| `f4654a7c772a57e97606414982d8cb6099661456` | yes | 1 / 0 | unresolved |
| `9d65f4cb6257d6b55c6052212a55caa555e8297d` | yes | 1 / 0 | unresolved |
| `64cf275ffa9aea01ce119f9cd7bc8f915fc1cfc5` | yes | 2 / 0 | unresolved |
| `262d7ca7659b3f1f6ee619cda2b5f6aad2815314` | no | 10 / 11 | unresolved |
| `66d9c2be8895e935f63f22979fe019562745e52b` | yes | 1 / 1 | unresolved |
| `1f9b4db12c745e1b4554f822667627aa09d4e6bd` | yes | 1 / 0 | unresolved |
| `d17f97f4313eb8134a127a6ecfd8f4089e44f028` | yes | 2 / 0 | unresolved |
| `d82e1e601491d1dca0ba8f529c5f8201bd8341dd` | yes | 3 / 0 | unresolved |
| `cb536768bedb28b490a8190e8b1296aacf197a69` | yes | 1 / 0 | unresolved |
| `2278498371650c89aa16305ec06390b93d581d55` | yes | 1 / 0 | unresolved |
| `8fa5eefcca9c22e556aa139f84aa30869fca5a76` | yes | 1 / 0 | unresolved |
| `8e29e1568f442ed9422e0fcd24d2594ae0ba7893` | no | 19 / 23 | unresolved; nonempty combined diff |
| `232764a12295a59f05619a2bd032c8f92af6f17d` | no | 66 / 61 | unresolved |
| `9130fab63d23b5386f12fa4cfa57961b247ff07b` | no | 113 / 80 | unresolved; nonempty combined diff |
| `13ce2c0cd5838c1f550d2a34af53ece6b82fb47e` | yes | 2 / 3 | unresolved |
| `2b4084ff0c562a2e15e6609173ad6036e77d2a98` | yes | 1 / 0 | unresolved |
| `17718126ea3214825b34d72482728f8c3c182c6f` | yes | 1 / 0 | unresolved |
| `d550dcbe73558bb773211a0ff9463daff91721fd` | no | 4 / 8 | unresolved |
| `8eb63ecfe79ea1f33fd911b8913a8b704ecde9d8` | no | 11 / 12 | unresolved |
| `87ba464596337fc6574a825ef88392e70a733dc9` | no | 178 / 140 | unresolved |
| `5621a1c9c47fd4b9446cc88a9c6e13f503e42a44` | yes | 3 / 3 | unresolved |
| `2b34ebcbf137101dbb6433c699d262a076f3b105` | yes | 1 / 0 | unresolved |
| `11957ae6554b7d768095dcda95168331bd654363` | no | 151 / 107 | unresolved |
| `69fb951cd0c7f38808fc2b8b89bf3a9a4c5dc417` | yes | 2 / 1 | unresolved |
| `65fc0bb4f77fdbb6449199c147e1ed112654b151` | no | 130 / 88 | unresolved |
| `82a6ddff81c6c8583c73f3d628037e41a9b412f0` | no | 21 / 19 | unresolved |
| `92910147aba1e56a1cb8fff7da5e74dbf8011b35` | yes | 2 / 2 | unresolved |
| `80eb92a7017dab9a0773430433660a966b80cc15` | yes | 2 / 0 | unresolved |
| `aa53278b29b5abd32e4c326fafa68db7495c555f` | yes | 2 / 0 | unresolved; nonempty combined diff |
| `2bed1c8c4a5590c0ed1239f934101a81791bcffe` | yes | 1 / 0 | unresolved |
| `f5ecb5f739c7c9d5bb7de209553e3bfa9fd1041c` | yes | 1 / 0 | unresolved |
| `e9a3d18fbdf4e4f91f06561dcc45606aab838359` | yes | 1 / 0 | unresolved |
| `0cff5dbe7c396e9a52355f69aaec3e38f626a323` | yes | 1 / 0 | unresolved |
| `09819ba144c6eaa178705681e76378be7e738745` | yes | 1 / 0 | unresolved |
| `6bce86383ea192a2f0fbf795e161d3ddca4a78cc` | yes | 1 / 0 | unresolved |
| `7821a1babbdc29dd73e114a98b7f0a1a80cdb336` | yes | 2 / 0 | unresolved |
| `e2f9fcc399e431288ad9ff0f00a8aac2560a81a8` | yes | 2 / 0 | unresolved |
| `5910c47a97c81c4c4c1367ba3e08a14917743822` | yes | 2 / 0 | unresolved |
| `78795ad76a5a1fbd16ce50145da05988ef3b7590` | yes | 1 / 0 | unresolved |
| `30a410b3985f120b9d6a1c0ccb7b97fa19283b30` | yes | 1 / 0 | unresolved |
| `9df483f0aff0efa93a61443a6731af957c4049d9` | yes | 1 / 0 | unresolved |
| `94097f0727e2bfe57aa8af54d65f2d3ed902d8a5` | yes | 1 / 0 | unresolved |
| `6746f435497bca057444c908e5158f9790340073` | yes | 1 / 0 | unresolved |
| `372aff2b5ceb229be3983a6432424ba7c120aca6` | yes | 1 / 0 | unresolved |
| `368f33f9e0a2c2793587dd9f16536f27a5bfc0ed` | yes | 1 / 0 | unresolved |
| `1ff6299976220bcbb064d869173661d4f7ee7eb9` | yes | 1 / 0 | unresolved |
| `029919c6603e323d58c5caef731fa533cf917bd6` | yes | 1 / 0 | unresolved |
| `cb251ed4725652e2574ada680889fb38a69e0f4e` | yes | 1 / 0 | unresolved; nonempty combined diff |
| `6d19350e3722995069e3d73fc3cf51271ceb70c6` | yes | 1 / 0 | unresolved |
| `d0ec2313d925708f09f4de39996abc8f3b462aa9` | yes | 2 / 0 | unresolved |
| `39da5de00cce7ceb327acb2d41ee1da521f6a9ba` | yes | 1 / 0 | unresolved |
| `083bc99d806b68f3837b4554ad27e4eb52d59369` | yes | 1 / 0 | unresolved |
| `48acb559e6c90ddd82aebce91f7d6147e4e2c889` | yes | 1 / 0 | unresolved |
| `0bee4c1b796e9beaba7d61d5591ca658e3bb2f40` | yes | 1 / 0 | unresolved |
| `218f0e5b78214ea9ce5753aef253c36cc67c71c4` | yes | 1 / 0 | unresolved |
| `8b8cafb616b9e9fff06fdb521547ffd874013eb3` | yes | 1 / 0 | unresolved |
| `f4ff01b10b23297209a199fdf4f571837d50ffee` | yes | 1 / 0 | unresolved |
| `878379a8afba61e774bcbd1a13b00516352cc9e7` | yes | 1 / 0 | unresolved |
| `0f52d30c2964a5a538149f8b39846ba52517a020` | yes | 1 / 0 | unresolved |
| `0be78a53a56e51506f0db3d0d6d9d0dbbccad621` | yes | 1 / 0 | unresolved |
| `3f0f49cd6caa84f68bae84a14151ff9ac0c18178` | yes | 5 / 0 | unresolved |
| `77ea3bb95c26f0e5ffde18a209d39073e9c333aa` | yes | 1 / 0 | unresolved |
| `ea7e01e91e64da62a74c1abbfe7a138710d1c88a` | yes | 1 / 0 | unresolved |
| `f67243ff0e6e32d116e01a54398c48492edecf18` | yes | 1 / 0 | unresolved |
| `28e24c6e5153a431c0835266aa802fa3fc60f76a` | yes | 1 / 0 | unresolved |
| `0bed0d372d6d5ccdc6c0bea45fb54f75c6aedb7d` | yes | 2 / 0 | unresolved |
| `c61b3806bfe4c25f7003728b21d5b96a0db20e8f` | yes | 1 / 0 | unresolved |
| `7d4a12cb0af73f33b012f9f8e887853d1cfe0142` | yes | 1 / 0 | unresolved |
| `bedbc0ac00c04f4a73ea92dcde93fbcfef6cbe8a` | yes | 2 / 0 | unresolved |
| `fc70534e81453db793a7250c25e3411a58df166f` | yes | 1 / 0 | unresolved |
| `cb7d937638f77fb67aacc39444f6a6709d217d51` | yes | 1 / 0 | unresolved |
| `96c3f41cf334d87670cb085f1fcf16f637293222` | yes | 1 / 0 | unresolved |

### Older canonical selected, deferred and excluded rows

Every old row is retained here with its full hash and current bounded disposition.
The legacy label is historical, not a renewed exclusion or acceptance decision.
The 190-token crosswalk also includes local implementation/QA references and is
retained in run evidence.

| Full hash | Legacy label | Current bounded disposition / group |
|---|---|---|
| `004c555448e455172fd328ca9bdb1cdfef995ed2` | excluded | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef |
| `075f40e9d4ccbfead4df0e199c7730f94c2154ff` | deferred | semantic_unresolved; python-iris-class-calls |
| `07eb426c95386e299a6aa6adbf2b4eb9f272ff3b` | excluded | regression_followup_unresolved; watcher-backoff |
| `086422020f5fd9396b46384b10413cdca6994c96` | selected | regression_followup_unresolved; watcher-backoff |
| `183cd727be5968fec3ce43176ebb1fa255f8976b` | excluded | semantic_unresolved; native-merge:c98323bff7e92a3bbefbffd065589a0bf7a4b118 |
| `19d144477b9558dad148d3f555ec6ede16caa3f5` | deferred | semantic_unresolved; native-merge:cb536768bedb28b490a8190e8b1296aacf197a69 |
| `1a3425536be95f0d2db7b932edd6d5f19143aa09` | deferred | semantic_unresolved; graph-quality-omnibus-needs-split |
| `1d2bbc425988eb78fcd966dc8c9cfbdb814f0685` | deferred | semantic_unresolved; native-merge:64c23fab6c3eba3de226355c0ca1ff750116edab |
| `246e4190c3086d1074adbe8afe39bf4261db59b8` | deferred | semantic_unresolved; route-decorator-paths |
| `24afb562dc6551072a611fc7ab7922ab2a8f6ab3` | deferred | semantic_unresolved; graph-quality-omnibus-needs-split |
| `259dbb67bee1d3547ae61a98c9d0c1cf4e491d44` | excluded | required_deferred_followup; sql-literal-values |
| `25d08258e567ec417bf7b18b14a63e30162fc7da` | deferred | semantic_unresolved; native-merge:b82cf7b1a305b043839e5321e7ffa8f2400ded21 |
| `282fbfcdabfec79c19c36f5bec5f75c22eb36df5` | selected | required_candidate_followup; ancestor-ignore-chain |
| `2ea73059f894df564e21be409908c2b5d3d53b37` | deferred | semantic_unresolved; ui-unicode-git-paths |
| `2f557f53986976b2df1eb2a1a540c9d48cf91a8e` | excluded | semantic_unresolved; native-merge:95c91b8d8d0fc11a01b9109b43d0eea94393f538 |
| `343d7dd5e47b817999098605370f0eab706675f0` | selected | candidate_not_represented; c-declared-return-type |
| `3604d6cf66c3d3099cec0b0b9a2f6da7f15e9e88` | excluded | semantic_unresolved; native-merge:b0691e0d2fd281b272f69eff970e157412e95bac |
| `3ab2994f3026a0bed2bb778cb52f58e82049f665` | excluded | semantic_unresolved; native-merge:11b662f9f7fba92012b872dd4fcaef7ee0c1300d |
| `452485f73e488c86e01c90b6cf3935971e723792` | deferred | semantic_unresolved; route-decorator-paths |
| `460ebefc87df4dd5142bdc339702edd4feec4696` | selected | candidate_not_represented; atomic-publish-diagnostics |
| `4fc5999a85f50bbb131dd87c5f41949c08617c1a` | selected | mixed_partial_representation; watcher-backoff |
| `58c1c0caa7061ae868305dbe59a6c5e67004849d` | excluded | semantic_unresolved; native-merge:8b57fe2a2b141df72228fb2d3a66bc7a190a23c2 |
| `5a9c72d8f8c65b38c66a29a34184b49edf3378ff` | excluded | regression_followup_unresolved; watcher-backoff |
| `5abd77f5d9580d75f8600914b46495be21e9a4a5` | excluded | semantic_unresolved; native-merge:13ce2c0cd5838c1f550d2a34af53ece6b82fb47e |
| `5cb90f64adaf9c5a38b591971aa7c09e3d9e958c` | deferred | semantic_unresolved; native-merge:9b7d63f5796c0e39358c724b814f8fdbeef204b7 |
| `60449f895dbaedb4e74c4f155a8e214d78486e12` | excluded | direct_adapter_import_excluded_parity_unresolved; native-merge:5621a1c9c47fd4b9446cc88a9c6e13f503e42a44 |
| `61ccd905111e6b9e0f262a7636d4af399e9be192` | selected | regression_followup_unresolved; watcher-backoff |
| `629e9c2c1f87e2ac8b5d644d38ff4e935928752a` | excluded | direct_adapter_import_excluded_parity_unresolved; native-merge:2b34ebcbf137101dbb6433c699d262a076f3b105 |
| `64297ff3e8e142887fc36c933d57fd4d32924cc2` | excluded | semantic_unresolved; native-merge:5df8b0442c48fed654c6f11d94cb6f64f00fc368 |
| `65d60e72f5b86f5982b15f0d88826ba79a085aa2` | excluded | semantic_unresolved; native-merge:e783f73d752f83b689451e3a7e48061cda1202f7 |
| `6ae6cd4c3f280cb8e3ffec2858269055e390ffec` | deferred | semantic_unresolved; doclinks |
| `6f91a16fc2a753db248f122dea7e4b50763972d1` | excluded | semantic_unresolved; native-merge:66d9c2be8895e935f63f22979fe019562745e52b |
| `73c509f4246b0f97ca772b6d9a3159181a7fa6b6` | excluded | direct_adapter_import_excluded_parity_unresolved; native-merge:5621a1c9c47fd4b9446cc88a9c6e13f503e42a44 |
| `76e54f7525983ecd8fdeef6fdc14896a72be7063` | selected | represented_source_only; watcher-backoff |
| `7707c9d7918263cd446a4b54d4eb48c3fea91048` | selected | represented_source_only; ast-walker-growth |
| `77ab4bea87008547acee60435a8e3df29d90a5cb` | selected | represented_source_only; ast-walker-growth |
| `7883507db6779a20bcfad398c8d6594f4206aa9b` | excluded | semantic_unresolved; native-merge:181c03dc188082ef67e1f27372afbf1117d44edb |
| `7a909466a7f0bab98af0004f311c8668f3cae44c` | excluded | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef |
| `8093d10b812d9a7840ae99d8f9d24a04cb0f9559` | selected | integration_in_progress; rescript-admission-termination |
| `88269ac2ee70ab88f3ee8d84bd509399e3a8f3d6` | excluded | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef |
| `8c30c7850975ed6a4cd0009e60d2679727ce9905` | excluded | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef |
| `8f3c15044b8200a15ee1895c5127f4bf117da493` | excluded | semantic_unresolved; native-merge:69fb951cd0c7f38808fc2b8b89bf3a9a4c5dc417 |
| `8f803f7262d1d1e58068f677f3e365048050b728` | deferred | semantic_unresolved; python-iris-class-calls |
| `95fa63441bf80c9ed871fa090ba925b8b6106d83` | excluded | semantic_unresolved; native-merge:11b662f9f7fba92012b872dd4fcaef7ee0c1300d |
| `99c3bb4cbe4c0f9542bf722d29cbb5dd01397455` | selected | represented_source_only; store-count-errors |
| `9a9534cbc45d40afa4e8bf070030a806acea1f5f` | excluded | semantic_unresolved; doclinks |
| `a041794f8e51ddf42342899be7b2ac26f951d777` | deferred | semantic_unresolved; native-merge:5c0b2418779ea74baacb8a74374d8dbd4f8beff3 |
| `a262a35724b3246353f07b5de96ec4731681dd4b` | excluded | semantic_unresolved; native-merge:e783f73d752f83b689451e3a7e48061cda1202f7 |
| `a378d47e7b4b14ad679b57d892ff98e9705d4f23` | excluded | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef |
| `aaa72fbe60ad341325e9c944e32cf67d98eca387` | deferred | semantic_unresolved; native-merge:95c91b8d8d0fc11a01b9109b43d0eea94393f538 |
| `aec9bfe3751d8c1f2eb2cb48d8fe584b9d3fd973` | excluded | semantic_unresolved; native-merge:13ce2c0cd5838c1f550d2a34af53ece6b82fb47e |
| `af3e003ad8e67cd1fe3b20dafaf4d33a4b351852` | excluded | semantic_unresolved; native-merge:def38f3ebcd6c00978373315c9785ec56270a2bf |
| `bc5b3dd216568c864b41303a674020a2d456ced9` | excluded | excluded_empty_tree_change; native-merge:11b662f9f7fba92012b872dd4fcaef7ee0c1300d |
| `bc960d87d663b7b4fc4c3ba558013c4609073762` | excluded | semantic_unresolved; native-merge:f7cfccfd0cce3f18347b8f7c498802adab452d85 |
| `c067c771b9f93d08a3e614e68aee7d710d2f256c` | excluded | semantic_unresolved; native-merge:8b57fe2a2b141df72228fb2d3a66bc7a190a23c2 |
| `c0a06b3d0a90513e0d80f79514b869f91cb958f1` | deferred | semantic_unresolved; sql-literal-values |
| `c1128db8187db2dac27a7ad14120fc09802c8018` | excluded | semantic_unresolved; native-merge:def38f3ebcd6c00978373315c9785ec56270a2bf |
| `c314dea4c1409a3a68fb33706b49e272809b5079` | deferred | semantic_unresolved; native-merge:5958f546dc0f9af9b856f4bbc1e709382ba8425f |
| `c8184ed94887a6b5e130726885b44a07c63a47b7` | selected | candidate_not_represented; ancestor-ignore-chain |
| `cda5e3c9e5c4d2251467ced00ff305205c368d17` | excluded | semantic_unresolved; native-merge:8ab13c94f6030cf4bb7eb2c7280fbe85ee740a83 |
| `d402623c20e1dbff62e560a865fd4955798dfbfe` | excluded | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef |
| `d4931241adaef732066afb5ebf1cb225e2474403` | deferred | semantic_unresolved; route-decorator-paths |
| `d7eba5a71d2a99d7b7bc588c666d22516ac0b563` | selected | represented_source_only; persisted-lsp-linear-decode |
| `da4bd25a01b612517512ee628bf0ec652000e80a` | excluded | semantic_unresolved; native-merge:5c69c149f9218ea3412945e82454792d0d016425 |
| `dadc57dbfa8615ce897c338f6af8d6c6eb1674b1` | excluded | semantic_unresolved; native-merge:8ab13c94f6030cf4bb7eb2c7280fbe85ee740a83 |
| `dc735c01fbbb4eddbbb73ff4df6007ab68413a4d` | deferred | semantic_unresolved; native-merge:2278498371650c89aa16305ec06390b93d581d55 |
| `df83d25ace9885e252363cb04be25a17da85c9e8` | deferred | semantic_unresolved; native-merge:bf4c476d5e18621d4424bc3eff322b1c7a678eeb |
| `df8dd1011bd1987d0ca5a45f324775aad2411f54` | selected | semantic_unresolved; python-complete-binding-lookup |
| `e026a60bb213b215caf2588455a8c885f0d82bc0` | excluded | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef |
| `e171bd48b5e5845ec6f00db65ff300caac4f7304` | excluded | semantic_unresolved; native-merge:69fb951cd0c7f38808fc2b8b89bf3a9a4c5dc417 |
| `e4780c208632b3c42488a3c9e315014ef4da157e` | selected | represented_source_only; macro-coverage-scans |
| `e8dcb28ba719e4272f605f3b961422bb1f3c3fad` | excluded | semantic_unresolved; native-merge:a5550e1a9eaa7487b79d02a048eb82b079771c60 |
| `e9058e11db9ed94e401ad711b2016249970a9040` | deferred | semantic_unresolved; native-merge:8fa5eefcca9c22e556aa139f84aa30869fca5a76 |
| `ea6fa49fc2c209de69c1b0f393d9d2cc25b61419` | excluded | integration_in_progress; rescript-admission-termination |
| `eb80a4ecff04d8bfacef20e1aa2ee29445a782ff` | deferred | semantic_unresolved; native-merge:28b4093c999c932b44a63b6b2f4036ccac719d60 |
| `eccc615398a0fa7fa29fd4494a5db4b0be77ebb8` | excluded | semantic_unresolved; native-merge:11b662f9f7fba92012b872dd4fcaef7ee0c1300d |
| `ed991154f212928df63ce5a5cd1159cfc8196f00` | deferred | semantic_unresolved; native-merge:9d65f4cb6257d6b55c6052212a55caa555e8297d |
| `efb231285d188ffda16a58f7a5ce8e0e6448638f` | excluded | semantic_unresolved; native-merge:4f7fa84e3210fb86c5268efe24f30bb58afab96f |
| `f05893757bed0fe83e450b6dbc62cf1c0dc22045` | excluded | semantic_unresolved; native-merge:9b1033790779aa6662d90b98c0c56d23272ae6ef |
| `f339aa625ffd71b936fe386250858e281e5b87ff` | selected | semantic_unresolved; python-complete-binding-lookup |
| `f3bdf11a039b8ed8d685b04ddecedaac1e409167` | deferred | semantic_unresolved; doclinks |
| `f428d583008ce87281bac138c02bd4375c56d735` | excluded | semantic_unresolved; native-merge:d1b2548ee81a3e335d7b54557e523090d557ea47 |
| `f50434eddfde4bbec93b4cd4716e60e10d1006e4` | excluded | semantic_unresolved; native-merge:11b662f9f7fba92012b872dd4fcaef7ee0c1300d |
| `fa138acf44525ef94cfd6e16772d601da5623aef` | excluded | semantic_unresolved; native-merge:c98323bff7e92a3bbefbffd065589a0bf7a4b118 |
| `fa2292111802b5f3b66db3e4dd168280b53ac17e` | excluded | mixed_shared_scope_unresolved; artifact-export-attribution |
| `fcf451c6a6dda768dfec5a8f61a01ac1e1cfa6bf` | deferred | semantic_unresolved; native-merge:c7488b031ce95a517ca91dac1df6ddab30718189 |
| `ff8dd47011c43c622d3dc82b33be32b4493e4ea9` | excluded | semantic_unresolved; native-merge:5df8b0442c48fed654c6f11d94cb6f64f00fc368 |

### DeepSeek Phase1 independent review — 2026-10-03 UTC

All three Herdr workers are idle with saved dispositions matching their exact
39/26/50 assignments. Parent verified all115 assignment sets, all cited evidence
links and342 manifest entries. Three Jev requests batched six bounded primary-source
questions using jev-1.13.0; worker verdicts and earlier Jev answers were excluded.
The [consolidated review](UPSTREAM_RECONCILIATION_REPORT_20261002.md#deepseek-phase1-review-with-batched-jev-advice--2026-10-03-utc)
records full distributions, exact source reasons and required continuation.
Documentary disposition is **changes requested**, not acceptance or a host review gate.

Runtime `9724d903046626df0640ced218259cb58070e4e8` needs a narrower diagnostics
claim: normal Linux budget pressure reads OS RSS, not the committed counter.
Operations `b04f54506ab42eddcbd89232a0d47d8355edd596` remains property-level
partial, with changed refinement caller/mechanism unresolved;
`f6afd6138e605365a76d065d04521ec2037bed3d` retains the neutral CLI title-policy question.
Integration `00411ce7bc41995783bfb16aada673515585446d` has missing changed Linux
alias/ordinary-directory continuation behavior in all three frozen CLI targets.
Bounded exclusions for `d70b79417978a9f3ab0d1aa849879dbf049793cf` and
`8f3c15044b8200a15ee1895c5127f4bf117da493` apply only at their exact absent seams.
These sampled judgments do not approve other16 integration exclusions.

Six commits have bounded independent semantic review; other109 full hashes are
explicitly independently unreviewed in
`CBM-DEEPSEEK-JEV-REVIEW-20261003/review-crosswalk.jsonl` under handoff evidence.
Integration admits incomplete function-level reading for29 partial rows and1
unresolved row; operations retains four remainders. Runtime finish requires host
verification binding; operations/integration finished partial. Worker completion,
source comparison, review, Jenkins validation, acceptance and integration remain
separate. No whole-commit closure counter is advanced by this bounded review.

### Luna low verification pilot20 — 2026-10-03 UTC

At the user's request, prepared external run `CBM-RECON-LUNA-LOW-PILOT20-20261003`
for independent function-level verification of20 existing integration findings.
Its exact full-hash assignment is a subset of the109 independently unreviewed
commits, excludes all six parent-reviewed hashes, and groups related config
symlink/uninstall, activation guard/alias followups, help, YAML, Codex guidance,
hook-path and sidecar behavior. Other89 remain outside the pilot. Related prior
source diffs/receipts are reused as discovery, not verdict authority; every changed
hunk and frozen target behavior must receive source evidence or explicit unknowns.

Interactive Codex `gpt-6-luna` with `model_reasoning_effort=low` launched in
Herdr `wC:pJ`, label `codex - luna-review-pilot20`, with `--no-daemon`. Actual
Codex process argv, managed pane environment and Jev-token inheritance are verified
without logging credential values. The official Jev footer is in the prepared
prompt once; bounded shared-context questions must be batched through the official
helper, with saved requests, full distributions and advice-use notes.

The pilot reuses `/tmp/cbm-recon-deepseek-integration-20261003` after its prior
worker stopped: source remains clean frozenQA `559214af`, and fresh status/root/
coverage generation matches its full persistence-disabled index
`2026-10-03T01:27:06Z` (26,764nodes/129,775edges,91 partial files).
Exact launch/index/assignment/ownership plan remains run evidence and
`handoffs/CBM-LUNA-LOW-PILOT20-20261003/`, not a second canonical ledger.
The worker must retain detailed checkpoints/handoffs at each batch and stop.

User-authorized cleanup: completed DeepSeek operations `wC:pG` and integration
`wC:pH` panes closed after handoff/checkpoint/disposition and manifest verification.
Runtime `wC:pF` was already absent. Other panes and all worktrees/evidence retained.
Pilot launch is not completed verification, runtime validation or acceptance;
no tests/builds/Jenkins/remote operations are authorized or performed.
