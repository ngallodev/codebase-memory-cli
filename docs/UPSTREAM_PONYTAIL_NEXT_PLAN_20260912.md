# Ponytail next merge set — 2026-09-12

Select only the remaining Cypher correctness guard:

`25a7aacc`

- `25a7aacc`: reject queries the parser did not fully consume, rather than
  returning a silently truncated interpretation.

The post-formatter refresh found `afc948ba` is already an ancestor of current
`main`; do not duplicate it.

This is a direct correctness guard with focused `cypher` regression coverage.
It is smaller and more immediate than language expansions, coverage-model
rewrites, logging policy, or speculative Clang 23 portability work. Before
implementation, re-run reverse/normal applicability checks against current
`main`, adapt only CLI-core code/tests, and validate `cypher`.

Deferred under Ponytail: `0ac290e2` (no current Clang 23 venue), `c25c1620`
(diagnostic-only), `cc9c2f4c` (MCP-coupled test surface), and the coverage
chain (crosses excluded MCP reporting code).
