# Upstream CLI Merge Plan — 2026-09-12

## Scope and boundary

Selectively port the next CLI-relevant fixes from
`/lump/apps/codebase-memory-mcp`. The CLI fork intentionally excludes
`src/mcp/**`, MCP tests, MCP session adapters, and OpenHands integration.
Do not change Jenkins, hooks, remotes, or unrelated local artifacts.

For every source commit, first run `git apply -R --check`: a successful
reverse check means the behavior is already represented and is skipped. Then
run `git apply --check`; conflicts are adapted narrowly to the CLI fork.

## Implementation batches

### A. Pipeline, store, cypher, and coverage

- `699be6c2`: fail an over-budget index attempt as a whole.
- `77a0c7d9`: avoid sorting an empty function list.
- `98899ba5`: count the unnamed Cypher head node against the cap.
- `4cbc0536`: bind store pagination to published generations.
- `e5c17de6`: avoid duplicate and EOF+1 coverage ranges.

Validate the smallest affected suites: `pipeline,cypher,store_pragmas,parse_coverage`.

### B. Daemon resource and session-root semantics

- `21591d51`: divide worker memory by active jobs, not configured capacity.
- `9846c3f1`: adapt only daemon-neutral separator-equivalent root comparison;
  exclude its MCP session adapter.

Validate: `daemon_application,daemon`.

### C. CLI filesystem and output behavior

- `3c854ae2`: judge followed symlink ownership using the followed inode.
- `8622d6a0`: make user-owned symlink following opt-in at CLI call sites;
  exclude MCP/UI call sites.
- `b1100a49`: emit the verbose cold-start hint as a plain line.
- `48149cac`: retain the CLI discovery defaults/help change only.

Validate: `platform,cli,config_json_like`.

## Deferred pending semantic comparison

- `e410c86f` (complexity ordering): CP90 may already supply equivalent
  determinism. Compare its algorithm and test coverage before porting; do not
  import its large fixture matrix speculatively.

## Integration and acceptance

1. Each batch works in an Agent-Workflow isolated worktree based on `main`.
2. Workers commit only their scoped changes and report focused validation.
3. Integrate only independently reviewed diffs, then run `git diff --check`,
   `scripts/build-dev.sh`, and the combined focused suites.
4. The known `daemon_runtime` hang remains a separate diagnostic item; do not
   call the combined daemon suite green until it is resolved.
5. No push without a new explicit instruction.
