# Upstream candidate collection — 2026-09-12

Collected while UPSTREAM-0926 batches A–C were in progress. Those batches are
now merged on `main` as `d3bb290c`, `c716cb7d`, and `fc9aeedc`; their upstream
sources must not be selected again. All remaining candidates need a refreshed
applicability check; absence alone is not port authority.

## Recommended next set

| Group | Upstream commits | Reason |
|---|---|---|
| Cypher binding correctness | `04ba2fa3`, `426e415f`, `63b99976` | Core query semantics with focused `test_cypher` coverage. |
| Pipeline diagnostics | `c25c1620` | Four-line, CLI-relevant format-migration route visibility fix. |
| Go selector resolution | `349aecba`, `2c76563a` | Restores usage/read/write resolution for genuine Go field selectors. |
| Cohort handoff race | `9104feb6` | Daemon lifecycle race; isolated `version_cohort` scope. |
| Clang 23 portability | `0ac290e2` | POSIX unused-global diagnostic; no behavior change outside Windows tests. |

## Evaluate as cohesive, dependent changes

- Coverage chain: `b04f5450`, `a9535092`, `cb167b9d` (plus already-active
  `e5c17de6`). Earlier revisions include pipeline/store and MCP reporting
  interactions, so port only after the active coverage change is integrated
  and compare the complete resulting range model.
- Embedded markup extraction: `24f54c0e`. Valuable core capability, but a
  289-line language/extraction expansion rather than a small corrective port.
- CLI output policy: `4a9fcd97`, `5f58e233`. The former is a feature and the
  latter couples CLI diagnostics to excluded MCP compact-output behavior;
  retain the current CLI policy unless a concrete output defect is reported.
- Environment/confidence hardening: `6b801eec`, `62b44519`. Both mix core
  behavior with MCP callers; map CLI callers before taking a slice.

## Explicitly rejected or superseded

- MCP-only: `7a1e069b`, `7be95be5`, `8a8c6839`, `10ece1bb`.
- Store scan scoping `cc9c2f4c`: includes MCP contract tests and needs a
  caller-level value assessment.
- `3a8b8e82`: superseded by active, more general symlink work (`3c854ae2`,
  `8622d6a0`).
- `7d20ff93`: broad Claude-hook registration rewrite; defer pending a
  user-observed hook defect.
- `e410c86f`: remains deferred for CP90 equivalence review.

No MCP frontend code, MCP tests, OpenHands integration, Jenkins configuration,
or hooks are included in the recommended set.
