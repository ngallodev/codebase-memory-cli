[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$ReleaseTag,
  [string]$CandidateArchive = '',
  [string]$Checksums = '',
  [string]$ExpectedSourceCommit = '',
  [string]$SourceRoot = '',
  [string]$QualificationRoot = 'C:\cbm-qualification',
  [string]$WorkspaceRoot = 'C:\cbm-benchmark\repos',
  [string]$CorpusManifest = 'docs/qualification/BENCHMARK_CORPUS.json',
  [int]$Repeats = 5,
  [ValidateSet('BASELINE_ZERO','PASS')][string]$BenchmarkResult = 'BASELINE_ZERO',
  [switch]$InitializeCorpus,
  [switch]$UploadEvidence,
  [string]$Repository = 'ngallodev/codebase-memory-cli',
  [string]$ExpectedHost = 'luigi.home.arpa'
)
$ErrorActionPreference='Stop'
if($PSVersionTable.PSVersion.Major -lt 7){throw 'Windows RC qualification requires PowerShell 7 or newer for deterministic UTF-8 evidence output'}
$repoRoot=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if(-not $SourceRoot){$SourceRoot=$repoRoot}else{$SourceRoot=(Resolve-Path -LiteralPath $SourceRoot).Path}
if([IO.Path]::GetFullPath($SourceRoot).TrimEnd('\') -ne [IO.Path]::GetFullPath($repoRoot).TrimEnd('\')){
  throw 'SourceRoot must be the same exact checkout that contains this qualification script; cross-checkout qualification is not allowed'
}
$stamp=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')
$runRoot=Join-Path $QualificationRoot (($ReleaseTag -replace '[^A-Za-z0-9._-]','_') + '-' + $stamp)
$artifactDir=Join-Path $runRoot 'artifact'
$machineDir=Join-Path $runRoot 'machine'
$portableDir=Join-Path $runRoot 'portable'
$guardsDir=Join-Path $runRoot 'windows-guards'
$recoveryDir=Join-Path $runRoot 'recovery'
$installedDir=Join-Path $runRoot 'installed'
$benchmarkDir=Join-Path $runRoot 'benchmark'
$evidenceDir=Join-Path $runRoot 'evidence'
New-Item -ItemType Directory -Force $artifactDir,$machineDir,$portableDir,$guardsDir,$recoveryDir,$installedDir,$benchmarkDir,$evidenceDir | Out-Null

function Write-Text([string]$Path,[object]$Value){ $Value | Out-String -Width 4096 | Set-Content -Encoding utf8 -LiteralPath $Path }
function Require-Exit([int]$Code,[string]$What){ if($Code -ne 0){throw "$What failed (exit $Code)"} }
function Invoke-Recorded([string]$Label,[string]$Command,[string[]]$Arguments,[string]$Dir,[switch]$ExpectNonZero){
  New-Item -ItemType Directory -Force $Dir | Out-Null
  $stdout=Join-Path $Dir "$Label.stdout"
  $stderr=Join-Path $Dir "$Label.stderr"
  $started=[DateTime]::UtcNow
  & $Command @Arguments 1> $stdout 2> $stderr
  $rc=$LASTEXITCODE
  [pscustomobject]@{label=$Label;command=$Command;arguments=$Arguments;exit_code=$rc;started_utc=$started.ToString('o');finished_utc=[DateTime]::UtcNow.ToString('o')} |
    ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 -LiteralPath (Join-Path $Dir "$Label.record.json")
  if($ExpectNonZero){if($rc -eq 0){throw "$Label unexpectedly succeeded"}}elseif($rc -ne 0){throw "$Label failed (exit $rc)"}
  return $stdout
}
function Assert-Json([string]$Path,[string]$What){ try{Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json | Out-Null}catch{throw "$What did not produce valid JSON: $Path"} }
function Assert-Contains([string]$Path,[string]$Needle,[string]$What){$text=Get-Content -LiteralPath $Path -Raw;if($text -notlike "*$Needle*"){throw "$What missing expected text: $Needle"}}
function Hash-Lower([string]$Path){return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()}
function Resolve-PublishedHash([string]$ChecksumsPath,[string]$FileName){
  foreach($line in Get-Content -LiteralPath $ChecksumsPath){
    if($line -match '^([0-9A-Fa-f]{64})\s+\*?(.+)$'){
      if([IO.Path]::GetFileName($Matches[2].Trim()) -eq $FileName){return $Matches[1].ToLowerInvariant()}
    }
  }
  throw "checksums file does not contain $FileName"
}
function Save-RelevantEnvironment([string]$Path){
  Get-ChildItem Env: | Where-Object {$_.Name -like 'CBM_*'} | Sort-Object Name | ForEach-Object {"$($_.Name)=$($_.Value)"} | Set-Content -Encoding utf8 -LiteralPath $Path
}

# Identity/source preflight.
$sourceHead=(& git -C $SourceRoot rev-parse HEAD).Trim(); Require-Exit $LASTEXITCODE 'source rev-parse'
if(-not $ExpectedSourceCommit){
  $ExpectedSourceCommit=(& git -C $SourceRoot rev-parse "$ReleaseTag^{commit}" 2>$null).Trim()
  if($LASTEXITCODE -ne 0 -or -not $ExpectedSourceCommit){throw 'ExpectedSourceCommit was not supplied and ReleaseTag could not be resolved in SourceRoot'}
}
if($sourceHead -ne $ExpectedSourceCommit){throw "source checkout mismatch: HEAD=$sourceHead expected=$ExpectedSourceCommit"}
$dirty=@(& git -C $SourceRoot status --porcelain)
if($dirty.Count -ne 0){throw 'source checkout is dirty; qualification requires the exact clean RC source'}

# Acquire exact draft assets if paths were not supplied.
if(-not $CandidateArchive -or -not $Checksums){
  $gh=(Get-Command gh -ErrorAction SilentlyContinue)
  if(-not $gh){throw 'CandidateArchive/Checksums were not supplied and gh is unavailable'}
  & $gh.Source release download $ReleaseTag --repo $Repository --dir $artifactDir --clobber --pattern 'codebase-memory-cli-windows-amd64.zip' --pattern 'checksums.txt'
  Require-Exit $LASTEXITCODE 'gh release download'
  if(-not $CandidateArchive){$CandidateArchive=Join-Path $artifactDir 'codebase-memory-cli-windows-amd64.zip'}
  if(-not $Checksums){$Checksums=Join-Path $artifactDir 'checksums.txt'}
}
$CandidateArchive=(Resolve-Path -LiteralPath $CandidateArchive).Path
$Checksums=(Resolve-Path -LiteralPath $Checksums).Path
$archiveHash=Hash-Lower $CandidateArchive
$publishedHash=Resolve-PublishedHash $Checksums ([IO.Path]::GetFileName($CandidateArchive))
if($archiveHash -ne $publishedHash){throw "candidate archive checksum mismatch: computed=$archiveHash published=$publishedHash"}
$archiveCopy = Join-Path $artifactDir ([IO.Path]::GetFileName($CandidateArchive))
$checksumsCopy = Join-Path $artifactDir 'checksums.txt'
if([IO.Path]::GetFullPath($CandidateArchive) -ne [IO.Path]::GetFullPath($archiveCopy)){Copy-Item -LiteralPath $CandidateArchive -Destination $archiveCopy -Force}
if([IO.Path]::GetFullPath($Checksums) -ne [IO.Path]::GetFullPath($checksumsCopy)){Copy-Item -LiteralPath $Checksums -Destination $checksumsCopy -Force}
$extractDir=Join-Path $artifactDir 'extracted'; New-Item -ItemType Directory -Force $extractDir | Out-Null
Expand-Archive -LiteralPath $CandidateArchive -DestinationPath $extractDir -Force
$executables=@(Get-ChildItem -LiteralPath $extractDir -Recurse -File -Filter 'codebase-memory-cli.exe')
if($executables.Count -ne 1){throw "candidate archive must contain exactly one codebase-memory-cli.exe; found $($executables.Count)"}
$candidate=$executables[0].FullName
$exeHash=Hash-Lower $candidate
$versionOut=Invoke-Recorded 'version' $candidate @('--version') $artifactDir
$normalizedTag=$ReleaseTag.TrimStart('v')
Assert-Contains $versionOut $normalizedTag 'candidate version'
@("release_tag=$ReleaseTag","source_commit=$ExpectedSourceCommit","archive=$CandidateArchive","archive_sha256=$archiveHash","executable=$candidate","executable_sha256=$exeHash") | Set-Content -Encoding utf8 -LiteralPath (Join-Path $artifactDir 'identity.txt')

# Machine state.
Write-Text (Join-Path $machineDir 'computer-info.txt') (Get-ComputerInfo | Select-Object WindowsProductName,WindowsVersion,OsBuildNumber,OsArchitecture,CsName,CsDomain,CsTotalPhysicalMemory)
Write-Text (Join-Path $machineDir 'processor.txt') (Get-CimInstance Win32_Processor | Select-Object Name,NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed)
Write-Text (Join-Path $machineDir 'memory.txt') (Get-CimInstance Win32_ComputerSystem | Select-Object TotalPhysicalMemory)
Write-Text (Join-Path $machineDir 'powershell.txt') $PSVersionTable
Write-Text (Join-Path $machineDir 'power-plan.txt') (& powercfg /GETACTIVESCHEME 2>&1)
Write-Text (Join-Path $machineDir 'drives.txt') (Get-PSDrive -PSProvider FileSystem | Select-Object Name,Root,Used,Free)
Write-Text (Join-Path $machineDir 'git-version.txt') (& git --version 2>&1)
Write-Text (Join-Path $machineDir 'python-version.txt') (& python --version 2>&1)
Write-Text (Join-Path $machineDir 'processes-before.txt') (Get-Process -ErrorAction SilentlyContinue | Where-Object {$_.ProcessName -like '*codebase-memory*'} | Select-Object ProcessName,Id,Path)
Save-RelevantEnvironment (Join-Path $machineDir 'cbm-environment.txt')
$actualHost=''
try{$actualHost=[System.Net.Dns]::GetHostEntry($env:COMPUTERNAME).HostName}catch{$actualHost=$env:COMPUTERNAME}
if($ExpectedHost -and $actualHost.ToLowerInvariant() -ne $ExpectedHost.ToLowerInvariant()){throw "qualification host mismatch: actual=$actualHost expected=$ExpectedHost"}

# Portable smoke and canonical workflow against exact source checkout.
$savedHome=$env:HOME; $savedUserProfile=$env:USERPROFILE; $savedCache=$env:CBM_CACHE_DIR
$portableHome=Join-Path $portableDir 'home'; New-Item -ItemType Directory -Force $portableHome | Out-Null
$env:HOME=$portableHome; $env:USERPROFILE=$portableHome; $env:CBM_CACHE_DIR=Join-Path $portableDir 'cache'
try{
  Invoke-Recorded 'help' $candidate @('--help') $portableDir | Out-Null
  $doctor=Invoke-Recorded 'doctor-json' $candidate @('doctor','--json') $portableDir; Assert-Json $doctor 'doctor --json'
  Invoke-Recorded 'invalid-command' $candidate @('definitely-not-a-command') $portableDir -ExpectNonZero | Out-Null
  Invoke-Recorded 'daemon-start' $candidate @('daemon','start') $portableDir | Out-Null
  Invoke-Recorded 'daemon-status' $candidate @('daemon','status') $portableDir | Out-Null
  $indexOut=Invoke-Recorded 'index' $candidate @('index',$SourceRoot,'--mode','full','--json') $portableDir; Assert-Json $indexOut 'portable index'
  $project=[string](Get-Content -LiteralPath $indexOut -Raw | ConvertFrom-Json).project
  if(-not $project){throw 'portable index did not return project name'}
  $searchOut=Invoke-Recorded 'search' $candidate @('search','--project',$project,'--query','daemon','--limit','5','--json') $portableDir; Assert-Json $searchOut 'portable search'
  $statusOut=Invoke-Recorded 'status' $candidate @('status','--project',$project,'--json') $portableDir; Assert-Json $statusOut 'portable status'
  Invoke-Recorded 'daemon-stop' $candidate @('daemon','stop') $portableDir | Out-Null
} finally {$env:HOME=$savedHome;$env:USERPROFILE=$savedUserProfile;$env:CBM_CACHE_DIR=$savedCache}
$portableResult='PASS'

# Existing native Windows green guards against supplied candidate bytes.
& (Join-Path $repoRoot 'scripts\test-windows.ps1') -GuardsOnly -Binary $candidate *> (Join-Path $guardsDir 'test-windows.log')
Require-Exit $LASTEXITCODE 'Windows guard suite'
$windowsGuardsResult='PASS'

# Explicit recovery/lifecycle cycle in addition to the retained daemon guards.
$env:HOME=$portableHome; $env:USERPROFILE=$portableHome; $env:CBM_CACHE_DIR=Join-Path $recoveryDir 'cache'
try{
  & $candidate daemon stop *> $null
  for($i=1;$i -le 3;$i++){
    Invoke-Recorded "start-$i" $candidate @('daemon','start') $recoveryDir | Out-Null
    Invoke-Recorded "status-$i" $candidate @('daemon','status') $recoveryDir | Out-Null
    Invoke-Recorded "stop-$i" $candidate @('daemon','stop') $recoveryDir | Out-Null
  }
  $doctorRecovery=Invoke-Recorded 'doctor-after-recovery' $candidate @('doctor','--json') $recoveryDir; Assert-Json $doctorRecovery 'doctor after recovery'
} finally {$env:HOME=$savedHome;$env:USERPROFILE=$savedUserProfile;$env:CBM_CACHE_DIR=$savedCache}
$recoveryResult='PASS'

# Installed-product and side-by-side ownership scenario using the exact candidate bytes.
$qualHome=Join-Path $installedDir 'home'; $installRoot=Join-Path $installedDir 'app'; New-Item -ItemType Directory -Force (Join-Path $qualHome '.claude\hooks') | Out-Null
$mcpSettings=Join-Path $qualHome '.claude\settings.json'; $legacyHook=Join-Path $qualHome '.claude\hooks\cbm-session-reminder.cmd'
'{"mcpServers":{"codebase-memory-mcp":{"command":"codebase-memory-mcp"}},"hooks":{"SessionStart":[{"matcher":"startup","hooks":[{"type":"command","command":"echo user-mcp-hook"}]}]}}' | Set-Content -Encoding utf8 -LiteralPath $mcpSettings
'@echo off`r`necho legacy-mcp-owned-hook`r`n' | Set-Content -Encoding ascii -LiteralPath $legacyHook
$legacyHash=Hash-Lower $legacyHook
$env:HOME=$qualHome; $env:USERPROFILE=$qualHome; $env:CBM_CACHE_DIR=Join-Path $installedDir 'cache'
try{
  & (Join-Path $repoRoot 'scripts\setup-windows.ps1') -Binary $candidate -InstallDir $installRoot -NoPathPrompt *> (Join-Path $installedDir 'setup.log')
  Require-Exit $LASTEXITCODE 'candidate install'
  $installedBinary=Join-Path $installRoot 'codebase-memory-cli.exe'
  if((Hash-Lower $installedBinary) -ne $exeHash){throw 'installed executable bytes differ from candidate'}
  Invoke-Recorded 'installed-version' $installedBinary @('--version') $installedDir | Out-Null
  Invoke-Recorded 'asset-install' $installedBinary @('install','-y','--skip-binary','--clients=claude') $installedDir | Out-Null
  $currentHooks=@('codebase-memory-cli-discovery-gate.cmd','codebase-memory-cli-session-reminder.cmd','codebase-memory-cli-subagent-reminder.cmd')
  foreach($name in $currentHooks){if(Test-Path -LiteralPath (Join-Path $qualHome ".claude\hooks\$name")){throw "normal install unexpectedly created hook: $name"}}
  Assert-Contains $mcpSettings 'codebase-memory-mcp' 'asset install MCP preservation'
  if((Hash-Lower $legacyHook) -ne $legacyHash){throw 'asset install modified MCP-owned hook file'}
  Invoke-Recorded 'hook-install' $installedBinary @('install-hooks','--clients=claude') $installedDir | Out-Null
  foreach($name in $currentHooks){if(-not (Test-Path -LiteralPath (Join-Path $qualHome ".claude\hooks\$name"))){throw "explicit hook install did not create: $name"}}
  Assert-Contains $mcpSettings 'codebase-memory-mcp' 'hook install MCP preservation'
  Assert-Contains $mcpSettings 'user-mcp-hook' 'hook install foreign hook preservation'
  if((Hash-Lower $legacyHook) -ne $legacyHash){throw 'explicit CLI hook install modified MCP-owned hook file'}
  $installedDoctor=Invoke-Recorded 'installed-doctor' $installedBinary @('doctor','--json') $installedDir; Assert-Json $installedDoctor 'installed doctor'
  Invoke-Recorded 'uninstall' $installedBinary @('uninstall','-y',"--dir=$installRoot") $installedDir | Out-Null
  Assert-Contains $mcpSettings 'codebase-memory-mcp' 'uninstall MCP preservation'
  Assert-Contains $mcpSettings 'user-mcp-hook' 'uninstall foreign hook preservation'
  if((Hash-Lower $legacyHook) -ne $legacyHash){throw 'CLI uninstall modified MCP-owned hook file'}
} finally {$env:HOME=$savedHome;$env:USERPROFILE=$savedUserProfile;$env:CBM_CACHE_DIR=$savedCache}
$installedResult='PASS'

# Freeze/validate/run the five-repository corpus. Keep the frozen manifest in evidence rather than dirtying SourceRoot.
$frozenManifest=Join-Path $evidenceDir 'BENCHMARK_CORPUS.frozen.json'
$benchArgs=@('-CandidateBinary',$candidate,'-Manifest',$CorpusManifest,'-WorkspaceRoot',$WorkspaceRoot,'-ResultsRoot',$benchmarkDir,'-CodebaseMemoryRef',$ReleaseTag,'-FrozenManifestOut',$frozenManifest,'-Repeats',$Repeats)
if($InitializeCorpus){$benchArgs += '-Initialize'}
& (Join-Path $repoRoot 'scripts\qualification\run-windows-benchmark-corpus.ps1') @benchArgs *> (Join-Path $benchmarkDir 'corpus-run.log')
Require-Exit $LASTEXITCODE 'Windows benchmark corpus'
$corpusResults=@(Get-ChildItem -LiteralPath $benchmarkDir -Recurse -File -Filter 'corpus-results.json')
if($corpusResults.Count -ne 1){throw "benchmark corpus must produce exactly one corpus-results.json; found $($corpusResults.Count)"}
$corpusPass=Join-Path $corpusResults[0].Directory.FullName 'RESULT'
if(-not (Test-Path -LiteralPath $corpusPass) -or (Get-Content -LiteralPath $corpusPass -Raw).Trim() -ne 'PASS'){throw 'benchmark corpus did not produce a PASS root marker'}

# Human summary and promotion-compatible manifest.
$summary=Join-Path $evidenceDir 'qualification-summary.md'
@(
  "# Windows RC Qualification — $ReleaseTag",
  '',
  "- Host: $actualHost",
  "- Source commit: $ExpectedSourceCommit",
  "- Windows archive SHA-256: $archiveHash",
  "- Windows executable SHA-256: $exeHash",
  "- Portable result: $portableResult",
  "- Installed result: $installedResult",
  "- Windows guards result: $windowsGuardsResult",
  "- Recovery result: $recoveryResult",
  "- Benchmark result: $BenchmarkResult",
  "- Corpus generation: windows-corpus-1",
  "- Evidence root: $runRoot",
  '',
  'The exact supplied candidate bytes were used throughout. Normal `install` was verified asset-only; `install-hooks` was exercised separately; seeded MCP-owned state and a foreign hook entry survived CLI install/hooks/uninstall.'
) | Set-Content -Encoding utf8 -LiteralPath $summary
$summaryHash=Hash-Lower $summary
$manifestPath=Join-Path $evidenceDir 'qualification-manifest.json'
[ordered]@{
  schema_version=1
  result='PASS'
  host=$ExpectedHost
  corpus_generation='windows-corpus-1'
  portable_result=$portableResult
  installed_result=$installedResult
  windows_guards_result=$windowsGuardsResult
  recovery_result=$recoveryResult
  benchmark_result=$BenchmarkResult
  summary_sha256=$summaryHash
  release=[ordered]@{tag=$ReleaseTag;source_commit=$ExpectedSourceCommit;windows_archive_sha256=$archiveHash;windows_executable_sha256=$exeHash}
} | ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 -LiteralPath $manifestPath

# Verify the evidence with the same verifier used by promotion before optional upload.
& python (Join-Path $repoRoot 'scripts\ci\verify-external-qualification.py') --manifest $manifestPath --summary $summary --archive $CandidateArchive --checksums $Checksums --expected-tag $ReleaseTag --expected-source-commit $ExpectedSourceCommit --expected-host $ExpectedHost --expected-corpus 'windows-corpus-1'
Require-Exit $LASTEXITCODE 'external qualification evidence verification'
if($UploadEvidence){
  $gh=(Get-Command gh -ErrorAction SilentlyContinue); if(-not $gh){throw 'gh is required for -UploadEvidence'}
  & $gh.Source release upload $ReleaseTag $manifestPath $summary --repo $Repository --clobber
  Require-Exit $LASTEXITCODE 'qualification evidence upload'
}
Write-Host "Windows RC qualification PASS: $runRoot"
Write-Host "Manifest: $manifestPath"
Write-Host "Summary:  $summary"
