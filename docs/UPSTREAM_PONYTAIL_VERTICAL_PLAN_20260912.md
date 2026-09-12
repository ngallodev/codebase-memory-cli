# Ponytail next verticals — 2026-09-12

Selected from the 150-candidate inventory using the smallest useful-delta
rule. This intentionally skips feature additions, language expansions,
performance work, output-policy changes, and CI pins without a current CLI
failure. Never port `src/mcp/**`, MCP tests/adapters, OpenHands, Jenkins, or
hooks.

## A. Cypher live-scope and binding-capacity correctness

`63b99976 -> 426e415f -> 04ba2fa3`

The upstream ancestry confirms this order. It prevents `RETURN *` after
`WITH` from projecting stale names, and rejects scopes wider than the binding
can represent rather than silently returning blank/truncated values.

Validate: `scripts/test.sh --suites cypher`.

## B. Registry receiver-chain correctness

`2c76563a`

The earlier Go binding vertical is already integrated. This small follow-on
rejects name-only symbol matches contradicted by an observed receiver chain.

Validate: `scripts/test.sh --suites pipeline,registry`.

## C. Daemon cohort handoff race

`9104feb6`

The daemon activation vertical is already integrated. This isolated retry
fix waits only until the caller deadline when teardown transiently holds a
mismatched cohort lock, while retaining fail-fast behavior for unbounded
conflicts.

Validate: `scripts/test.sh --suites version_cohort`.

## Worker contract

For each source commit: run reverse and normal `git apply --check`, adapt only
the CLI-core hunk, keep the listed order, commit scoped changes, and report
`git diff --check`, `scripts/build-dev.sh`, and focused results. Parent
reviews/integrates only passing scoped commits. No push is authorized.

## Completion update

- Cypher scope/capacity: merged as `a7721244` after independent review.
- Registry receiver chain: merged as `e52b31cd` after independent review.
- Cohort retry `9104feb6`: already represented by `3036e195`; verified with
  18 focused tests and intentionally not duplicated.

Post-integration `cypher,pipeline,registry` validation passed. Exclude all
five source hashes from future selections.
