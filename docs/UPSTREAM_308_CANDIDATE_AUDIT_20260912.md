# Upstream merge progress — canonical ledger

This is the single canonical document for upstream merge work in this
repository. The filename is retained for stable links; the old 308-commit
snapshot and the separate dated plan documents are superseded by this ledger.

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

Of 367 differing non-merges, 288 have no target semantic review, 37 only have
adapter-import/path/hunk triage with neutral parity unresolved, 39 have bounded
source/dependency dispositions, and three are verified empty tree changes.
Even a bounded represented behavior is not exhaustive whole-commit verification:
364 non-merge rows and all 311 merge rows retain explicit full-hash semantic
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
