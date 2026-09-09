# codebase-memory-cli

`codebase-memory-cli` is the npm distribution wrapper for Codebase Memory CLI,
a local CLI-first fork and C port of Codebase Memory MCP. The fork removes the
third-party MCP server surface for workplaces where such servers are not
permitted.

The package downloads and verifies the native runtime set for the current platform, then exposes the `codebase-memory-cli` executable on `PATH`.

## Installation

```bash
npm install -g codebase-memory-cli
```

## Quick start

From a repository:

```bash
codebase-memory-cli index .
codebase-memory-cli status
codebase-memory-cli search "ClaimValidator"
codebase-memory-cli snippet ClaimValidator.validate
codebase-memory-cli trace ClaimValidator.validate --direction both
```

For scripts and agents, request stable machine-readable output with `--json` where supported:

```bash
codebase-memory-cli search "ClaimValidator" --json
```

Running `codebase-memory-cli --help` shows the installed command surface. Operational failures return a non-zero exit status.

## Agent integrations

The CLI can install compatible skills, durable instructions, and lifecycle/context hooks for detected coding agents:

```bash
codebase-memory-cli install --skip-binary
```

Use `codebase-memory-cli install --plan` to inspect planned writes before applying them. New installs do not create MCP server registrations.

## Supported platforms

| OS | Architecture |
|---|---|
| macOS | arm64, amd64 |
| Linux | arm64, amd64 |
| Windows | arm64, amd64 |

## Documentation

The source repository and full documentation are at https://github.com/ngallodev/codebase-memory-cli.

## License

MIT
