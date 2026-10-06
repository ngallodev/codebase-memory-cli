# codebase-memory-cli setup script (Windows)
# Default: install the pre-built native Windows binary through install.ps1
# -FromSource: build a native Windows executable with MSYS2/Clang

param(
    [switch]$FromSource,
    [string]$Binary,
    [string]$InstallDir,
    [switch]$NoPathPrompt,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

$Repo = "ngallodev/codebase-memory-cli"
$BinaryName = "codebase-memory-cli"
$DefaultInstallDir = Join-Path $env:LOCALAPPDATA "codebase-memory-cli"
if (-not $InstallDir) { $InstallDir = $DefaultInstallDir }

# --- Helpers ---

function Write-Ok($msg)   { Write-Host "  $msg" -ForegroundColor Green }
function Write-Fail($msg)  { Write-Host "  $msg" -ForegroundColor Red }
function Write-Warn($msg)  { Write-Host "  $msg" -ForegroundColor Yellow }

function Write-AgentIntegrationGuidance($Command) {
    Write-Host ""
    Write-Host "  Codebase Memory is CLI-first; this setup script does not write MCP client configuration." -ForegroundColor White
    Write-Host "  To install CLI-first skills/instructions for detected agents, run:" -ForegroundColor White
    Write-Host ""
    Write-Host "    $Command install --skip-binary" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Hooks are never installed by setup or install. To opt in separately, run:" -ForegroundColor White
    Write-Host ""
    Write-Host "    $Command install-hooks" -ForegroundColor Yellow
}


# --- Main ---

if ($Help) {
    Write-Host ""
    Write-Host "Usage: .\setup-windows.ps1 [-FromSource] [-Binary PATH] [-InstallDir PATH] [-NoPathPrompt] [-Help]"
    Write-Host ""
    Write-Host "  Default:      Install the pre-built Windows binary through install.ps1"
    Write-Host "  -FromSource:  Build a native Windows .exe with MSYS2/Clang (no WSL)"
    Write-Host "  -Binary PATH: Install exact existing candidate bytes (qualification/offline mode)"
    Write-Host "  -InstallDir:  Override the installation directory"
    Write-Host "  -NoPathPrompt: Never prompt to change the user PATH"
    Write-Host ""
    exit 0
}

Write-Host ""
Write-Host "codebase-memory-cli installer (Windows)" -ForegroundColor White
Write-Host ""

if ($FromSource -and $Binary) { throw "-FromSource and -Binary are mutually exclusive" }

if ($Binary) {
    $sourceBinary = (Resolve-Path -LiteralPath $Binary).Path
    if (-not (Test-Path -LiteralPath $InstallDir)) { New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null }
    $binaryPath = Join-Path $InstallDir "$BinaryName.exe"
    Copy-Item -LiteralPath $sourceBinary -Destination $binaryPath -Force
    $sourceHash = (Get-FileHash -LiteralPath $sourceBinary -Algorithm SHA256).Hash
    $installedHash = (Get-FileHash -LiteralPath $binaryPath -Algorithm SHA256).Hash
    if ($sourceHash -ne $installedHash) { throw "installed candidate hash mismatch" }
    $verOut = & $binaryPath --version 2>&1
    if ($LASTEXITCODE -ne 0) { throw "installed candidate failed --version" }
    Write-Ok "Installed exact candidate bytes to $binaryPath"
    Write-Ok "SHA-256: $($installedHash.ToLowerInvariant())"
    Write-Ok "Version: $verOut"
    Write-AgentIntegrationGuidance ('"' + $binaryPath + '"')
    Write-Host ""
    Write-Ok "Done! Try: $binaryPath --help"
    return
}

if ($FromSource) {
    # --- Native Windows source build via MSYS2/Clang ---
    $git = Get-Command git.exe -ErrorAction SilentlyContinue
    if (-not $git) {
        Write-Fail "git.exe is required for -FromSource."
        exit 1
    }

    $msysRoot = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { "C:\msys64" }
    $buildHelper = Join-Path $msysRoot "usr\bin\make.exe"
    if (-not (Test-Path -LiteralPath $buildHelper)) {
        Write-Fail "MSYS2 was not found at $msysRoot."
        Write-Host "  Install MSYS2 and its CLANG64 Clang/zlib + make packages, then retry." -ForegroundColor Yellow
        Write-Host "  This source-build path intentionally does not use WSL." -ForegroundColor Yellow
        exit 1
    }

    $sourceDir = Join-Path $env:LOCALAPPDATA "codebase-memory-cli-src"
    if (Test-Path -LiteralPath (Join-Path $sourceDir ".git")) {
        Write-Host "Updating source..." -ForegroundColor White
        & $git.Source -C $sourceDir pull --ff-only
        if ($LASTEXITCODE -ne 0) { throw "git pull failed" }
    } else {
        Write-Host "Cloning repository..." -ForegroundColor White
        & $git.Source clone "https://github.com/$Repo.git" $sourceDir
        if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
    }
    Write-Ok "Source at $sourceDir"

    Write-Host "Building native Windows executable..." -ForegroundColor White
    & (Join-Path $sourceDir "scripts\build-windows.ps1") -Msys2Root $msysRoot
    if ($LASTEXITCODE -ne 0) { throw "native Windows source build failed" }

    if (-not (Test-Path $InstallDir)) {
        New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    }
    $builtBinary = Join-Path $sourceDir "build\c\codebase-memory-cli.exe"
    $binaryPath = Join-Path $InstallDir "$BinaryName.exe"
    Copy-Item -LiteralPath $builtBinary -Destination $binaryPath -Force
    Write-Ok "Installed native executable to $binaryPath"

    $verOut = & $binaryPath --version 2>&1
    if ($LASTEXITCODE -ne 0) { throw "installed executable failed --version" }
    Write-Ok "Version: $verOut"
    Write-AgentIntegrationGuidance ('"' + $binaryPath + '"')

    Write-Host ""
    Write-Ok "Done! Try: $binaryPath --help"
    Write-Host ""
    Write-Host "  To uninstall:" -ForegroundColor White
    Write-Host "    Remove-Item -Recurse -Force '$InstallDir'"
    Write-Host "    Remove-Item -Recurse -Force '$sourceDir'"

} else {
    # --- Download + install through install.ps1 ---
    #
    # install.ps1 is the one implementation of "fetch a release and install
    # it": it downloads checksums.txt next to the archive, verifies the
    # archive's SHA-256 against it, validates the zip layout and only then
    # runs the binary's own `install`. This script used to carry a second
    # copy of that download, which did not keep up with the installer. It
    # now fetches install.ps1 from the same origin and branch it is itself
    # served from and hands over to it, so there is exactly one install path.
    #
    # CBM_DOWNLOAD_URL (the installers' download-base override, for local
    # testing) also moves the installer fetch: install.ps1 is then taken from
    # "$env:CBM_DOWNLOAD_URL/install.ps1".
    $installerUrl = "https://raw.githubusercontent.com/$Repo/main/install.ps1"
    if ($env:CBM_DOWNLOAD_URL) {
        $installerUrl = $env:CBM_DOWNLOAD_URL.TrimEnd('/') + "/install.ps1"
    }

    # Same transport rule as install.ps1: HTTPS, or plain HTTP for a loopback
    # authority only (the local test fixture). No redirects are followed: both
    # origins serve the installer directly.
    try { $installerUri = [Uri]$installerUrl } catch { $installerUri = $null }
    $loopbackHttp = (
        $installerUri -and $installerUri.IsAbsoluteUri -and
        $installerUri.Scheme -eq "http" -and $installerUri.IsLoopback -and
        [string]::IsNullOrEmpty($installerUri.UserInfo)
    )
    if (-not $installerUri -or -not $installerUri.IsAbsoluteUri -or
        ($installerUri.Scheme -ne "https" -and -not $loopbackHttp) -or
        -not [string]::IsNullOrEmpty($installerUri.UserInfo)) {
        Write-Fail "Refusing non-HTTPS installer URL: $installerUrl"
        exit 1
    }

    # TLS 1.2+ for the fetch (older Windows PowerShell defaults to TLS 1.0,
    # which GitHub rejects). TLS 1.3 only where schannel can negotiate it; see
    # the note in install.ps1.
    $protocols = [Net.SecurityProtocolType]::Tls12
    if ([Environment]::OSVersion.Version.Build -ge 20348 -and
        ([enum]::GetNames([Net.SecurityProtocolType]) -contains 'Tls13')) {
        $protocols = $protocols -bor [Net.SecurityProtocolType]::Tls13
    }
    [Net.ServicePointManager]::SecurityProtocol = $protocols

    # A fresh staging directory of its own with an owner-only DACL, the
    # way install.ps1 stages its own download. The ACL is best effort: a
    # filesystem that cannot carry one must not fail the install.
    $stageDir = Join-Path ([System.IO.Path]::GetTempPath()) "cbm-setup-$(Get-Random)"
    New-Item -ItemType Directory -Path $stageDir -Force | Out-Null
    try {
        $stageAcl = New-Object System.Security.AccessControl.DirectorySecurity
        $stageAcl.SetAccessRuleProtection($true, $false)
        $stageOwner = ([System.Security.Principal.WindowsIdentity]::GetCurrent()).User
        $stageAcl.SetOwner($stageOwner)
        $stageAcl.AddAccessRule((New-Object System.Security.AccessControl.FileSystemAccessRule(
            $stageOwner, 'FullControl', 'ContainerInherit,ObjectInherit', 'None', 'Allow')))
        Set-Acl -Path $stageDir -AclObject $stageAcl -ErrorAction Stop
    } catch {
        Write-Host "  note: could not harden the staging directory ACL: $($_.Exception.Message)"
    }

    $installer = Join-Path $stageDir "install.ps1"
    $failure = $null
    try {
        Write-Host "Fetching install.ps1..." -ForegroundColor White
        Invoke-WebRequest -Uri $installerUrl -OutFile $installer -UseBasicParsing -MaximumRedirection 0
        if (-not (Test-Path -LiteralPath $installer -PathType Leaf) -or
            (Get-Item -LiteralPath $installer).Length -eq 0) {
            throw "fetched an empty install.ps1 from $installerUrl"
        }
        Write-Ok "install.ps1 fetched"

        Write-Host ""
        Write-Host "Installing through install.ps1..." -ForegroundColor White
        # The installer configures nothing (--skip-config): agent configuration
        # stays this script's interactive step below. It runs in a child host
        # of the same PowerShell so its exit code is unambiguous and its global
        # settings do not leak into this session. The pre-seeded exit code
        # keeps a child that fails to start from reading as success.
        $hostExe = (Get-Process -Id $PID).Path
        $global:LASTEXITCODE = 1
        & $hostExe -NoProfile -ExecutionPolicy Bypass -File $installer "--dir=$InstallDir" --skip-config
        if ($LASTEXITCODE -ne 0) {
            throw "install.ps1 exited with $LASTEXITCODE"
        }
    } catch {
        $failure = "$_"
    } finally {
        Remove-Item -Recurse -Force $stageDir -ErrorAction SilentlyContinue
    }
    if ($failure) {
        Write-Fail "Installation through install.ps1 failed: $failure"
        exit 1
    }

    $binaryPath = Join-Path $InstallDir "$BinaryName.exe"

    if (-not (Test-Path $binaryPath)) {
        Write-Fail "Binary not found at $binaryPath after installation"
        exit 1
    }
    Write-Ok "Installed to $binaryPath"

    # Verify binary runs
    try {
        $verOut = & $binaryPath --version 2>&1
        Write-Ok "Version: $verOut"
    } catch {
        Write-Warn "Could not verify binary version (may still work)"
    }

    # SmartScreen note
    Write-Host ""
    Write-Warn "Windows SmartScreen may show a warning when the binary runs for the first time."
    Write-Host "    This is normal for unsigned open-source binaries." -ForegroundColor Yellow
    Write-Host "    Click 'More info' then 'Run anyway' to proceed." -ForegroundColor Yellow
    Write-Host "    Verify checksums at: https://github.com/$Repo/releases" -ForegroundColor Yellow

    # Check if install dir is on PATH
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if (-not $NoPathPrompt -and $userPath -notlike "*$InstallDir*") {
        Write-Host ""
        Write-Warn "$InstallDir is not on your PATH."
        $addPath = Read-Host "  Add it to your user PATH? [y/N]"
        if ($addPath -match '^[Yy]$') {
            [Environment]::SetEnvironmentVariable("Path", "$userPath;$InstallDir", "User")
            Write-Ok "Added to user PATH (restart your terminal to take effect)"
        }
    }

    Write-AgentIntegrationGuidance ('"' + $binaryPath + '"')

    Write-Host ""
    Write-Ok "Done! Try: $binaryPath --help"
    Write-Host ""
    Write-Host "  To uninstall:" -ForegroundColor White
    Write-Host "    Remove-Item -Recurse -Force '$InstallDir'"
    Write-Host "    Remove-Item -Recurse -Force `"$env:LOCALAPPDATA\codebase-memory-mcp`"  # legacy-compatible graph database location"
}
