# High-value upstream verticals — 2026-09-12

This plan implements the three highest-value, dependency-complete source
verticals from `UPSTREAM_308_CANDIDATE_AUDIT_20260912.md`. Each sequence is
ported from its earliest prerequisite forward; no terminal commit is picked in
isolation. The CLI fork excludes `src/mcp/**`, MCP tests/adapters, OpenHands,
Jenkins, and post-commit hooks.

## 1. Bounded Cypher trail correctness

`8546070d -> 0c5ffa50 -> 0d97729c -> cd22487c -> b3d31ca0 -> 58ef9f19`

Corrects path semantics, recursive row bounds, traversal scoping/truncation,
and aggregate correctness. Validate: `cypher,store_search`.

## 2. Go struct extraction and binding correctness

`47116b8e -> cb7cb444 -> cc00a027 -> d6417ada -> 7d76b88f -> 349aecba`

Restores Go struct field definitions and prevents invalid bare/import/cross-
language bindings. Validate: `pipeline,registry,edge_imports`.

## 3. Daemon activation and rendezvous safety

`8e70590d -> 9d4ac2b9 -> fc1b1ee7 -> 3a8c0d97 -> 7710f86e -> 4a29f0da ->
3e1703d7 -> 1a6f80c3 -> daac6bd4 -> 7f3e30e1`

Recovers stale rendezvous sockets, hardens background generation lifecycle,
and makes activation/rendezvous failures diagnosable. Platform-only NetBSD and
Windows evidence may be adapted or omitted, but the shared behavior must be
retained. Validate: `daemon_ipc,daemon_application,daemon_bootstrap,
daemon_runtime,daemon_frontend,cli`.

## Mandatory worker rules

1. For every upstream source commit, run both `git apply -R --check` and
   `git apply --check`; record represented behavior and narrow adaptations.
2. Keep the order above. If an excluded upstream change is an actual semantic
   prerequisite, stop that vertical and report the gap rather than bypass it.
3. Commit only scoped source/tests; report `git diff --check`, build, focused
   test evidence, and every intentionally omitted platform-only hunk.
4. Parent independently reviews and integrates worker commits, then validates
   the combined tree. No push is part of this plan.

## Completion record

- Bounded Cypher trails: all six source commits were already ancestors of the
  CLI baseline; no source delta was required.
- Go struct binding: the first five source commits were ancestors; the missing
  `349aecba` behavior was integrated as `e497dd0c`.
- Daemon activation: seven source commits were ancestors; the missing
  `8e70590d`, `9d4ac2b9`, and `1a6f80c3` behaviors were integrated together as
  `6e289d5a`.

The combined tree passed `git diff --check`, `scripts/build-dev.sh`, and the
selected pipeline/registry/import/daemon/CLI focused suite command. No MCP,
Jenkins, hook, remote, or push changes were made.
