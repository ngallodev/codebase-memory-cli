# Pending native Windows qualification

These checks have not run on this Linux host. Run from a fresh checkout of the
candidate branch using native PowerShell and the documented MSYS2 toolchain.
The benchmark helper resets/cleans its corpus checkouts, so use the fresh,
run-owned `corpus-repos` directory below, never an existing development tree.

```powershell
$ErrorActionPreference = 'Stop'
$diag = Join-Path $PWD ('diagnostics/sync-13a-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
if (Test-Path -LiteralPath $diag) { throw 'Choose a fresh diagnostics directory' }
New-Item -ItemType Directory -Path $diag | Out-Null
Start-Transcript -Path (Join-Path $diag 'transcript.txt')
try {
    git rev-parse HEAD | Set-Content (Join-Path $diag 'source-revision.txt')
    git status --porcelain | Set-Content (Join-Path $diag 'git-status.txt')
    & scripts/build-windows.ps1 -Jobs 8 *>&1 | Tee-Object (Join-Path $diag 'build.log')
    if ($LASTEXITCODE) { throw 'Native build failed; inspect build.log' }
    & scripts/test-windows.ps1 -GuardsOnly -Binary build/c/codebase-memory-cli.exe *>&1 |
        Tee-Object (Join-Path $diag 'guards.log')
    if ($LASTEXITCODE) { throw 'Native product guards failed; inspect guards.log' }
    & scripts/qualification/run-windows-benchmark-corpus.ps1 `
        -CandidateBinary build/c/codebase-memory-cli.exe `
        -Manifest benchmark-runs/linux/20260911T045124Z/BENCHMARK_CORPUS.frozen.json `
        -WorkspaceRoot (Join-Path $diag 'corpus-repos') `
        -ResultsRoot (Join-Path $diag 'benchmark-results') -Repeats 5 *>&1 |
        Tee-Object (Join-Path $diag 'benchmark.log')
    if ($LASTEXITCODE) { throw 'Pinned-corpus qualification failed; inspect benchmark.log' }
} finally {
    Stop-Transcript
}
```

Keep stdout/stderr, source revision and benchmark JSON with the diagnostics.
This is a reproducible follow-up, not evidence of Windows success. Later batches
that introduce Windows SQLite offsets must add a real >4 GiB write/reopen check;
13A does not claim that later fix. Compare Linux and Windows against the same
frozen manifest pins, not moving upstream branches.
