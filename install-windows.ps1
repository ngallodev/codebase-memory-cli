# install-windows.ps1 - offline installer for the extracted Windows release zip.
#
# Run it from a terminal inside the extracted archive directory:
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\install-windows.ps1 [options]
#
# It never downloads anything. It installs the codebase-memory-cli.exe that sits
# beside it, removes earlier installs, and registers the app in Windows
# Settings > Apps so it can be uninstalled from there. Current-user only; no
# administrator rights and no machine-wide (HKLM) changes.
#
# Works on Windows PowerShell 5.1 and PowerShell 7. Keep this file ASCII-only.

[CmdletBinding()]
param(
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\codebase-memory-cli",
    [switch]$SkipConfig,
    [string]$CacheDir,
    [string]$RuntimeDir,
    [switch]$Yes,
    [switch]$Help
)

$ErrorActionPreference = 'Stop'

if ($Help) {
    Write-Host @'
Usage: powershell -NoProfile -ExecutionPolicy Bypass -File .\install-windows.ps1 [options]

Installs the codebase-memory-cli.exe next to this script (no download) and
registers it in Windows Settings > Apps.

Options:
  -InstallDir <path>   Install directory.
                       Default: %LOCALAPPDATA%\Programs\codebase-memory-cli
  -SkipConfig          Do not install agent skills or instructions.
  -CacheDir <path>     Create an owner-private cache directory and set it as the
                       persistent user variable CBM_CACHE_DIR.
  -RuntimeDir <path>   Create an owner-private runtime directory and set it as
                       the persistent user variable CBM_RUNTIME_DIR.
  -Yes                 Do not prompt; also removes other copies found on PATH.
  -Help                Show this text.
'@
    exit 0
}

$BinName = 'codebase-memory-cli.exe'
$AppKeyName = 'codebase-memory-cli'
$UninstallRoot = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall'
$AppKey = "$UninstallRoot\$AppKeyName"
$InnoKey = "$UninstallRoot\{FEB1E0BB-E136-4164-AD3D-0E5CCBB15673}_is1"
$AppUrl = 'https://github.com/ngallodev/codebase-memory-cli'

function Write-Step([string]$Message) { Write-Host $Message -ForegroundColor White }
function Write-Warn([string]$Message) { Write-Host "warning: $Message" -ForegroundColor Yellow }

function Get-FullPath([string]$Path) {
    return [System.IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
}

function Test-SamePath([string]$A, [string]$B) {
    return ((Get-FullPath $A) -ieq (Get-FullPath $B))
}

function Test-PathInside([string]$Child, [string]$Parent) {
    $c = Get-FullPath $Child
    $p = Get-FullPath $Parent
    return ($c -ieq $p) -or $c.StartsWith($p + '\', [System.StringComparison]::OrdinalIgnoreCase)
}

function Confirm-Action([string]$Prompt) {
    if ($Yes) { return $true }
    try { $answer = Read-Host "$Prompt [y/N]" } catch { return $false }
    return ($answer -match '^(y|yes)$')
}

# Run a native command with its output shown. Windows PowerShell 5.1 turns
# redirected native stderr into ErrorRecords, which would abort a HEALTHY run
# under ErrorActionPreference=Stop, so relax it here; failure detection stays
# on the exit code. LASTEXITCODE is pre-seeded so a binary that fails to START
# can never read as success. Returns the exit code.
function Invoke-Native([string]$Exe, [string[]]$Arguments) {
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $global:LASTEXITCODE = 1
        # Stringify so stderr lines print as plain text, not as red
        # NativeCommandError records (Windows PowerShell 5.1).
        & $Exe @Arguments 2>&1 | ForEach-Object { "$_" } | Out-Host
        return $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previous
    }
}

# Same relaxation, capturing output. Returns @{ Code = <int>; Output = <string> }.
function Get-NativeOutput([string]$Exe, [string[]]$Arguments) {
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $global:LASTEXITCODE = 1
        $text = (& $Exe @Arguments 2>&1 | ForEach-Object { "$_" } | Out-String).Trim()
        return @{ Code = $LASTEXITCODE; Output = $text }
    } finally {
        $ErrorActionPreference = $previous
    }
}

