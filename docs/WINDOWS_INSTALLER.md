# Windows installer

The `Windows Installer` workflow (`.github/workflows/windows-installer.yml`)
builds the Windows CLI and packages it as a setup program. It is build-only: it
runs no tests, lint, soak, smoke, antivirus scan, attestation or CodeQL, and it
creates no release or tag. Its only output is a workflow artifact.

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
both variables when they are set before you run them.
