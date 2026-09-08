# Linux Release Qualification

Status: **IN PROGRESS**

This document records phase-end Linux qualification evidence for the CLI-first release candidate line. It is intentionally separate from migration history.

## Qualification policy

- Run product and test targets with their production/project compiler flags.
- Treat failures as evidence and classify them before changing code.
- Do not restore retired MCP/protocol behavior to satisfy stale tests.
- Platform-only Windows skips are acceptable on Linux when explicitly marked `SKIP_PLATFORM`.
- A production build is not considered verified until `build/c/codebase-memory-cli` links successfully.

## Fast sanitized foundation gate

Command:

```sh
make -f Makefile.cbm test-foundation
```

The foundation target now has its own minimal runner and incremental object graph. It no longer links the full `tests/test_main.c`, whose daemon/index/UI subprocess roles are intentionally outside the foundation boundary.

Current result:

```text
312 passed, 0 failed, 2 platform-only skips
```

The two skips are native-Windows-only process/locking probes.

During qualification this gate exposed and corrected two stale test-infrastructure contracts:

1. `test-foundation` incorrectly used the global test runner and therefore referenced subsystems it did not link.
2. `tests/test_log.c` still asserted the removed `mcp.request` observability event after that API had been deliberately deleted.

Neither finding required product architecture changes.

## Release and fixture contracts

The following contracts pass on the consolidated source:

```text
tests/test_runtime_isolation_contract.sh
tests/test_security_fuzz_harness.sh
tests/test_version_metadata_contract.sh
tests/test_language_count_contract.sh
tests/test_smoke_fixture_contract.sh
tests/test_release_archive_extractor_contract.sh
tests/test_release_candidate_derivation_contract.sh
tests/test_release_gate_chain_contract.sh
tests/test_vt_release_notes_contract.sh
scripts/check-no-test-skips.sh
```

`test_smoke_fixture_contract.sh` was reconciled with the supported CLI-first smoke topology. It now protects the actual release properties: canonical CLI invariants, installer round-trip, native Windows PowerShell installation, isolated PATH verification, single-binary packaging, and proxy-safe loopback requests.

`test_vt_release_notes_contract.sh` now uses `codebase-memory-cli` artifact names while retaining the historical GitHub repository slug where appropriate.

## Optimized production build

Command:

```sh
make -f Makefile.cbm cbm
```

The build has progressed incrementally through production sources and the large vendored grammar set with no compiler correctness failure. In the current execution environment, repeated bounded compiler windows expire during the optimized vendored/grammar build before final link.

Status: **NOT YET VERIFIED IN THIS PHASE**.

This is an execution-duration/build-ergonomics limitation, not a claimed pass. CP48 previously established an explicit Linux production link on an earlier consolidated state; current release qualification still requires a final link of the current candidate.

The GitHub RC build is expected to provide the authoritative release build evidence. A local final link should still be obtained when practical, but release qualification must use the immutable GitHub-produced artifact bytes.

## Remaining Linux phase-end work

1. Obtain final optimized link for the consolidated candidate, locally or through the GitHub RC workflow.
2. Build the full sanitized test runner.
3. Run targeted integration/E2E suites first.
4. Run the broad Linux suite as a dedicated phase-end pass.
5. Classify every failure as product defect, stale/test defect, environment limitation, or accepted known issue.
6. Rerun affected gates after any code change.

Do not expand test count solely to improve coverage statistics; prefer existing integration/E2E correctness guards.

## CP64 broad-runner progress and RC hold preparation

The broad sanitized `build/c/test-runner` build was resumed incrementally across three bounded compiler windows. It progressed through production, pipeline, and test objects with no compiler correctness failure, but did not yet reach the vendored link boundary in this environment. This remains **IN PROGRESS**, not a pass.

In parallel, the GitHub release workflow was reconciled with the external native-Windows qualification contract. Releases now default to a draft hold after CI verification; npm/PyPI publication and public un-drafting are deferred until `promote-qualified-release.yml` validates qualification evidence produced on `luigi.home.arpa` against the exact Windows ZIP and executable hashes.

## RC authoritative-build boundary

The local broad sanitized build remains useful diagnostic evidence, but it is not allowed to delay the release-qualification handoff indefinitely when the only blocker is bounded local compiler time and no compiler/source failure has surfaced.

For an actual release candidate, the GitHub release workflow is the authoritative final Linux/all-platform build and test venue because it:

