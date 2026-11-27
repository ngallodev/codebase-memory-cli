# Windows installer

The `Windows Installer` workflow (`.github/workflows/windows-installer.yml`)
builds the Windows CLI and packages it as a setup program. It is build-only: it
runs no tests, lint, soak, smoke, antivirus scan, attestation or CodeQL, and it
creates no release or tag. Its only output is a workflow artifact. The optional
`smoke_install` input (default false) adds a final job that runs
`install-windows.ps1` from the amd64 zip in a temporary directory, checks the
Settings entry and PATH, runs the registered quiet uninstall command and checks
that everything was removed.

## Building

The workflow runs only on manual dispatch:

    gh workflow run windows-installer.yml -R ngallodev/codebase-memory-cli \
      -f version=v0.9.0-dev.1 -f arches=both

`version` must be v-prefixed semver (`vX.Y.Z` or `vX.Y.Z-suffix`) and is stamped
into the binary. `arches` is `both` (default), `amd64` or `arm64`. amd64 builds
on `windows-latest` (MSYS2 CLANG64) and arm64 on `windows-11-arm` (MSYS2
CLANGARM64).

Each build job runs `scripts/build.sh --with-ui --version <version>`, which is a
clean build only, then `scripts/ci/prepare-release-candidates.sh` (strip) and
`scripts/package-release.sh`, the same packaging the release uses. A final job
compiles `pkg/windows/codebase-memory-cli.iss` with Inno Setup.

## Output

Download the artifact `codebase-memory-cli-windows-<version>` from the run page
or with `gh run download <run-id> -R ngallodev/codebase-memory-cli`. It holds:

- `codebase-memory-cli-<version>-windows-setup.exe`
- `SHA256SUMS` (checksum of the setup program)
- `codebase-memory-cli-windows-amd64.zip` and `codebase-memory-cli-windows-arm64.zip`
  (only the selected architectures)
- `checksums.txt` (checksums of the zips, in the format `install.ps1` verifies)
- `install.ps1`

The artifact is kept for 30 days.

## What setup.exe does

Setup installs for the current user and does not ask for administrator rights.
It bundles the zips, `checksums.txt` and `install.ps1`, picks the zip for the
machine architecture (ARM64 or amd64), and runs `install.ps1` with
`CBM_ARCHIVE_DIR` pointing at its temporary directory. `install.ps1` then does
what it does for a web install: verifies the checksum, checks the archive
contents, installs to `%LOCALAPPDATA%\Programs\codebase-memory-cli`, adds that
directory to the user PATH and configures agents. Setup fails if `install.ps1`
exits non-zero, and shows the end of its output. If the installer was built for
one architecture only and the machine is the other, setup stops at the start.

Setup registers an entry in Add/Remove Programs. Uninstalling runs
`codebase-memory-cli.exe uninstall -y`, which removes the agent configuration
and the PATH entry; project indexes are kept.

## Offline install

`install.ps1` reads the archive and `checksums.txt` from a local directory when
`CBM_ARCHIVE_DIR` is set to an absolute path of an existing directory (not a
reparse point):

    $env:CBM_ARCHIVE_DIR = 'C:\path\to\artifact'
    powershell -NoProfile -ExecutionPolicy Bypass -File C:\path\to\artifact\install.ps1

The directory must contain `codebase-memory-cli-windows-<arch>.zip` and
`checksums.txt`. All other checks are the same as for a download.

## Offline install from the extracted zip

Each Windows release zip (`codebase-memory-cli-windows-<arch>.zip`) also holds
`install-windows.ps1`. It installs the `codebase-memory-cli.exe` next to it,
downloads nothing, and registers the app in Windows Settings > Apps. Extract the
zip to a folder outside the install directory, open a terminal in that folder
and run:

    powershell -NoProfile -ExecutionPolicy Bypass -File .\install-windows.ps1 [options]

It works on Windows PowerShell 5.1 and PowerShell 7, for the current user only:
no administrator rights, nothing under HKLM. If you run it elevated it warns
that it installs for the elevated user's profile.

Options:

- `-InstallDir <path>`: install directory. Default
  `%LOCALAPPDATA%\Programs\codebase-memory-cli`.
- `-SkipConfig`: do not install agent skills or instructions.
- `-CacheDir <path>`, `-RuntimeDir <path>`: create the directory with access for
  the current user only and set `CBM_CACHE_DIR` / `CBM_RUNTIME_DIR` as
  persistent user environment variables (and for the install itself). Use these
  for the permission refusal described below.
- `-Yes`: do not prompt. Also uninstalls other copies found on PATH (see below).
- `-Help`: print the options.

The script refuses to run if the executable is missing, if it is the arm64 build
on an x64 machine, or if the script itself is inside the install directory. An
amd64 build on an ARM64 machine installs with a warning (it runs under
emulation). It clears the downloaded-file mark on the extracted files and runs
`codebase-memory-cli.exe --version` before changing anything.

What it removes before installing:

- An install made by the Setup program (`setup.exe`): its own uninstaller is run
  silently and the script waits up to 120 seconds for it to finish.
- An earlier install registered by this script at a different directory: that
  copy's `codebase-memory-cli.exe uninstall -y` runs and leftover files are
  deleted.
- Other `codebase-memory-cli.exe` copies in directories on your user PATH: they
  are listed and, after confirmation or with `-Yes`, uninstalled. Without
  confirmation they are left in place and the script warns, because an older
  copy earlier on PATH can shadow the new one.
- An install in the same directory (from `install.ps1` or an earlier run): the
  daemon is stopped and `codebase-memory-cli.exe install --force` replaces it.

The install itself is `codebase-memory-cli.exe install -y --force --dir=<dir>`,
so the PATH entry and agent assets are handled by the binary. The script then
copies `LICENSE` and `THIRD_PARTY_NOTICES.md` and writes `uninstall-windows.ps1`
into the install directory. If the install fails, no Settings entry is written.
Open a new terminal afterwards so the updated PATH applies.

Uninstall from Settings > Apps > Codebase Memory CLI, or run
`powershell -NoProfile -ExecutionPolicy Bypass -File "<install dir>\uninstall-windows.ps1"`
(add `-DeleteIndexes` to also delete project indexes; add `-Quiet` for no
prompts). It runs `codebase-memory-cli.exe uninstall -y`, deletes the files the
installer placed, removes only the user environment variables that
`install-windows.ps1` set (`-CacheDir` / `-RuntimeDir`), and then deletes the
Settings entry. Project indexes are kept by default. If the executable cannot be
removed (for example a locked file), the Settings entry is kept so you can retry.
A directory given with `-CacheDir` or `-RuntimeDir` is not deleted.

## Limits

- The setup program and the binaries are unsigned; Windows SmartScreen and
  antivirus may warn.
- The pipeline has not been run on a Windows machine by its author. The build
  and setup steps are unverified until a first dispatch completes.
- If the runner does not have Inno Setup 6, the workflow installs it with
  Chocolatey. The script needs Inno Setup 6.3 or later (`x64compatible`).

## Troubleshooting: permission refusal

If setup fails with `acl-grants-cross-account-mutation` or `secure CLI coordination
could not be created`, another account has write access to a profile folder
(often an agent sandbox). The check is intentional. Set `CBM_CACHE_DIR` and
`CBM_RUNTIME_DIR` to owner-private directories; see "Windows: permission refusal"
in [CONFIGURATION.md](CONFIGURATION.md). `setup.exe` and `install.ps1` honor
both variables when they are set before you run them; `install-windows.ps1`
can create and set them with `-CacheDir` and `-RuntimeDir`.
