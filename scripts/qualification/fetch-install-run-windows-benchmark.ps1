[CmdletBinding()]
param(
  [string]$Repository = 'ngallodev/codebase-memory-cli',
  [string]$ReleaseTag = '',
  [string]$Root = 'C:\cbm-benchmark',
  [int]$Repeats = 5
)

$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSVersion.Major -lt 7) { throw 'PowerShell 7 or newer is required' }
foreach ($command in @('git.exe', 'python.exe')) {
  if (-not (Get-Command $command -ErrorAction SilentlyContinue)) { throw "$command is required" }
}

$stamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')
$runRoot = Join-Path $Root "run-$stamp"
$sourceRoot = Join-Path $runRoot 'source'
$artifactRoot = Join-Path $runRoot 'artifact'
$installRoot = Join-Path $runRoot 'install'
$workspaceRoot = Join-Path $runRoot 'repos'
$resultsRoot = Join-Path $runRoot 'results'
New-Item -ItemType Directory -Force $artifactRoot, $installRoot, $workspaceRoot, $resultsRoot | Out-Null

$apiRoot = "https://api.github.com/repos/$Repository/releases"
$release = if ($ReleaseTag) {
  Invoke-RestMethod -Uri "$apiRoot/tags/$ReleaseTag" -Headers @{ 'User-Agent' = 'codebase-memory-cli-benchmark' }
} else {
  Invoke-RestMethod -Uri "$apiRoot/latest" -Headers @{ 'User-Agent' = 'codebase-memory-cli-benchmark' }
}
$ReleaseTag = [string]$release.tag_name
if (-not $ReleaseTag) { throw 'GitHub release has no tag name' }
$zipAsset = @($release.assets | Where-Object name -eq 'codebase-memory-cli-windows-amd64.zip')
$sumAsset = @($release.assets | Where-Object name -eq 'checksums.txt')
if ($zipAsset.Count -ne 1 -or $sumAsset.Count -ne 1) { throw "release $ReleaseTag is missing the Windows package or checksums.txt" }
$zipPath = Join-Path $artifactRoot $zipAsset[0].name
$sumPath = Join-Path $artifactRoot $sumAsset[0].name
Invoke-WebRequest -Uri $zipAsset[0].browser_download_url -OutFile $zipPath
Invoke-WebRequest -Uri $sumAsset[0].browser_download_url -OutFile $sumPath
$publishedHash = (Select-String -LiteralPath $sumPath -Pattern "^([0-9A-Fa-f]{64})\s+\*?$([regex]::Escape($zipAsset[0].name))$").Matches | Select-Object -First 1
if (-not $publishedHash) { throw "checksums.txt has no entry for $($zipAsset[0].name)" }
$actualHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actualHash -ne $publishedHash.Groups[1].Value.ToLowerInvariant()) { throw 'Windows package checksum mismatch' }

$extractRoot = Join-Path $artifactRoot 'extracted'
Expand-Archive -LiteralPath $zipPath -DestinationPath $extractRoot
$candidate = @(Get-ChildItem -LiteralPath $extractRoot -Recurse -File -Filter 'codebase-memory-cli.exe')
if ($candidate.Count -ne 1) { throw "expected one codebase-memory-cli.exe, found $($candidate.Count)" }

& git.exe clone --depth 1 --branch $ReleaseTag "https://github.com/$Repository.git" $sourceRoot
if ($LASTEXITCODE) { throw "could not clone source tag $ReleaseTag" }
$sourceCommit = (& git.exe -C $sourceRoot rev-parse HEAD).Trim()

$setup = Join-Path $sourceRoot 'scripts\setup-windows.ps1'
& pwsh.exe -NoProfile -File $setup -Binary $candidate[0].FullName -InstallDir $installRoot -NoPathPrompt
if ($LASTEXITCODE) { throw 'binary installation failed' }
$installed = Join-Path $installRoot 'codebase-memory-cli.exe'

$corpus = Join-Path $sourceRoot 'scripts\qualification\run-windows-benchmark-corpus.ps1'
& pwsh.exe -NoProfile -File $corpus `
  -CandidateBinary $installed `
  -WorkspaceRoot $workspaceRoot `
  -ResultsRoot $resultsRoot `
  -CodebaseMemoryRef $sourceCommit `
  -Repeats $Repeats `
  -Initialize
if ($LASTEXITCODE) { throw 'Windows benchmark failed' }

Write-Host "Benchmark complete: $runRoot"
Write-Host "Release tag: $ReleaseTag"
Write-Host "Source commit: $sourceCommit"
