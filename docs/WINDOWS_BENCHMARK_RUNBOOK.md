# Windows RC Benchmark Runbook

This runbook establishes `windows-corpus-1` and captures the first Windows CLI baseline on `luigi.home.arpa` from the exact release-candidate executable. The first qualified CLI release is **BASELINE_ZERO**: its measurements become generation zero; they are not compared to the older MCP-era release.

## Fixed five-repository corpus

The repository identities are fixed. Do not substitute repositories without creating a new corpus generation.

| ID | Repository | Initialization identity for this generation |
|---|---|---|
| `codebase-memory-cli` | `https://github.com/DeusData/codebase-memory-mcp.git` | **Exact RC tag/commit being qualified.** Do not pin this until the RC draft exists. |
| `specgen-aw` | `https://github.com/ngallodev-software/specgen-aw.git` | SpecGen `0.2.1`, `master`; snapshot observed 2026-09-08: `466debfec788c7aeeee858c6b51e98b1b9afe653`. |
| `agent-workflow` | `https://github.com/ngallodev-software/agent-workflow.git` | Agent-Workflow `0.10.0`, `master`; snapshot observed 2026-09-08: `e84925205301ff7169aff5a8cb14d10680c3739c`. |
| `agent-workflow-spec-contracts` | `https://github.com/ngallodev-software/agent-workflow-spec-contracts.git` | released tag `v0.2.1`; commit `0b32e556ef33bd00c0111d30a8b9225eb5196fcd`. |
| `herdr` | `https://github.com/herdrdev/herdr.git` | `master`; snapshot observed 2026-09-08: `68c7b78ec237034cbb0e21c8666842ed7991641d`. |

The two moving `master` references above are **initialization references only**. Once `BENCHMARK_CORPUS.json` is frozen on Luigi, its exact commit SHAs are authoritative even when upstream moves.

## Prerequisites on Luigi

Use native 64-bit **PowerShell 7 or newer**. The qualification orchestrator intentionally rejects Windows PowerShell 5.1 so JSON/evidence UTF-8 encoding is deterministic. Required commands: `git`, `python`, and the candidate `codebase-memory-cli.exe`. Use a local SSD directory, not a network share. Keep AC power connected, disable sleep for the run, close large background workloads, and keep the same Windows power mode for all trials.

Suggested layout:

```text
C:\cbm-benchmark\
  candidate\codebase-memory-cli.exe
  repos\
  results\
  source\                 # checkout of the exact codebase-memory-cli RC source
```

Verify the downloaded candidate before doing anything else:

```powershell
Get-FileHash C:\cbm-benchmark\candidate\codebase-memory-cli.exe -Algorithm SHA256
& C:\cbm-benchmark\candidate\codebase-memory-cli.exe --version
```

Record the hash and ensure it matches the draft-release artifact/evidence.


## Preferred one-command RC qualification

For the first CLI-first Windows RC, use the top-level orchestrator from a **clean checkout of the exact RC source/tag**. It acquires the draft Windows artifact when local paths are not supplied, verifies archive/executable identity, captures machine state, runs portable/recovery/installed-product qualification, executes the retained Windows guards against the supplied executable, freezes/runs the five-repository corpus, and creates promotion-compatible evidence.

```powershell
Set-Location C:\cbm-benchmark\source
.\scripts\qualification\run-windows-rc-qualification.ps1 `
  -ReleaseTag v0.11.0-rc.1 `
  -QualificationRoot C:\cbm-qualification `
  -WorkspaceRoot C:\cbm-benchmark\repos `
  -InitializeCorpus `
  -BenchmarkResult BASELINE_ZERO `
  -Repeats 5
```

When `-CandidateArchive` and `-Checksums` are omitted, authenticated GitHub CLI (`gh`) downloads `codebase-memory-cli-windows-amd64.zip` and `checksums.txt` from the named draft release. To use files copied to Luigi manually, supply both paths explicitly.

The script intentionally installs the exact candidate bytes through `scripts/setup-windows.ps1 -Binary ...`; it never rebuilds the candidate and never substitutes the public `latest` release. The installed-product scenario seeds MCP-owned Claude state, verifies ordinary `install` remains asset-only, invokes `install-hooks` separately, and verifies CLI uninstall preserves the seeded MCP state.

A successful run ends by running `scripts/ci/verify-external-qualification.py` against its own evidence. Add `-UploadEvidence` only after reviewing the evidence tree; this uploads `qualification-manifest.json` and `qualification-summary.md` to the existing draft without publishing it.

The run directory contains `artifact`, `machine`, `portable`, `windows-guards`, `recovery`, `installed`, `benchmark`, and `evidence` subdirectories. Preserve the entire directory unchanged.

## One-time corpus initialization

From the exact RC source checkout:

```powershell
Set-Location C:\cbm-benchmark\source
```

Run the corpus orchestrator in initialization mode. `-CodebaseMemoryRef` must be the exact immutable RC tag or SHA, for example `v0.11.0-rc.1`:

