# CP90 Follow-up Completion Plan

Date: 2026-09-09

This plan closes the five upstream-follow-up workstreams whose durable
completion records were blocked or partial. It is based on CP89 plus the
selective upstream integration commit 3036e195, and does not authorize
merging MCP frontend code or pushing worker branches.

## Current status

| Workstream | Worker result | Implementation evidence | Evidence gap |
| --- | --- | --- | --- |
| YAML removal guards | complete on Linux | 523d398d; Windows-gated reparse-point and access-denied tests; focused suite passed | Native Windows execution remains deferred |
| Version-cohort handoff | complete on Linux | 01af5cb2; distinct cache-root handoff and Windows separator-form tests; focused suite passed | Native Windows execution remains deferred |
| Complexity determinism | complete | 43d3a210; canonical tie-breaker and fixture corpus; pipeline suite passed | None |
| Razor / .cshtml | complete | 5b1bc085; .cshtml language mapping, route extraction, and three regressions; focused suites passed | None |
| Embedded Svelte/HTML/Astro | complete on Linux | f8971e7a; corrected test-runner linkage and focused extraction/structural/label suites passed | Native Windows execution is not required for this feature |

The separate complexity-r2 run is not a sixth task. It only rechecked the
complexity fixture and was blocked while writing its completion sidecar; its
fixture identity check passed.

## Completion sequence

### 1. Normalize the isolated worker results

Use disposable worktrees based on 3036e195 and apply worker commits as review
inputs, not as unconditional cherry-picks:

    YAML        523d398d
    cohort      01af5cb2
    complexity  43d3a210
    Razor       5b1bc085
    embedded    f8971e7a

For each worktree:

- verify the commit parent and changed-file list;
- reject src/mcp, MCP tests, and unrelated release-tooling changes;
- run git diff --check;
- preserve the CP89 release-provenance surface;
- retain the worker completion record, even when its status was blocked by
  workflow permissions rather than code.

Do not merge these commits directly into the dirty checkout until each gate
below passes.

### 2. Finish complexity evidence

Worktree: 43d3a210.

Run:

    scripts/build-dev.sh
    bash scripts/test.sh --suites pipeline

If generated grammar compilation stalls again, build the test runner with the
repository's normal cached incremental path, capture the exact stalled target,
and retry once in a clean disposable worktree. Do not weaken the grammar build
or disable sanitizers.

Before acceptance, add or verify a duplicate-key cyclic fixture covering equal
qualified name, file path, start line, end line, name, and label. Ensure the
test checks allocation before freeing the signature. This addresses Terra's
remaining determinism concern.

Acceptance: pipeline suite green, duplicate-key fixture present, no sanitizer
error, and production incremental build green.

### 3. Finish version-cohort evidence

Worktree: 01af5cb2.

Run the focused suite in an environment where cbm_mkdtemp is permitted to
create its secure private root:

    bash scripts/test.sh --suites version_cohort

Then run the daemon/application handoff tests that exercise the same cache-root
identity path. Native Windows must execute the separator-form case; Linux
cannot substitute for that result.

Acceptance: distinct cache-root holder/waiter handoff passes, Windows
slash/backslash normalization passes, finite-deadline conflict retry remains
bounded, and no MCP source is present.

### 4. Finish YAML evidence

Worktree: 523d398d.

Run:

    bash scripts/test.sh --suites config_yaml_edit

On a native Windows runner, execute the reparse-point and access-denied cases
with Developer Mode or symlink privilege explicitly recorded. Confirm that
access-denied targets fail closed, dangling links are not treated as absent,
and no lock sidecar or target mutation occurs after refusal.

Acceptance: Linux focused suite green, Windows-gated tests execute rather than
silently skip except for explicitly recorded platform privilege, and
check-no-test-skips.sh remains green.

### 5. Finish Razor evidence

Worktree: 5b1bc085.

Run:

    bash scripts/test.sh --suites language,extraction

Confirm .cshtml maps to C#, @page produces a GET route, .razor behavior is
unchanged, and a layout without @page produces no route. Preserve the current
CP89 .frm/.cls VB6 disambiguation in src/discover/language.c.

Acceptance: language and extraction suites green, no unrelated grammar
classification changes, and no MCP files or tests added.

### 6. Finish embedded extraction evidence

Worktree: f8971e7a.

First repair the test-runner link invocation in the disposable worktree; the
worker's reported missing SQLite symbols indicate an incomplete link set, not
an extraction assertion. Use the same Makefile.cbm target and library list as
the current repository test runner rather than adding a new dependency.

Run:

    bash scripts/test.sh --suites extraction,edge_structural,grammar_labels

Review the behavior change carefully:

- JavaScript/module blocks extract definitions, imports, and calls;
- src=, JSON, importmap, and other non-JavaScript blocks remain inert;
- Svelte and Astro label goldens intentionally change;
- Astro frontmatter's TypeScript default is an explicit contract change.

Acceptance requires all three focused suites to link and pass, existing Vue
negative controls to remain valid, and graph-count/label deltas to be
intentional and documented. If the link failure cannot be repaired without
changing the fork's test infrastructure, defer this feature rather than
claiming validation.

### 7. Final integration gate

After all five workstreams have independently passed their gates, apply only
the accepted commits to a clean CP89-derived integration worktree in this
order:

1. 523d398d YAML evidence closeout
2. 01af5cb2 cohort evidence closeout
3. 43d3a210 complexity evidence closeout
4. 5b1bc085 Razor evidence closeout
5. f8971e7a embedded extraction evidence closeout

Then run:

    git diff --check
    scripts/build-dev.sh
    bash scripts/test.sh --suites cli,config_yaml_edit,version_cohort,language,extraction,edge_structural,grammar_labels,pipeline
    bash scripts/test.sh --suites mem

Run the clean full venue gate only after the focused gates pass. Native Windows
qualification is required for Windows-specific claims. Commit the accepted
integration as a separate release-tooling change; do not push until the full
gate and release review are green.

## Remaining qualification deferrals

- 92725a5e remains excluded: it is a Windows MCP stdio-server watchdog, not a
  CLI process behavior.
- c1b9c451 remains excluded as a direct port; its MCP session adapter is
  obsolete. Any 9846c3f1 separator-equivalence behavior requires a separate
  neutral daemon implementation and test.
- Native Windows execution of the YAML and cohort cases remains outstanding;
  Linux cannot substitute for that evidence.
- 24f54c0e is accepted on Linux in f8971e7a after the focused runner linked and
  passed; its intentional graph/label behavior changes are covered by the
  focused suites.