function Stop-Daemon([string]$Dir) {
    $exe = Join-Path $Dir $BinName
    if (Test-Path -LiteralPath $exe -PathType Leaf) {
        try { [void](Get-NativeOutput $exe @('daemon', 'stop')) } catch { }
    }
}

# Delete files an earlier install can leave in a directory, then the directory
# if it is empty. Best effort: a locked file is reported, not fatal.
function Remove-CbmLeftovers([string]$Dir) {
    if (-not (Test-Path -LiteralPath $Dir -PathType Container)) { return }
    $names = @('LICENSE', 'THIRD_PARTY_NOTICES.md', 'install.ps1', 'install-windows.ps1',
               'uninstall-windows.ps1', $BinName)
    foreach ($name in $names) {
        $path = Join-Path $Dir $name
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            try { Remove-Item -LiteralPath $path -Force -ErrorAction Stop }
            catch { Write-Warn "could not remove $path : $($_.Exception.Message)" }
        }
    }
    Get-ChildItem -LiteralPath $Dir -Filter "$BinName.retired-*" -Force -ErrorAction SilentlyContinue |
        ForEach-Object {
            try { Remove-Item -LiteralPath $_.FullName -Force -ErrorAction Stop }
            catch { Write-Warn "could not remove $($_.FullName) : $($_.Exception.Message)" }
        }
    if (-not (Get-ChildItem -LiteralPath $Dir -Force -ErrorAction SilentlyContinue)) {
        try { Remove-Item -LiteralPath $Dir -Force -ErrorAction Stop } catch { }
    }
}