- builds the immutable artifacts that are later qualified;
- executes the release test matrix, smoke, soak, and security gates;
- records the workflow run against the exact dispatched source commit;
- leaves the release in DRAFT for external Windows qualification.

A GitHub RC workflow failure is qualification evidence and must be fixed in a new source commit/new RC. A successful workflow supplies the final-link/build evidence that the current local execution environment has been unable to complete within bounded compiler windows.

## 2026-09-04 full-suite remediation (4f0d7b9 follow-up)

The first direct full sanitized run on `release-tooling` exposed stale test contracts plus one
ASan failure. The first remediation slice makes the following distinctions explicit:

- daemon operation responses on the internal application wire are length-delimited; tests must
  not treat those buffers as NUL-terminated strings;
- the old injectable daemon update-generation seam had no production provider or caller and is
  removed instead of maintaining tests for unreachable behavior;
- `projects` treats a not-yet-created cache directory as an empty fresh installation while still
  reporting real directory access errors;
- the Windows VM guard contract expects `codebase-memory-cli.exe`;
- the POSIX quiet-timeout tree probe gives sanitizer startup enough time to publish its PID marker.

Changed C surfaces compile under the repository `-Wall -Wextra -Werror` sanitizer and production
flags. The Windows VM manifest and build-directory safety contracts pass locally after the stale
binary expectation is corrected. The remaining auto-index/watch and daemon-runtime assertions are
tracked separately and are not marked resolved by this slice.

## CP72 external full-run evidence and CP74 qualification-harness rebase

A clean production build from the CP72 working tree completed successfully with
`scripts/build.sh` and produced `build/c/codebase-memory-cli`. This closes the
local optimized-link evidence gap for that source state; immutable release
qualification still requires the GitHub-produced RC bytes described above.

The canonical `scripts/test.sh` run did not reach the sanitized suite. Its
venue-parity preflight exposed three harness/interface defects:

1. the interface probe invoked every entry through `bash` before dispatching
   Python entries through `python3`, so `generate-sbom.py --help` could execute
   Python source as shell input and hang at `import datetime`;
2. `scripts/smoke-test.sh` needed a `--help` contract;
3. `scripts/smoke-invariants.sh` needed `--help` and strict unknown-option
   behavior.

The authoritative `cf0869fc` source already contains the smoke entry-point
interface behavior from items 2 and 3. CP74 rebases the remaining CP73 intent
onto that source without replacing those newer smoke-script revisions: the
venue-parity probe now chooses the interpreter before execution, so Python
entry points are never first executed through `bash`.

The CP72 memory-analysis evidence also showed that the previous dynamic gate
mixed allocation retention with leak ownership:

- Heaptrack reported 85 allocations not deallocated at process exit;
- the test suite deliberately retains process-lifetime lock-registry identity
  tombstones so stale raw registry pointers can never become live through
  allocator address reuse;
- POSIX fork tests intentionally execute child paths with inherited parent
  allocations and `_exit()`;
- the top-level Valgrind summary reported 0 definitely lost bytes, 0 indirectly
  lost bytes, 0 possibly lost bytes, and 0 memory-error contexts, with 2,816
  bytes still reachable in 16 blocks from the deliberate retired-registry
  tombstones.

The three daemon-IPC test failures under Memcheck were therefore harness
artifacts: leak findings in fork children were counted as errors and
`--error-exitcode=99` replaced the child status that the tests intentionally
assert. CP74 preserves CP73's corrected memory gate: leak kinds cannot rewrite
fork-child behavioral exit codes, actual Memcheck error contexts remain fatal,
and top-level definite/indirect loss remains a release failure. Heaptrack's raw
retained-allocation count remains diagnostic attribution rather than an
ownership verdict.

The subsequent post-CP74 qualification run reached both legs: dynamic memory
analysis completed green, while the broad sanitizer suite reached execution and
isolated its remaining failures to CLI/incremental contracts. See the CP76
reconciliation below; only the broad `scripts/test.sh` rerun remains required
for the CP76 candidate unless new memory evidence appears.

## CP75 immutable RC identity hardening

Gate-2 review found one remaining contradiction between the release-qualification plan and the active production workflow: `release.yml` still allowed `replace=true`, deleted an existing release/tag, and force-pushed the requested release tag. That made a supposedly immutable RC identity mutable if the workflow was dispatched incorrectly or reused after a partial failure.

The release workflow now fails closed. It refuses an existing GitHub release or remote tag at the draft boundary, creates the tag for the exact dispatched `GITHUB_SHA` without force, and contains no replace/delete path. A concurrent duplicate dispatch can pass the read check, but only one normal tag push can succeed; the other run therefore stops before release creation. Failed candidates are retained as immutable evidence and require a new deliberate release identity.

