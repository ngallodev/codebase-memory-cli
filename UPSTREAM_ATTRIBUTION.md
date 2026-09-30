# Upstream attribution

This repository is a **modified fork** of
[DeusData/codebase-memory-mcp](https://github.com/DeusData/codebase-memory-mcp).

## Original project

The original project is authored and maintained upstream by **DeusData**.
GitHub records this repository as a fork of `DeusData/codebase-memory-mcp`,
and the upstream MIT copyright is preserved verbatim in
[`LICENSE`](LICENSE):

> Copyright (c) 2025 DeusData

The upstream project is the source of the original architecture and substantial
implementation represented in this fork, including the code-intelligence
engine, tree-sitter parsing/extraction system, persistent graph/index model,
storage pipeline, daemon/watcher/runtime foundation, broad language support,
query capabilities, and other functionality inherited through the fork.

This repository does **not** claim that those systems were independently
designed or implemented by Nathan Gallo / ngallodev.

## What this fork changes

The fork exists to change the supported product boundary from an MCP-server
integration to a CLI-first local tool. Fork-specific changes include work in
areas such as:

- removing the supported MCP server and third-party MCP integration surface;
- making `codebase-memory-cli` the supported product entry point;
- exposing agent/script-oriented CLI and stable JSON workflows;
- separating CLI-owned skills/hooks from MCP registration behavior;
- adapting installation and integration behavior for restricted environments;
- tightening selected security/authorization behavior around the CLI-first use
  case;
- qualification, compatibility, and benchmark work for the modified fork.

These modifications can be substantial without changing the provenance of the
underlying product: **Codebase Memory CLI remains a derivative fork of
DeusData/codebase-memory-mcp.**

## License and third-party work

The root [`LICENSE`](LICENSE) preserves the upstream MIT license and DeusData
copyright. [`THIRD_PARTY.md`](THIRD_PARTY.md) documents additional vendored and
referenced third-party components inherited by or used within the codebase.

When describing this project publicly, use wording such as:

> Codebase Memory CLI is a modified CLI-first fork of
> DeusData/codebase-memory-mcp. DeusData authored the upstream code-intelligence
> engine and architecture; this fork removes the supported MCP server/integration
> surface and adapts the project for CLI-first use in restricted environments.

Do not describe the repository as a from-scratch implementation or imply
original authorship of the upstream engine or architecture.