# Remove one directory from the persistent current-user PATH, leaving every
# other segment unchanged and keeping the value's registry type.
function Remove-UserPathSegment([string]$Dir) {
    try {
        $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('Environment', $true)
        if ($null -eq $key) { return }
        try {
            $raw = [string]$key.GetValue('Path', '', [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
            if (-not $raw) { return }
            $kind = $key.GetValueKind('Path')
            $kept = @()
            $removed = $false
            foreach ($segment in ($raw -split ';')) {
                $match = $false
                if (-not [string]::IsNullOrWhiteSpace($segment)) {
                    try {
                        $expanded = [Environment]::ExpandEnvironmentVariables($segment.Trim().Trim('"'))
                        $match = Test-SamePath $expanded $Dir
                    } catch { }
                }
                if ($match) { $removed = $true } else { $kept += $segment }
            }
            if ($removed) { $key.SetValue('Path', ($kept -join ';'), $kind) }
        } finally {
            $key.Close()
        }
    } catch {
        Write-Warn "could not update the user PATH for $Dir : $($_.Exception.Message)"
    }
}

function Get-RegistryString([string]$Key, [string]$Name) {
    if (-not (Test-Path -LiteralPath $Key)) { return $null }
    $item = Get-ItemProperty -LiteralPath $Key -ErrorAction SilentlyContinue
    if ($null -eq $item) { return $null }
    $prop = $item.PSObject.Properties[$Name]
    if ($null -eq $prop) { return $null }
    return [string]$prop.Value
}

function New-PrivateDirectory([string]$Path) {
    if (-not [System.IO.Path]::IsPathRooted($Path)) {
        throw "directory must be an absolute path: $Path"
    }
    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
    $sid = [System.Security.Principal.WindowsIdentity]::GetCurrent().User
    $acl = New-Object System.Security.AccessControl.DirectorySecurity
    # Protected: drop inherited entries so only the current user remains.
    $acl.SetAccessRuleProtection($true, $false)
    $acl.SetOwner($sid)
    $rule = New-Object System.Security.AccessControl.FileSystemAccessRule(
        $sid,
        [System.Security.AccessControl.FileSystemRights]::FullControl,
        ([System.Security.AccessControl.InheritanceFlags]'ContainerInherit,ObjectInherit'),
        [System.Security.AccessControl.PropagationFlags]::None,
        [System.Security.AccessControl.AccessControlType]::Allow)
    $acl.AddAccessRule($rule)
    Set-Acl -LiteralPath $Path -AclObject $acl
}

# The binary validates the directory it is installed FROM and refuses an
# inherited write grant to another account there (for example CREATOR OWNER
# or an agent-sandbox group on the extracted folder). Stage the executable in
# a fresh owner-private directory under %TEMP%, as install.ps1 does, and
# install from that copy.
function New-StagingDirectory {
    $parent = [System.IO.Path]::GetTempPath()
    for ($attempt = 0; $attempt -lt 32; $attempt++) {
        $candidate = Join-Path $parent ('cbm-install-' + [guid]::NewGuid().ToString('N'))
        try {
            New-Item -ItemType Directory -Path $candidate -ErrorAction Stop | Out-Null
        } catch [System.IO.IOException] {
            continue  # never adopt a path another process created
        }
        New-PrivateDirectory $candidate
        return $candidate
    }
    throw 'could not create a private staging directory under %TEMP%'
}

# The uninstaller written next to the installed binary. Single-quoted here-string:
# nothing in it is expanded while this script runs.
$UninstallScript = @'
# uninstall-windows.ps1 - removes Codebase Memory CLI installed by install-windows.ps1.
# Registered in Windows Settings > Apps; can also be run directly:
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\uninstall-windows.ps1 [-Quiet] [-DeleteIndexes]
[CmdletBinding()]
param(
    [switch]$Quiet,
    [switch]$DeleteIndexes
)

$ErrorActionPreference = 'Stop'

$Dir = $PSScriptRoot
$BinName = 'codebase-memory-cli.exe'
$Exe = Join-Path $Dir $BinName
$AppKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\codebase-memory-cli'
$Failed = New-Object System.Collections.ArrayList

function Invoke-Native([string]$Path, [string[]]$Arguments) {
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $global:LASTEXITCODE = 1
        & $Path @Arguments 2>&1 | ForEach-Object { "$_" } | Out-Host
        return $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previous
    }
}

function Remove-Quietly([string]$Path) {
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        try { Remove-Item -LiteralPath $Path -Force -ErrorAction Stop }
        catch { [void]$Failed.Add("could not remove $Path : $($_.Exception.Message)") }
    }
}

