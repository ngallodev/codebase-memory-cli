[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$CandidateBinary,
  [string]$Manifest = 'docs/qualification/BENCHMARK_CORPUS.json',
  [string]$WorkspaceRoot = 'C:\cbm-benchmark\repos',
  [string]$ResultsRoot = 'C:\cbm-benchmark\results',
  [string]$CodebaseMemoryRef = '',
  [string]$FrozenManifestOut = '',
  [int]$Repeats = 5,
  [switch]$Initialize
)
$ErrorActionPreference='Stop'
$repoRoot=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$CandidateBinary=(Resolve-Path -LiteralPath $CandidateBinary).Path
if (-not [IO.Path]::IsPathRooted($Manifest)) { $Manifest=Join-Path $repoRoot $Manifest }
$Manifest=(Resolve-Path -LiteralPath $Manifest).Path
if($Initialize -and -not $FrozenManifestOut){
  $FrozenManifestOut=Join-Path $ResultsRoot 'BENCHMARK_CORPUS.frozen.json'
}
$doc=Get-Content -LiteralPath $Manifest -Raw | ConvertFrom-Json
New-Item -ItemType Directory -Force $WorkspaceRoot,$ResultsRoot | Out-Null
foreach($r in $doc.repositories){
  $path=Join-Path $WorkspaceRoot $r.id
  if(-not (Test-Path -LiteralPath $path)){ git clone --no-checkout $r.remote $path; if($LASTEXITCODE){throw "clone failed: $($r.id)"} }
  if(-not (Test-Path -LiteralPath (Join-Path $path '.git')) -and -not (& git -C $path rev-parse --git-dir 2>$null)) { throw "not a git checkout: $path" }
  git -C $path remote set-url origin $r.remote; if($LASTEXITCODE){throw "remote update failed: $($r.id)"}
  git -C $path fetch --tags --prune origin; if($LASTEXITCODE){throw "fetch failed: $($r.id)"}
  $ref=[string]$r.commit
  if($ref -eq 'TO_BE_PINNED_FROM_FROZEN_CHECKOUT'){
    if($r.id -eq 'codebase-memory-cli' -and $CodebaseMemoryRef){$ref=$CodebaseMemoryRef}
    elseif($r.initialization_ref){$ref=[string]$r.initialization_ref}
    else { throw "No initialization ref for $($r.id)" }
  }
  git -C $path checkout --detach $ref; if($LASTEXITCODE){throw "checkout failed: $($r.id) ref=$ref"}
  git -C $path reset --hard HEAD | Out-Null
  git -C $path clean -ffd | Out-Null
  $dirty=@(& git -C $path status --porcelain)
  if($dirty.Count -ne 0){throw "checkout is dirty after reset/clean: $($r.id)"}
  $r.local_path=$path
  if($Initialize){$r.commit=(& git -C $path rev-parse HEAD).Trim()}
}
$effectiveManifest=$Manifest
if($Initialize){
  $parent=Split-Path -Parent $FrozenManifestOut
  if($parent){New-Item -ItemType Directory -Force $parent | Out-Null}
  $doc | ConvertTo-Json -Depth 12 | Set-Content -Encoding utf8 $FrozenManifestOut
  $effectiveManifest=(Resolve-Path -LiteralPath $FrozenManifestOut).Path
}
& python (Join-Path $repoRoot 'scripts\qualification\validate-corpus-checkouts.py') $effectiveManifest
if($LASTEXITCODE){throw 'corpus validation failed'}
$stamp=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')
$runRoot=Join-Path $ResultsRoot $stamp
New-Item -ItemType Directory -Force $runRoot | Out-Null
$corpusSummary=@()
for($i=0;$i -lt $doc.repositories.Count;$i++){
  $r=$doc.repositories[$i]
  $secondary=''
  if($r.operations -contains 'cross_repo'){
    $secondary=($doc.repositories | Where-Object id -eq 'codebase-memory-cli').local_path
    if($secondary -eq $r.local_path){$secondary=($doc.repositories | Where-Object id -eq 'agent-workflow').local_path}
  }
  $out=Join-Path $runRoot $r.id
  $benchArgs=@(
    '-Binary',$CandidateBinary,
    '-Repo',$r.local_path,
    '-ResultsDir',$out,
    '-Repeats',$Repeats,
    '-Operations',@($r.operations)
  )
  if($r.workload){
    if($r.workload.query){$benchArgs += @('-Query',[string]$r.workload.query)}
    if($r.workload.symbol){$benchArgs += @('-Symbol',[string]$r.workload.symbol)}
    if($r.workload.file_path){$benchArgs += @('-FilePath',[string]$r.workload.file_path)}
    if($r.workload.base_branch){$benchArgs += @('-BaseBranch',[string]$r.workload.base_branch)}
  }
  if($secondary){$benchArgs += @('-SecondaryRepo',$secondary)}
  & (Join-Path $repoRoot 'scripts\benchmark-agent-workflows.ps1') @benchArgs
  if($LASTEXITCODE){throw "benchmark failed: $($r.id)"}
  if(-not (Test-Path -LiteralPath (Join-Path $out 'RESULT')) -or (Get-Content -LiteralPath (Join-Path $out 'RESULT') -Raw).Trim() -ne 'PASS'){
    throw "benchmark did not produce PASS marker: $($r.id)"
  }
  $corpusSummary += [pscustomobject]@{id=$r.id;commit=$r.commit;result='PASS';results=$out}
}
$corpusSummary | ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 (Join-Path $runRoot 'corpus-results.json')
'PASS' | Set-Content -Encoding ascii (Join-Path $runRoot 'RESULT')
Write-Host "Benchmark corpus complete: $runRoot"
Write-Host "Frozen manifest: $effectiveManifest"