The existing release gate-chain contract was extended rather than adding another test surface. It now rejects replacement/deletion/force-tag behavior and requires both the remote identity guards and normal dispatched-SHA tag push. This is Gate-2 readiness work and does not alter product/runtime code or change the pending need for a fresh broad Linux suite after CP74.

## CP76 broad-suite failure reconciliation

The post-CP74 canonical run reached the sanitized suite and reported 7,352 passed / 59 failed. Failure concentration was narrow rather than systemic: 53 failures were in `cli` and 6 in `incremental`; daemon application, daemon runtime, and daemon IPC were green. The corrected memory-analysis leg also completed green, with Valgrind reporting no lost memory.

CP76 reconciles that evidence rather than treating 59 assertions as independent product defects. Most failures were stale MCP-era test expectations after the CLI-first migration. Three production defects were found while doing that reconciliation and are fixed in this candidate:

1. registry-backed agent lifecycle installation could run before the effective config path was resolved, causing Qoder/Devin hooks to be skipped;
2. hook project lookup could open a nonexistent derived-name DB during its fast probe and thereby shadow an existing custom-named index discoverable by canonical `root_path`;
3. Claude's exact-owned gate-script migration did not recognize the released MCP-era sibling binary basename after the product executable rename.

The incremental assertions now follow the neutral operation-result wire shape. CLI installer assertions require supported CLI-first durable instructions/skills from `install`, explicit CLI-owned hooks only from `install-hooks`, and preservation of MCP registrations/assets across CLI lifecycle operations.

Focused sanitizer verification of the final eight-case CLI cluster passes 8/8 with ASan/UBSan and leak detection enabled. Changed production and test objects compile under the normal strict sanitizer and production warning flags. The sandbox cannot execute the full incremental fixture because that fixture performs an outbound GitHub clone; its changed assertions were reconciled against the captured real-run response shapes and compile successfully.

**Gate status:** `scripts/test.sh` must be rerun on CP76 because CP76 changes `src/cli/cli.c` and `src/cli/hook_augment.c`. A repeat of `scripts/analyze-memory.sh` is not required unless the new broad sanitizer run produces memory evidence that needs attribution; the post-CP74 corrected memory-analysis run is already green.

## CP76 qualification evidence and CP77 incremental closeout

The canonical CP76 `scripts/test.sh` rerun completed the clean ASan/UBSan build,
all preflight contracts, and all 138 suites. It improved the broad result to
7,408 passed / 3 failed / 3 skipped. `cli` is now fully green at 236/236;
`daemon_application` is 44/44, `daemon_runtime` 46/46, and `daemon_ipc` 43/43.
The only failing suite is `incremental` (160 passed / 3 failed).

The CP76 memory-analysis rerun is independently green: scan-build reported no
findings, Heaptrack retained 85 allocations under the diagnostic retention
policy, and Valgrind reported 0 lost bytes with the same 2,816 bytes of
intentional process-lifetime reachable state. No new daemon-memory remediation
is indicated.

The three incremental failures reduce to two causes:

1. the in-process test operation host did not wire the physical index worker's
   `project_invalidate` seam. After ADR operations cached the current project
   store, later reindex tests attempted to publish while that test-only cached
   reader remained live. Production daemon indexing does not share that
   in-process store: it delegates physical indexing to a separate worker
   process. CP77 wires the existing invalidation seam in the test host so the
   integration host obeys the production worker lifecycle contract;
2. one query assertion still expected a JSON-shaped result key even though the
   default neutral query response is the compact table form (`rows: ...
   (cols: n)`). CP77 asserts the current compact response contract directly.

The invalid-project `search_graph` failure also exposed a real read-path edge
case: search used the create-capable project open, so an unknown project could
be materialized as an empty DB and returned as an empty search rather than a
`project not indexed` result. CP77 opens the requested project read-only and
requires its project row to exist before searching, preventing that ghost DB
path.

The 2,896-byte LeakSanitizer tail in the failing incremental suite is accounted
for by the three assertion exits before their response buffers were freed
(1,024 + 992 + 880 bytes, exactly matching the report). It is not separate leak
evidence once those assertions are corrected.

**Gate status:** CP77 requires one final external `scripts/test.sh` rerun because
it changes `src/operations/search.c`. The memory-analysis leg does not need to
be repeated unless that rerun exposes new memory evidence.
