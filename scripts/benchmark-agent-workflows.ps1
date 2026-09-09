[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$Binary,
  [Parameter(Mandatory=$true)][string]$Repo,
  [Parameter(Mandatory=$true)][string]$ResultsDir,
  [string]$SecondaryRepo = "",
  [int]$Repeats = 5,
  [string]$Query = "daemon",
  [string]$Symbol = "main",
  [string]$FilePath = "src/main.c",
  [string]$BaseBranch = "main",
  [string[]]$Operations = @('index','search','architecture','snippet','outline','changes','status')
)
$ErrorActionPreference = 'Stop'
$Binary = (Resolve-Path -LiteralPath $Binary).Path
$Repo = (Resolve-Path -LiteralPath $Repo).Path
if ($SecondaryRepo) { $SecondaryRepo = (Resolve-Path -LiteralPath $SecondaryRepo).Path }
if ($Repeats -lt 1) { throw 'Repeats must be >= 1' }
if (Test-Path -LiteralPath $ResultsDir) {
  if ((Get-ChildItem -Force -LiteralPath $ResultsDir | Measure-Object).Count -gt 0) { throw "ResultsDir must be new/empty: $ResultsDir" }
} else { New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null }
$ResultsDir = (Resolve-Path -LiteralPath $ResultsDir).Path
$env:CBM_CACHE_DIR = Join-Path $ResultsDir 'cache'
New-Item -ItemType Directory -Force -Path $env:CBM_CACHE_DIR | Out-Null
$timings = Join-Path $ResultsDir 'timings.tsv'
"case`trun`telapsed_ms`texit_code" | Set-Content -Encoding utf8 $timings
$failures = [System.Collections.Generic.List[string]]::new()

function Invoke-CbmCase([string]$Label,[int]$Run,[string[]]$Args) {
  $out = Join-Path $ResultsDir "$Label.$Run.json"
  $err = Join-Path $ResultsDir "$Label.$Run.stderr"
  $sw=[Diagnostics.Stopwatch]::StartNew()
  & $Binary @Args 1> $out 2> $err
  $rc=$LASTEXITCODE
  $sw.Stop()
  "$Label`t$Run`t$($sw.ElapsedMilliseconds)`t$rc" | Add-Content -Encoding utf8 $timings
  if ($rc -ne 0) { $script:failures.Add("$Label run $Run exit=$rc") }
  return $rc
}
function Invoke-Repeat([string]$Label,[string[]]$Args) {
  & $Binary @Args *> $null
  $warmRc=$LASTEXITCODE
  if ($warmRc -ne 0) { $script:failures.Add("$Label warmup exit=$warmRc"); return }
  for($i=1;$i -le $Repeats;$i++){ [void](Invoke-CbmCase $Label $i $Args) }
}
function Wants([string]$Name) { return $Operations -contains $Name }