try {
    if ([string]::IsNullOrEmpty($Dir)) { throw 'cannot determine the install directory' }
    # A console started from Settings may have the install dir as its working
    # directory, which would keep the directory from being removed.
    Set-Location -LiteralPath ([System.IO.Path]::GetTempPath())

    Write-Host "Uninstalling Codebase Memory CLI from $Dir"

    if (Test-Path -LiteralPath $Exe -PathType Leaf) {
        try { [void](Invoke-Native $Exe @('daemon', 'stop')) } catch { }
        $uninstallArgs = @('uninstall', '-y', "--dir=$Dir")
        if ($DeleteIndexes) { $uninstallArgs += '--delete-indexes' }
        $code = Invoke-Native $Exe $uninstallArgs
        if ($code -ne 0) {
            [void]$Failed.Add("$BinName uninstall exited with code $code")
        }
    }

    Remove-Quietly (Join-Path $Dir 'LICENSE')
    Remove-Quietly (Join-Path $Dir 'THIRD_PARTY_NOTICES.md')
    Remove-Quietly (Join-Path $Dir 'install.ps1')
    Remove-Quietly (Join-Path $Dir 'install-windows.ps1')
    Get-ChildItem -LiteralPath $Dir -Filter "$BinName.retired-*" -Force -ErrorAction SilentlyContinue |
        ForEach-Object { Remove-Quietly $_.FullName }
    Remove-Quietly $Exe

    $binaryGone = -not (Test-Path -LiteralPath $Exe)

    if ($binaryGone) {
        # Only the variables install-windows.ps1 itself set are removed.
        $managed = $null
        if (Test-Path -LiteralPath $AppKey) {
            $prop = (Get-ItemProperty -LiteralPath $AppKey).PSObject.Properties['CbmManagedEnv']
            if ($null -ne $prop) { $managed = [string]$prop.Value }
        }
        if ($managed) {
            foreach ($name in ($managed -split ';')) {
                if ($name -ceq 'CBM_CACHE_DIR' -or $name -ceq 'CBM_RUNTIME_DIR') {
                    [Environment]::SetEnvironmentVariable($name, $null, 'User')
                    Write-Host "Removed user environment variable $name"
                }
            }
        }
        if (Test-Path -LiteralPath $AppKey) {
            Remove-Item -LiteralPath $AppKey -Recurse -Force
            Write-Host 'Removed the Settings > Apps entry'
        }
    } else {
        [void]$Failed.Add("$Exe is still present; the Settings > Apps entry was kept so you can retry")
    }

    # The script is already parsed, so deleting itself is safe.
    Remove-Quietly $PSCommandPath
    if (Test-Path -LiteralPath $Dir -PathType Container) {
        if (-not (Get-ChildItem -LiteralPath $Dir -Force -ErrorAction SilentlyContinue)) {
            try { Remove-Item -LiteralPath $Dir -Force -ErrorAction Stop }
            catch { [void]$Failed.Add("could not remove directory $Dir : $($_.Exception.Message)") }
        } else {
            Write-Host "Left in ${Dir}:"
            Get-ChildItem -LiteralPath $Dir -Force | ForEach-Object { Write-Host "  $($_.Name)" }
        }
    }
} catch {
    [void]$Failed.Add($_.Exception.Message)
}

if ($Failed.Count -gt 0) {
    foreach ($message in $Failed) { Write-Host "error: $message" -ForegroundColor Red }
    if (-not $Quiet) { Read-Host 'Press Enter to close' | Out-Null }
    exit 1
}

Write-Host 'Codebase Memory CLI was uninstalled. Project indexes were kept unless -DeleteIndexes was given.'
if (-not $Quiet) { Start-Sleep -Seconds 3 }
exit 0
'@

