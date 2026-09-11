# Codebase Memory CLI

Codebase Memory CLI builds a persistent structural knowledge graph of a source
repository and exposes it through a local command-line interface for coding
agents and humans.

This repository is a fork and C port of
[Codebase Memory MCP](https://github.com/DeusData/codebase-memory-mcp). The
fork keeps the code-intelligence engine, but removes the MCP server and its
third-party MCP integration surface. It exists for workplace environments
where policy prohibits using third-party MCP servers while still allowing a
local, auditable developer tool.

The product executable is `codebase-memory-cli`. Running it with no command shows CLI help; there is no supported MCP stdio server entry point.

> The CLI is the supported product surface. Some cache and package paths retain
> the historical `codebase-memory-mcp` name for compatibility; they do not
> enable or start an MCP server.

## Features

Codebase Memory parses source with tree-sitter and optional language-server enrichment, then stores functions, classes, methods, call relationships, imports, routes, cross-service links, and other structural facts in a local graph-backed index.

Use it when you need to answer questions such as:

- Where is this concept implemented?
- Who calls this function?
- What does this function call?
- What code is likely affected by a change?
- What is the exact source for a graph result?
- Is the relevant code actually covered by the current index?
- What projects have already been indexed?

The graph is a discovery and structural-evidence layer, not a substitute for source verification. For material claims, especially negative or exhaustive claims, verify exact source and index coverage.

It includes:

- tree-sitter parsing for 162 languages;
- hybrid LSP-style semantic enrichment for supported languages;
- definitions, calls, imports, usages, routes, channels, and cross-service links;
- architecture summaries, change-impact analysis, dead-code queries, ADRs, and Cypher queries;
- BM25, semantic, structural, and graph-augmented source search;
- cross-repository graph intelligence and optional 3D graph visualization;
- a persistent local index with incremental updates and optional shared graph artifacts;
- a coordination daemon, supervised workers, and filesystem watcher for reliable background indexing.

## Quick start

Build or install the binary, then from a repository:

```sh
codebase-memory-cli index .
codebase-memory-cli status
codebase-memory-cli search "ClaimValidator"
codebase-memory-cli snippet ClaimValidator.validate
codebase-memory-cli trace ClaimValidator.validate --direction both
codebase-memory-cli coverage src/Claims/ClaimValidator.cs
```

For scripts and coding agents, add `--json`:

```sh
codebase-memory-cli search "ClaimValidator" --json
codebase-memory-cli trace ClaimValidator.validate --direction inbound --json
```

Canonical machine output is ordinary CLI output or stable JSON. Operational
failures return a non-zero exit status.

## Core commands

| Command | Purpose |
|---|---|
| `index [PATH]` | Build or refresh an index. Defaults naturally to the current repository when possible. |
| `status` | Show index/runtime status for the current or selected project. |
| `search [QUERY]` | Find graph-backed symbols and structural candidates. |
| `trace SYMBOL` | Trace inbound, outbound, or bidirectional call relationships. |
| `snippet SYMBOL` | Retrieve exact source for a qualified symbol. |
| `coverage [PATH]` | Check whether graph evidence covers a file/scope before relying on absence or completeness. |
| `projects` | List indexed projects. |

Use command-specific help for the complete schema-derived flags:

```sh
codebase-memory-cli search --help
codebase-memory-cli trace --help
codebase-memory-cli coverage --help
```

When the working directory does not identify the intended index unambiguously, pass `--project NAME`.

### Compatibility interface

The generic operation form remains available for scripts that need the complete
operation catalog:

```sh
codebase-memory-cli cli search_graph --project my-project --name-pattern '.*Handler.*'
```

The named commands above are preferred for normal use.

## Agent workflow

The shipped Codebase Memory skill teaches a graph-first evidence loop rather than a tool-name inventory:

1. `projects` / `status` — establish project and freshness.
2. `search` — discover candidate symbols.
3. `snippet` — establish exact source truth.
4. `trace` — establish callers, callees, and likely impact.
5. `coverage` — establish where graph evidence is trustworthy.
6. Fall back to direct source reads/grep for literals, configs, non-code files, or every reported coverage gap.

The skill preserves three evidence levels:

- **Scout** — fast positive/provisional discovery; no negative or exhaustive claims.
- **Verify** — default task-directed evidence with source and coverage verification.
- **Auditor** — bounded exhaustive review with complete relevant pagination, coverage fallback, and explicit limitations.

### Skills and hooks

`install` may place CLI-first skills and durable instructions for detected coding agents. It **never installs hooks** and does not create, migrate, or remove `codebase-memory-mcp` registrations or MCP-owned assets.

Hooks are a separate, explicit opt-in surface. Install only `codebase-memory-cli`-owned lifecycle/context hooks with:

```sh
codebase-memory-cli install-hooks
```

Hooks are an optimization and guidance surface, not the only route to the graph. If a warm runtime is absent or a hook cannot augment context, it fails open and the ordinary CLI still works.

To configure detected agent assets without replacing an externally managed binary:

```sh
codebase-memory-cli install --skip-binary
```

Use `install --plan` to inspect asset writes and `install-hooks --plan` to inspect hook writes before applying either surface. `--clients=<list>` is supported independently by both commands.

## Installation

### From source

```sh
git clone https://github.com/ngallodev/codebase-memory-cli.git
cd codebase-memory-cli
scripts/build.sh
./build/c/codebase-memory-cli --help
```

### Setup scripts

macOS/Linux:

```sh
curl -fsSL https://raw.githubusercontent.com/ngallodev/codebase-memory-cli/main/scripts/setup.sh | bash
```

Windows PowerShell:

```powershell
irm https://raw.githubusercontent.com/ngallodev/codebase-memory-cli/main/scripts/setup-windows.ps1 | iex
```

The setup scripts install the executable. They do not install hooks or write
MCP client configuration. Afterward, run `codebase-memory-cli install
--skip-binary` for CLI skills and instructions. Run
`codebase-memory-cli install-hooks` separately only if you explicitly want
CLI-owned agent hooks.

To download the latest published Windows package, install its exact bytes, and
run the five-repository benchmark in one command, review and run the launcher
from the Windows benchmark runbook:

```powershell
$launcher = Join-Path $env:TEMP 'cbm-fetch-benchmark.ps1'
curl.exe -fsSL https://raw.githubusercontent.com/ngallodev/codebase-memory-cli/main/scripts/qualification/fetch-install-run-windows-benchmark.ps1 -o $launcher
pwsh.exe -NoProfile -File $launcher -ReleaseTag <exact-release-tag>
```

It verifies the package against `checksums.txt`, uses the matching source tag,
installs the native executable, and stores timestamped benchmark evidence under
`C:\cbm-benchmark`. See [`docs/WINDOWS_BENCHMARK_RUNBOOK.md`](docs/WINDOWS_BENCHMARK_RUNBOOK.md)
for prerequisites and the full qualification path.

To run the same build, install, and benchmark flow on a GitHub-hosted Windows
runner, manually dispatch the **Manual Windows Benchmark** workflow from the
desired branch. It uploads the complete benchmark evidence as a workflow
artifact and requires no local Windows build.

## Human and machine output

Human-readable output is the default. `--json` is the stable machine-oriented surface for the canonical commands in this slice.

Design rules:

- stdout is reserved for command results;
- progress and diagnostics belong on stderr;
- errors produce non-zero exit codes;
- canonical JSON hides the historical transport envelope;
- result ordering/pagination remain explicit where the underlying operation supports them.

## Project and trust boundaries

Codebase Memory can infer the current repository/project in common cases, but authorization is not inferred.

`allow-root` remains deliberately human-only. An agent/tool caller must not be able to expand its own indexing authorization boundary. `CBM_ALLOWED_ROOT` and the always-on sensitive/root-directory restrictions continue to constrain indexing scope.

See [`docs/CONFIGURATION.md`](docs/CONFIGURATION.md) for cache, runtime, extension mapping, and environment settings.

Release scanning policy: Microsoft `!ml` tolerance is the only tolerated detection.

## Indexing and language support

The existing indexing engine is intentionally unchanged by the CLI-first migration. It parses 162 languages and includes:

- tree-sitter-based extraction across the project’s supported grammar set;
- hybrid LSP semantic enrichment for selected languages;
- persistent local indexes;
- call/import/definition and selected cross-service relationships;
- change detection, architecture, Cypher, ADR, trace ingestion, and other operations exposed through the canonical CLI and its compatibility form.

The first vertical slice does **not** rewrite the graph schema, parser pipeline, store format, or existing indexes.

## Configuration

Common commands:

```sh
codebase-memory-cli config list
codebase-memory-cli config get auto_index
codebase-memory-cli config set auto_index true
codebase-memory-cli config set watcher_enabled false
codebase-memory-cli config reset auto_index
```

Important environment variables include:

| Variable | Purpose |
|---|---|
| `CBM_CACHE_DIR` | Override the local cache/index root. |
| `CBM_ALLOWED_ROOT` | Constrain permissible indexing roots. |
| `CBM_RUNTIME_DIR` | Override the secure local coordination rendezvous parent. |
| `CBM_WORKERS` | Override indexing worker count. |
| `CBM_LOG_LEVEL` | Control runtime logging. |

See [`docs/CONFIGURATION.md`](docs/CONFIGURATION.md) for the full reference.

## Architecture

The intended direction is:

```text
human shell ─┐
agent skill ─┼──> CLI / hooks ──> protocol-neutral operations ──> graph/index engine
agent hooks ─┘
```

The CLI, hooks, daemon, supervised workers, watcher, and UI dispatch through
the operation and runtime layers. Index supervision, project mutation locks,
version-cohort checks, process containment, memory limits, cancellation, store
recovery, and secure local coordination are part of the application itself.

## Build

The native production build is:

```sh
make -f Makefile.cbm cbm
```

or:

```sh
scripts/build.sh
```

The build remains C11-based and uses the existing vendored parser/storage dependencies.

## Security

Codebase Memory indexes source that the current account can read. Treat an allowed repository root as a real data-access boundary.

Key safeguards retained through the CLI migration include:

- human-controlled root enrollment;
- refusal of filesystem roots, broad system trees, home directories, and known credential directories as index roots;
- canonical cache/runtime paths and owner-private local coordination;
- exact-build/version-cohort admission;
- mutation locks around shared project state;
- supervised index workers with cancellation/resource controls;
- ownership-aware integration edits and removals.

See [`SECURITY.md`](SECURITY.md) and [`docs/SECURITY-DISCLOSURE.md`](docs/SECURITY-DISCLOSURE.md).

## Migration documents

- [`docs/CLI_MIGRATION_STATUS.md`](docs/CLI_MIGRATION_STATUS.md) — current port and qualification status.
- [`docs/RELEASE_QUALIFICATION_PLAN.md`](docs/RELEASE_QUALIFICATION_PLAN.md) — build, platform, benchmark, and release gates.
- [`docs/CLI_ONLY_MIGRATION_PLAN.md`](docs/CLI_ONLY_MIGRATION_PLAN.md) — historical rationale and scope of the MCP removal.

## License

MIT. See [`LICENSE`](LICENSE).
