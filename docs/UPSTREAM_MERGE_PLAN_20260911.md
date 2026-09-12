# Upstream CLI Merge Plan — 2026-09-11

## Scope

Port the CLI-relevant upstream commits from
`055fbb7d..b790be3d` and the still-missing carry-over fixes below. Never port
`src/mcp/**`, MCP tests, or OpenHands MCP configuration.

## Mandatory preflight for every commit

1. Confirm the source commit is still absent with `git apply -R --check`.
   A successful reverse check means it is already represented and must be
   skipped.
2. Check a normal patch with `git apply --check`. Cherry-pick only a clean
   patch; manually adapt conflicting patches without widening scope.
3. Run `git diff --check` and the listed focused suites.

## Worker batches

### A. Daemon security and reliability

- `33c4fd80`, `0567dc70`: IPC listen diagnostics and Windows guard.
- `00060cec`, `ecfb9642`: startup fast-fail and deterministic test.
- `8116672a`: single-UID user-namespace ancestor handling.
- `df579830`, `61884951`: Windows built-in Administrator trust and formatting.
- `6091db71`: preserve the cold-storm ephemeral generation.

Validate: `scripts/test.sh --suites daemon_ipc,daemon_bootstrap,daemon_runtime`.

### B. CLI installation, uninstall, and config editing

- `c572ddc4`: do not block uninstall on agent-config cleanup.
- `e5aabdf8`, `7bf366f6`, `91c63be5`, `a3c24a71`: safe user-owned config
  symlinks and Windows guards.
- `4fa11d1a`: remove orphan SQLite sidecars on index deletion.
- `8135be28`: remove the Windows user-PATH install entry on uninstall.

Excluded: `3a4160b4`, `7e73b48e` (OpenHands MCP integration).

Validate: `scripts/test.sh --suites cli,config_json_like,config_text_edit,config_toml_edit,config_yaml_edit`.

### C. Extraction and CI toolchains

- `52701625`: use thread CPU time for tree-sitter parse budgets.
- `557cb010`: pin GitHub diagnostic/analyzer lanes to LLVM 21.
- `8134712`: adapt the MSan image to LLVM 21.

Validate: `scripts/test.sh --suites edge_types_probe` and static workflow/Dockerfile checks.

## Integration order

1. Merge the three worker commits onto a clean integration branch.
2. Run `git diff --check`, `scripts/build-dev.sh`, and the union of their
   focused suites.
3. Run the full venue gate only after focused validation is green.
4. Independently review the resulting diff before commit/push.