$StagingDir = $null
try {
    # ---- 1. Preflight ------------------------------------------------------
    $SourceDir = $PSScriptRoot
    if ([string]::IsNullOrEmpty($SourceDir)) {
        throw 'run this script with -File from the extracted release directory'
    }
    $SourceExe = Join-Path $SourceDir $BinName
    if (-not (Test-Path -LiteralPath $SourceExe -PathType Leaf)) {
        throw "$BinName was not found next to this script ($SourceDir). Run install-windows.ps1 from inside the extracted release zip."
    }
    if ([string]::IsNullOrWhiteSpace($InstallDir)) { throw '-InstallDir must not be empty' }
    if (-not [System.IO.Path]::IsPathRooted($InstallDir)) {
        throw "-InstallDir must be an absolute path: $InstallDir"
    }
    $InstallDir = Get-FullPath $InstallDir
    if (Test-PathInside $SourceDir $InstallDir) {
        throw "this script is inside the install directory ($InstallDir). Extract the release zip to another folder and run it from there."
    }

    $isAdmin = $false
    try {
        $principal = New-Object System.Security.Principal.WindowsPrincipal(
            [System.Security.Principal.WindowsIdentity]::GetCurrent())
        $isAdmin = $principal.IsInRole([System.Security.Principal.WindowsBuiltInRole]::Administrator)
    } catch { }
    if ($isAdmin) {
        Write-Warn 'running elevated: this installs for the elevated user profile, not for other accounts.'
    }

    # PE header: e_lfanew at 0x3C, then "PE\0\0" and the Machine field.
    $stream = [System.IO.File]::OpenRead($SourceExe)
    try {
        $header = New-Object byte[] 4
        [void]$stream.Seek(0x3C, [System.IO.SeekOrigin]::Begin)
        if ($stream.Read($header, 0, 4) -ne 4) { throw "$BinName is not a valid executable" }
        $peOffset = [BitConverter]::ToInt32($header, 0)
        if ($peOffset -lt 0 -or $peOffset -gt ($stream.Length - 6)) { throw "$BinName is not a valid executable" }
        $pe = New-Object byte[] 6
        [void]$stream.Seek($peOffset, [System.IO.SeekOrigin]::Begin)
        if ($stream.Read($pe, 0, 6) -ne 6 -or $pe[0] -ne 0x50 -or $pe[1] -ne 0x45 -or $pe[2] -ne 0 -or $pe[3] -ne 0) {
            throw "$BinName is not a valid Windows executable"
        }
        $machine = [BitConverter]::ToUInt16($pe, 4)
    } finally {
        $stream.Dispose()
    }
    switch ($machine) {
        0x8664 { $binaryArch = 'amd64' }
        0xAA64 { $binaryArch = 'arm64' }
        default { throw ('unsupported executable architecture (PE machine 0x{0:X4})' -f $machine) }
    }
    $osArchRaw = $env:PROCESSOR_ARCHITEW6432
    if ([string]::IsNullOrEmpty($osArchRaw)) { $osArchRaw = $env:PROCESSOR_ARCHITECTURE }
    switch ($osArchRaw) {
        'AMD64' { $osArch = 'amd64' }
        'ARM64' { $osArch = 'arm64' }
        default { $osArch = $osArchRaw }
    }
    if ($binaryArch -eq 'arm64' -and $osArch -ne 'arm64') {
        throw "this is the arm64 build but the OS is $osArch. Use the amd64 release zip."
    }
    if ($binaryArch -eq 'amd64' -and $osArch -eq 'arm64') {
        Write-Warn 'installing the amd64 build on an ARM64 machine; it runs under emulation. The arm64 zip is faster.'
    }

    # Files from a downloaded zip carry Mark-of-the-Web; clear it. Best effort.
    Get-ChildItem -LiteralPath $SourceDir -File -ErrorAction SilentlyContinue | ForEach-Object {
        try { Unblock-File -LiteralPath $_.FullName -ErrorAction Stop } catch { }
    }

    $probe = Get-NativeOutput $SourceExe @('--version')
    if ($probe.Code -ne 0) {
        throw "$BinName --version failed (exit code $($probe.Code)): $($probe.Output)"
    }
    $DisplayVersion = $probe.Output
    if ($probe.Output -match '(\d+\.\d+\.\d+[0-9A-Za-z.+\-]*)') { $DisplayVersion = $Matches[1] }
    Write-Step "Verified $BinName $DisplayVersion ($binaryArch)"

    # ---- 2. Optional cache/runtime directories ----------------------------
    $PriorManaged = @()
    $existing = Get-RegistryString $AppKey 'CbmManagedEnv'
    if ($existing) { $PriorManaged = @($existing -split ';' | Where-Object { $_ }) }
    $ManagedEnv = New-Object System.Collections.ArrayList
    foreach ($name in $PriorManaged) { if (-not $ManagedEnv.Contains($name)) { [void]$ManagedEnv.Add($name) } }

    $envRequests = @(
        @{ Name = 'CBM_CACHE_DIR'; Path = $CacheDir },
        @{ Name = 'CBM_RUNTIME_DIR'; Path = $RuntimeDir }
    )
    foreach ($request in $envRequests) {
        if ([string]::IsNullOrWhiteSpace($request.Path)) { continue }
        $full = Get-FullPath $request.Path
        New-PrivateDirectory $full
        [Environment]::SetEnvironmentVariable($request.Name, $full, 'User')
        Set-Item -LiteralPath "Env:$($request.Name)" -Value $full
        if (-not $ManagedEnv.Contains($request.Name)) { [void]$ManagedEnv.Add($request.Name) }
        Write-Step "Set user environment variable $($request.Name) = $full (owner-private)"
    }

    # ---- 3. Remove prior versions -----------------------------------------
    # 3a. Inno Setup install of the same app.
    if (Test-Path -LiteralPath $InnoKey) {
        Write-Step 'Removing the earlier Setup-program install...'
        $uninstallString = Get-RegistryString $InnoKey 'UninstallString'
        if (-not $uninstallString) { $uninstallString = Get-RegistryString $InnoKey 'QuietUninstallString' }
        $uninsExe = $null
        if ($uninstallString -match '^\s*"([^"]+)"') { $uninsExe = $Matches[1] }
        elseif ($uninstallString -match '^\s*(.+?\.exe)') { $uninsExe = $Matches[1] }
        if (-not $uninsExe -or -not (Test-Path -LiteralPath $uninsExe -PathType Leaf)) {
            throw 'the earlier Setup install is registered but its uninstaller is missing. Remove "Codebase Memory CLI" in Settings > Apps, then re-run.'
        }
        Stop-Daemon (Get-RegistryString $InnoKey 'InstallLocation')
        Start-Process -FilePath $uninsExe -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART') | Out-Null
        # The Inno uninstaller copies itself to %TEMP% and relaunches, so the
        # process started above exits early. Wait for its registry key to vanish.
        $deadline = (Get-Date).AddSeconds(120)
        while ((Test-Path -LiteralPath $InnoKey) -and (Get-Date) -lt $deadline) {
            Start-Sleep -Seconds 1
        }
        if (Test-Path -LiteralPath $InnoKey) {
            throw 'timed out after 120 seconds waiting for the earlier Setup install to uninstall. Remove "Codebase Memory CLI" in Settings > Apps, then re-run.'
        }
        Write-Step 'Earlier Setup install removed.'
    }

    # 3b. A previous registration by this script at a different location.
    $oldLocation = Get-RegistryString $AppKey 'InstallLocation'
    if ($oldLocation -and -not (Test-SamePath $oldLocation $InstallDir)) {
        Write-Step "Removing the earlier install at $oldLocation..."
        $oldExe = Join-Path $oldLocation $BinName
        if (Test-Path -LiteralPath $oldExe -PathType Leaf) {
            Stop-Daemon $oldLocation
            $code = Invoke-Native $oldExe @('uninstall', '-y', "--dir=$oldLocation")
            if ($code -ne 0) { Write-Warn "uninstall of $oldLocation exited with code $code" }
        }
        Remove-CbmLeftovers $oldLocation
        Remove-Item -LiteralPath $AppKey -Recurse -Force -ErrorAction SilentlyContinue
    }

    # 3c. Other copies on the current-user PATH.
    $rawPath = ''
    try {
        $rawPath = [string](Get-Item -LiteralPath 'HKCU:\Environment').GetValue(
            'Path', '', [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
    } catch { }
    $others = New-Object System.Collections.ArrayList
    foreach ($entry in ($rawPath -split ';')) {
        if ([string]::IsNullOrWhiteSpace($entry)) { continue }
        try { $dir = Get-FullPath ([Environment]::ExpandEnvironmentVariables($entry.Trim().Trim('"'))) }
        catch { continue }
        if (Test-SamePath $dir $InstallDir) { continue }
        if (-not (Test-Path -LiteralPath (Join-Path $dir $BinName) -PathType Leaf)) { continue }
        $known = $false
        foreach ($o in $others) { if (Test-SamePath $o $dir) { $known = $true } }
        if (-not $known) { [void]$others.Add($dir) }
    }
    if ($others.Count -gt 0) {
        Write-Host 'Other codebase-memory-cli.exe copies found on your user PATH:'
        foreach ($o in $others) { Write-Host "  $o" }
        if (Confirm-Action 'Remove these copies?') {
            foreach ($o in $others) {
                # Do NOT run that copy's `uninstall --dir`: it removes its whole
                # directory from the user PATH, and these directories can be
                # shared bin folders (for example ~\.local\bin) holding other
                # tools. Delete only the Codebase Memory files; drop the
                # directory and its PATH segment only when nothing else is left.
                Stop-Daemon $o
                $ours = @('LICENSE', 'THIRD_PARTY_NOTICES.md', 'install.ps1', 'install-windows.ps1',
                          'uninstall-windows.ps1', $BinName)
                $foreign = @(Get-ChildItem -LiteralPath $o -Force -ErrorAction SilentlyContinue |
                    Where-Object { -not ($ours -contains $_.Name) -and -not $_.Name.StartsWith("$BinName.retired-") })
                if ($foreign.Count -eq 0) {
                    # A dedicated old install directory: remove all of it.
                    Remove-CbmLeftovers $o
                } else {
                    # A shared directory: remove only the executable and its
                    # retired copies; generic names like LICENSE may belong to
                    # another tool.
                    foreach ($f in @(Get-ChildItem -LiteralPath $o -Force -ErrorAction SilentlyContinue |
                            Where-Object { $_.Name -ieq $BinName -or $_.Name.StartsWith("$BinName.retired-") })) {
                        try { Remove-Item -LiteralPath $f.FullName -Force -ErrorAction Stop }
                        catch { Write-Warn "could not remove $($f.FullName) : $($_.Exception.Message)" }
                    }
                }
                if (Test-Path -LiteralPath (Join-Path $o $BinName)) {
                    Write-Warn "could not remove $(Join-Path $o $BinName); it is probably still running. It may shadow the new install on PATH."
                } elseif (-not (Test-Path -LiteralPath $o)) {
                    Remove-UserPathSegment $o
                    Write-Step "Removed $o and its user PATH entry."
                } else {
                    Write-Step "Removed codebase-memory-cli.exe from $o (the directory holds other files and stays on PATH)."
                }
            }
        } else {
            Write-Warn 'leaving the other copies in place; a stale copy earlier on PATH can shadow the new install.'
        }
    }

    # 3d. Same-directory install: the binary's own activation replaces it.
    Stop-Daemon $InstallDir

    # ---- 4. Install -------------------------------------------------------
    Write-Step "Installing to $InstallDir..."
    if ((Get-Item -LiteralPath $SourceExe).Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
        throw "refusing to install a reparse-point executable: $SourceExe"
    }
    $StagingDir = New-StagingDirectory
    $StagedExe = Join-Path $StagingDir $BinName
    Copy-Item -LiteralPath $SourceExe -Destination $StagedExe -ErrorAction Stop
    $installArgs = @('install', '-y', '--force', "--dir=$InstallDir")
    if ($SkipConfig) { $installArgs += '--skip-config' }
    $code = Invoke-Native $StagedExe $installArgs
    if ($code -ne 0) {
        Write-Host "error: installation failed (exit code $code)" -ForegroundColor Red
        Write-Host "hint: if the error is a permission refusal, set CBM_CACHE_DIR and CBM_RUNTIME_DIR to owner-private directories and re-run (docs/CONFIGURATION.md, 'Windows: permission refusal'). -CacheDir and -RuntimeDir on this script create and set them." -ForegroundColor Yellow
        Write-Host "hint: if the refused path is the install directory, re-run with -InstallDir <owner-private path>; CBM_CACHE_DIR and CBM_RUNTIME_DIR do not change the install directory." -ForegroundColor Yellow
        exit 1
    }

    # ---- 5. Sidecar files, uninstaller, verification ----------------------
    foreach ($name in @('LICENSE', 'THIRD_PARTY_NOTICES.md')) {
        $from = Join-Path $SourceDir $name
        if (Test-Path -LiteralPath $from -PathType Leaf) {
            Copy-Item -LiteralPath $from -Destination (Join-Path $InstallDir $name) -Force
        } else {
            Write-Warn "$name not found next to this script; not copied"
        }
    }
    $uninstallPath = Join-Path $InstallDir 'uninstall-windows.ps1'
    Set-Content -LiteralPath $uninstallPath -Value $UninstallScript -Encoding ASCII

    $InstalledExe = Join-Path $InstallDir $BinName
    $installedProbe = Get-NativeOutput $InstalledExe @('--version')
    if ($installedProbe.Code -ne 0) {
        throw "installed $BinName failed to run (exit code $($installedProbe.Code)): $($installedProbe.Output)"
    }
    Write-Step "Installed: $($installedProbe.Output)"

    # ---- 6. Register in Settings > Apps -----------------------------------
    $sizeKb = 0
    try {
        $bytes = (Get-ChildItem -LiteralPath $InstallDir -Recurse -File -Force -ErrorAction SilentlyContinue |
            Measure-Object -Property Length -Sum).Sum
        if ($bytes) { $sizeKb = [int][Math]::Max(1, [Math]::Ceiling($bytes / 1024)) }
    } catch { }

    $psExe = "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe"
    $uninstallCmd = "`"$psExe`" -NoProfile -ExecutionPolicy Bypass -File `"$uninstallPath`""

    New-Item -Path $AppKey -Force | Out-Null
    $strings = [ordered]@{
        DisplayName          = 'Codebase Memory CLI'
        DisplayVersion       = $DisplayVersion
        Publisher            = 'ngallodev'
        InstallLocation      = $InstallDir
        DisplayIcon          = $InstalledExe
        URLInfoAbout         = $AppUrl
        HelpLink             = $AppUrl
        InstallDate          = (Get-Date).ToString('yyyyMMdd')
        UninstallString      = $uninstallCmd
        QuietUninstallString = "$uninstallCmd -Quiet"
    }
    foreach ($name in $strings.Keys) {
        New-ItemProperty -LiteralPath $AppKey -Name $name -Value $strings[$name] -PropertyType String -Force | Out-Null
    }
    New-ItemProperty -LiteralPath $AppKey -Name 'EstimatedSize' -Value $sizeKb -PropertyType DWord -Force | Out-Null
    New-ItemProperty -LiteralPath $AppKey -Name 'NoModify' -Value 1 -PropertyType DWord -Force | Out-Null
    New-ItemProperty -LiteralPath $AppKey -Name 'NoRepair' -Value 1 -PropertyType DWord -Force | Out-Null
    if ($ManagedEnv.Count -gt 0) {
        New-ItemProperty -LiteralPath $AppKey -Name 'CbmManagedEnv' -Value ($ManagedEnv -join ';') -PropertyType String -Force | Out-Null
    }

    # ---- 7. Summary --------------------------------------------------------
    Write-Host ''
    Write-Host 'Codebase Memory CLI is installed.' -ForegroundColor Green
    Write-Host "  Version:   $DisplayVersion"
    Write-Host "  Directory: $InstallDir"
    Write-Host '  Open a NEW terminal so the updated PATH takes effect.'
    Write-Host '  Uninstall: Settings > Apps > Codebase Memory CLI, or run'
    Write-Host "             powershell -NoProfile -ExecutionPolicy Bypass -File `"$uninstallPath`""
    exit 0
} catch {
    Write-Host "error: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    if ($StagingDir -and (Test-Path -LiteralPath $StagingDir)) {
        Remove-Item -LiteralPath $StagingDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}
