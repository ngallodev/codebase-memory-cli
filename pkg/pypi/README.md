# codebase-memory-cli

`codebase-memory-cli` is the Python distribution wrapper for Codebase Memory
CLI, a local CLI-first fork and C port of Codebase Memory MCP. The fork removes
the third-party MCP server surface for workplaces where such servers are not
permitted.

The wrapper downloads and verifies the native runtime set for the current platform and exposes the `codebase-memory-cli` executable.

## Installation

```bash
pip install codebase-memory-cli
# or
pipx install codebase-memory-cli
```

## Quick start

```bash
codebase-memory-cli index .
codebase-memory-cli status
codebase-memory-cli search "ClaimValidator"
codebase-memory-cli --help
```

Use `--json` on supported commands for stable machine-readable output. Operational failures return a non-zero exit status.

Agent integrations can be installed separately with:

```bash
codebase-memory-cli install --skip-binary
```

New installs do not create MCP server registrations.

## Supported platforms

| OS | Architecture |
|---|---|
| macOS | arm64, amd64 |
| Linux | arm64, amd64 |
| Windows | arm64, amd64 |

## Documentation

The source repository and full documentation are at https://github.com/ngallodev/codebase-memory-cli.
