'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const { spawnSync } = require('node:child_process');
const path = require('node:path');
const test = require('node:test');
const vm = require('node:vm');

class ExitSignal extends Error {
  constructor(code) {
    super(`process.exit(${code})`);
    this.code = code;
  }
}

function runShim(targetPlatform, arguments_, childStatus) {
  const shimPath = path.join(__dirname, '..', 'bin.js');
  const source = fs.readFileSync(shimPath, 'utf8').replace(/^#![^\n]*\n/, '');
  const calls = [];
  let stderr = '';

  const fakeProcess = {
    platform: targetPlatform,
    argv: ['node.exe', shimPath, ...arguments_],
    execPath: 'node.exe',
    stderr: { write: (text) => { stderr += String(text); } },
    exit: (code) => { throw new ExitSignal(code); },
  };
  const fakeFs = {
    existsSync: () => true,
  };
  const fakeChildProcess = {
    spawnSync: (executable, args, options) => {
      calls.push({ executable, args, options });
      return { status: calls.length === 1 ? 0 : childStatus };
    },
  };
  const sandbox = {
    __dirname: path.dirname(shimPath),
    __filename: shimPath,
    Buffer,
    clearTimeout,
    console,
    exports: {},
    module: { exports: {} },
    process: fakeProcess,
    require: (specifier) => {
      if (specifier === 'fs') return fakeFs;
      if (specifier === 'child_process') return fakeChildProcess;
      return require(specifier);
    },
    setTimeout,
  };

  let exitCode = null;
  try {
    vm.runInNewContext(source, sandbox, { filename: shimPath });
  } catch (error) {
    if (!(error instanceof ExitSignal)) throw error;
    exitCode = error.code;
  }
  return { calls, exitCode, stderr };
}

test('Windows npm shim probes and executes the cached launcher', () => {
  const observed = runShim('win32', ['--version'], 0);

  assert.equal(observed.exitCode, 0);
  assert.equal(observed.calls.length, 2);
  for (const call of observed.calls) {
    assert.equal(path.basename(call.executable), 'codebase-memory-cli.exe');
    assert.notEqual(
      path.basename(call.executable),
      'codebase-memory-cli.payload.exe',
    );
  }
  assert.deepEqual(Array.from(observed.calls[1].args), ['--version']);
  assert.equal(observed.calls[1].options.stdio, 'inherit');
});

test('Windows npm shim keeps package-manager guidance after launcher refusal', () => {
  const observed = runShim('win32', ['update'], 1);

  assert.equal(observed.exitCode, 1);
  assert.equal(path.basename(observed.calls[1].executable), 'codebase-memory-cli.exe');
  assert.match(observed.stderr, /npm install codebase-memory-cli@latest/);
  assert.match(observed.stderr, /install --yes/);
});

test('non-Windows npm shim keeps its native payload execution path', () => {
  const observed = runShim('darwin', ['--version'], 0);

  assert.equal(observed.exitCode, 0);
  assert.equal(observed.calls.length, 1);
  assert.equal(path.basename(observed.calls[0].executable), 'codebase-memory-cli');
});

test('PowerShell install mutation runs through the downloaded binary', () => {
  const installer = fs.readFileSync(
    path.join(__dirname, '..', '..', '..', 'install.ps1'),
    'utf8',
  );

  assert.match(
    installer,
    /^\s*\$candidateVersion\s*=\s*&\s*\$DownloadedBinary\s+--version\b/m,
  );
  assert.match(
    installer,
    /^\s*&\s*\$DownloadedBinary\s+@InstallArgs\b/m,
  );
  // One binary ships per platform — no launcher/payload pair to resolve.
  assert.doesNotMatch(installer, /payload/i);
  // A running .exe cannot be overwritten, so an in-place update has to retire
  // the existing binary by renaming it aside first. This script IS the Windows
  // update path, so losing that step would silently break every update.
  assert.match(installer, /Move-Item[\s\S]{0,80}\$Dest[\s\S]{0,40}\$retired/);
});


test('PowerShell installer restores the retired image after failed activation', (t) => {
  const command = process.platform === 'win32' ? 'powershell.exe' : 'pwsh';
  const probe = spawnSync(command, ['-NoProfile', '-Command', '$PSVersionTable.PSVersion.ToString()']);
  if (probe.error && probe.error.code === 'ENOENT') {
    t.skip('PowerShell is not installed');
    return;
  }
  const installerPath = path.join(__dirname, '..', '..', '..', 'install.ps1');
  const script = `
    $ErrorActionPreference = 'Stop'
    $tokens = $null; $errors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile($env:CBM_INSTALLER_TEST_PATH, [ref]$tokens, [ref]$errors)
    if ($errors.Count) { throw 'installer parse failed' }
    $guard = @($ast.FindAll({ param($node)
      $node -is [System.Management.Automation.Language.TryStatementAst] -and
      $node.Finally -and $node.Finally.Extent.Text.Contains('-not $ActivationSucceeded')
    }, $true))
    if ($guard.Count -ne 1) { throw 'activation rollback guard missing' }
    $rollback = [ScriptBlock]::Create(($guard[0].Finally.Statements | ForEach-Object { $_.Extent.Text }) -join [Environment]::NewLine)
    $root = Join-Path ([IO.Path]::GetTempPath()) ('cbm-rollback-' + [Guid]::NewGuid().ToString('N'))
    [IO.Directory]::CreateDirectory($root) | Out-Null
    try {
      $Dest = Join-Path $root 'codebase-memory-cli.exe'
      $retired = $Dest + '.retired-test'
      foreach ($candidatePresent in @($false, $true)) {
        [IO.File]::WriteAllText($retired, 'previous image')
        if ($candidatePresent) { [IO.File]::WriteAllText($Dest, 'failed candidate') }
        $ActivationSucceeded = $false
        . $rollback
        if ([IO.File]::ReadAllText($Dest) -ne 'previous image' -or (Test-Path $retired)) { throw 'previous image not restored' }
        Remove-Item -LiteralPath $Dest
      }
      [IO.File]::WriteAllText($retired, 'previous image')
      [IO.File]::WriteAllText($Dest, 'accepted candidate')
      $ActivationSucceeded = $true
      . $rollback
      if ([IO.File]::ReadAllText($Dest) -ne 'accepted candidate' -or -not (Test-Path $retired)) { throw 'successful activation rolled back' }
    } finally { Remove-Item -LiteralPath $root -Recurse -Force }
  `;
  const result = spawnSync(command, ['-NoProfile', '-Command', script], {
    env: { ...process.env, CBM_INSTALLER_TEST_PATH: installerPath },
    encoding: 'utf8',
  });
  assert.equal(result.status, 0, result.stderr || result.stdout);
});