& $Binary daemon stop *> $null
& $Binary daemon start *> $null
if ($LASTEXITCODE -ne 0) { throw 'daemon start failed before benchmark workload' }
try {
  if (-not (Wants 'index')) { throw 'benchmark operations must include index' }
  $indexRc=Invoke-CbmCase 'index' 1 @('index',$Repo,'--mode','full','--json')
  if ($indexRc -ne 0) { throw 'index failed' }
  $index = Get-Content (Join-Path $ResultsDir 'index.1.json') -Raw | ConvertFrom-Json
  $project = [string]$index.project
  if (-not $project) { throw 'index did not return a project name' }
  if (Wants 'search') { Invoke-Repeat 'search' @('search','--project',$project,'--query',$Query,'--limit','20','--json') }
  if (Wants 'architecture') { Invoke-Repeat 'architecture' @('architecture','--project',$project,'--json') }
  if (Wants 'snippet') { Invoke-Repeat 'snippet' @('snippet','--project',$project,'--qualified-name',$Symbol,'--json') }
  if (Wants 'outline') { Invoke-Repeat 'outline' @('outline','--project',$project,'--file-path',$FilePath,'--limit','100','--json') }
  if (Wants 'changes') { Invoke-Repeat 'changes' @('changes','--project',$project,'--scope','files','--base-branch',$BaseBranch,'--json') }
  if (Wants 'status') { Invoke-Repeat 'status' @('status','--project',$project,'--json') }
  if (($Operations -contains 'cross_repo') -and $SecondaryRepo) {
    $secondaryRc=Invoke-CbmCase 'secondary_index' 1 @('index',$SecondaryRepo,'--mode','full','--json')
    $secondaryProject=''
    if ($secondaryRc -eq 0) {
      try { $secondaryProject=[string](Get-Content (Join-Path $ResultsDir 'secondary_index.1.json') -Raw | ConvertFrom-Json).project } catch { $secondaryProject='' }
    }
    if (-not $secondaryProject) { $failures.Add('secondary index did not return a project name') }
    else { [void](Invoke-CbmCase 'cross_repo' 1 @('index',$Repo,'--mode','cross-repo-intelligence','--target-projects',$secondaryProject,'--json')) }
  }

  $allRows = @(Import-Csv $timings -Delimiter "`t")
  $rows = $allRows | Where-Object exit_code -eq '0' | Group-Object case
  "case`truns`tmin_ms`tmedian_ms`tmax_ms" | Set-Content -Encoding utf8 (Join-Path $ResultsDir 'summary.tsv')
  foreach($g in $rows | Sort-Object Name){
    $v=@($g.Group.elapsed_ms | ForEach-Object {[int64]$_} | Sort-Object)
    $mid=$v[[int](($v.Count-1)/2)]
    "$($g.Name)`t$($v.Count)`t$($v[0])`t$mid`t$($v[-1])" | Add-Content -Encoding utf8 (Join-Path $ResultsDir 'summary.tsv')
  }
  $envFile=Join-Path $ResultsDir 'environment.txt'
  $sha=(Get-FileHash -LiteralPath $Binary -Algorithm SHA256).Hash.ToLowerInvariant()
  $hsha=(Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash.ToLowerInvariant()
  @(
    "captured_utc=$([DateTime]::UtcNow.ToString('o'))",
    "binary=$Binary",
    "binary_sha256=$sha",
    "harness_sha256=$hsha",
    "repo=$Repo",
    "repo_commit=$((& git -C $Repo rev-parse HEAD).Trim())",
    "repo_dirty=$([bool](& git -C $Repo status --porcelain))",
    "repo_origin=$((& git -C $Repo remote get-url origin).Trim())",
    "project=$project",
    "repeats=$Repeats",
    "operations=$($Operations -join ',')",
    "query=$Query",
    "symbol=$Symbol",
    "file=$FilePath",
    "base_branch=$BaseBranch",
    "os=$([Environment]::OSVersion.VersionString)",
    "powershell=$($PSVersionTable.PSVersion)",
    "processor=$env:PROCESSOR_IDENTIFIER",
    "logical_processors=$env:NUMBER_OF_PROCESSORS",
    "cache_dir=$env:CBM_CACHE_DIR"
  ) | Set-Content -Encoding utf8 $envFile

  $expectedRepeated=@($Operations | Where-Object { $_ -in @('search','architecture','snippet','outline','changes','status') })
  foreach($case in $expectedRepeated){
    $count=@($allRows | Where-Object { $_.case -eq $case -and $_.exit_code -eq '0' }).Count
    if($count -ne $Repeats){ $failures.Add("$case successful measured runs=$count expected=$Repeats") }
  }
  if($Operations -contains 'cross_repo'){
    $count=@($allRows | Where-Object { $_.case -eq 'cross_repo' -and $_.exit_code -eq '0' }).Count
    if($count -ne 1){ $failures.Add("cross_repo successful measured runs=$count expected=1") }
  }
  if($failures.Count -gt 0){
    $failures | Set-Content -Encoding utf8 (Join-Path $ResultsDir 'FAILURES.txt')
    throw ("benchmark workload failed: " + ($failures -join '; '))
  }
  'PASS' | Set-Content -Encoding ascii (Join-Path $ResultsDir 'RESULT')
  Get-Content (Join-Path $ResultsDir 'summary.tsv')
} finally {
  & $Binary daemon stop *> $null
}