```powershell
.\scripts\qualification\run-windows-benchmark-corpus.ps1 `
  -CandidateBinary C:\cbm-benchmark\candidate\codebase-memory-cli.exe `
  -WorkspaceRoot C:\cbm-benchmark\repos `
  -ResultsRoot C:\cbm-benchmark\results `
  -CodebaseMemoryRef v0.11.0-rc.1 `
  -Repeats 5 `
  -Initialize
```

`-Initialize` performs the one-time freeze:

1. clones missing repositories;
2. fetches tags/refs;
3. checks out each declared initialization ref (and the supplied exact RC ref for self-hosting);
4. hard-resets/cleans each benchmark checkout;
5. writes each local path and exact `HEAD` SHA into a separate frozen manifest (`BENCHMARK_CORPUS.frozen.json`);
6. runs `scripts/qualification/validate-corpus-checkouts.py` against that frozen manifest;
7. executes the benchmark corpus using the exact candidate executable.

The top-level RC orchestrator stores the frozen manifest under its `evidence` directory so the exact RC source checkout remains clean throughout qualification. After the first successful BASELINE_ZERO run, review that evidence manifest and deliberately copy its frozen repository entries into `docs/qualification/BENCHMARK_CORPUS.json` on the development branch for subsequent releases, then commit that change. Do not modify the already-qualified RC tag. Any later repository SHA change requires a new corpus generation rather than silently editing `windows-corpus-1`.

## Subsequent runs

Once a later source revision contains the committed frozen manifest, omit `-Initialize` and do not supply a moving branch as a substitute for a pinned commit:

```powershell
.\scripts\qualification\run-windows-benchmark-corpus.ps1 `
  -CandidateBinary C:\cbm-benchmark\candidate\codebase-memory-cli.exe `
  -WorkspaceRoot C:\cbm-benchmark\repos `
  -ResultsRoot C:\cbm-benchmark\results `
  -Repeats 5
```

Each corpus run creates a timestamped result directory per repository. Each repository trial gets a fresh `CBM_CACHE_DIR` inside its result directory.

## Workload executed for every repository

`scripts/benchmark-agent-workflows.ps1` is the native Windows equivalent of the Linux harness. For each repository it:

1. stops and starts the daemon outside measured read operations;
2. performs one full `index` and records it;
3. extracts the resulting project name;
4. performs one unrecorded warm-up before steady-state read cases;
5. records repeated `search`, `architecture`, `snippet`, `outline`, `changes`, and `status` operations;
6. performs `cross_repo` initialization where the corpus manifest requests it;
7. records stdout JSON and stderr separately for every measured invocation;
8. writes `timings.tsv` and `summary.tsv`;
9. records candidate SHA-256, harness SHA-256, repository SHA/origin/dirty state, Windows/PowerShell/CPU metadata, workload parameters, and cache location in `environment.txt`;
10. stops the daemon at the end.

The default repeated workload is five measured repetitions after a warm-up. Do not change query/symbol/file/base-branch parameters between baseline and candidate comparisons. If a repository requires a repository-specific workload override, document it before generation-zero capture and keep it fixed thereafter.

## Result acceptance checks

For each repository, inspect:

```powershell
Get-Content C:\cbm-benchmark\results\<run>-<repo>\summary.tsv
Get-Content C:\cbm-benchmark\results\<run>-<repo>\environment.txt
Import-Csv C:\cbm-benchmark\results\<run>-<repo>\timings.tsv -Delimiter "`t" | Format-Table
```

Every expected operation should have successful recorded invocations. Inspect all non-empty `.stderr` files and the corresponding JSON output before accepting the run. A fast failure is not a valid performance result.

For `BASELINE_ZERO`, archive the complete raw result tree unchanged. These values become the reference for the next qualified CLI candidate.

## Windows RC qualification sequence

Benchmarking is only one part of the Windows RC gate. For the same exact executable bytes:

1. verify release ZIP/executable hashes;
2. run the native Windows functional/recovery qualification described in `docs/agents/WINDOWS_RELEASE_QUALIFICATION_AGENT.md` and `scripts/test-windows.ps1`;
3. explicitly exercise `install` and confirm it installs assets only;
4. explicitly exercise `install-hooks` and confirm CLI hooks coexist with any installed MCP product;
5. exercise update and uninstall, confirming MCP-owned state is preserved;
6. freeze/validate `windows-corpus-1`;
7. run the benchmark corpus and archive raw evidence;
8. generate `qualification-summary.md` and `qualification-manifest.json` against the exact artifact hashes;
9. attach evidence to the draft release and use the existing promotion workflow.

## Manual fallback

To benchmark one repository without the orchestrator:

```powershell
.\scripts\benchmark-agent-workflows.ps1 `
  -Binary C:\cbm-benchmark\candidate\codebase-memory-cli.exe `
  -Repo C:\cbm-benchmark\repos\agent-workflow `
  -ResultsDir C:\cbm-benchmark\results\manual-agent-workflow `
  -SecondaryRepo C:\cbm-benchmark\repos\codebase-memory-cli `
  -Repeats 5
```

Never reuse a non-empty result directory for another trial.
