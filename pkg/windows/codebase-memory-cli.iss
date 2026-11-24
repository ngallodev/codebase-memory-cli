; Inno Setup 6 script for the codebase-memory-cli Windows installer.
;
; The installer is a thin wrapper: it carries the release zip archives plus
; checksums.txt and install.ps1, then runs install.ps1 in local-archive mode
; (CBM_ARCHIVE_DIR). install.ps1 verifies the checksum, validates the archive
; layout, and installs the binary, user PATH and agent configuration exactly as
; the web installer does. Inno only supplies the wizard, the Add/Remove
; Programs entry and the uninstaller.
;
; Compile (the workflow does this):
;   ISCC.exe /DAppVersion=v0.9.0-dev.1 /DStageDir=<dir with zips, checksums.txt,
;            install.ps1> /DRepoRoot=<repo root> /O<output dir> codebase-memory-cli.iss
; StageDir may hold one or both of codebase-memory-cli-windows-{amd64,arm64}.zip.

#ifndef AppVersion
  #error AppVersion is required: pass /DAppVersion=<version>
#endif
#ifndef StageDir
  #error StageDir is required: pass /DStageDir=<directory>
#endif
#ifndef RepoRoot
  #error RepoRoot is required: pass /DRepoRoot=<directory>
#endif

#if FileExists(StageDir + "\codebase-memory-cli-windows-amd64.zip")
  #define HaveAmd64
#endif
#if FileExists(StageDir + "\codebase-memory-cli-windows-arm64.zip")
  #define HaveArm64
#endif
#if !Defined(HaveAmd64) && !Defined(HaveArm64)
  #error StageDir contains no codebase-memory-cli-windows-*.zip archive
#endif

[Setup]
; Fixed identity: changing it makes upgrades install side by side.
AppId={{FEB1E0BB-E136-4164-AD3D-0E5CCBB15673}
AppName=Codebase Memory CLI
AppVersion={#AppVersion}
AppPublisher=ngallodev
AppPublisherURL=https://github.com/ngallodev/codebase-memory-cli
AppSupportURL=https://github.com/ngallodev/codebase-memory-cli/issues
; Same location install.ps1 uses; the page is hidden so the two cannot differ.
DefaultDirName={localappdata}\Programs\codebase-memory-cli
DisableDirPage=yes
; Keep unins000.* out of the CLI's install directory: `codebase-memory-cli
; uninstall` owns that directory and must not delete the running uninstaller.
UninstallFilesDir={localappdata}\Programs\codebase-memory-cli-setup
DisableProgramGroupPage=yes
DisableReadyPage=no
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible arm64
ArchitecturesInstallIn64BitMode=x64compatible arm64
LicenseFile={#RepoRoot}\LICENSE
OutputBaseFilename=codebase-memory-cli-{#AppVersion}-windows-setup
UninstallDisplayName=Codebase Memory CLI
UninstallDisplayIcon={app}\codebase-memory-cli.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Everything goes to {tmp}; install.ps1 does the real installation.
#ifdef HaveAmd64
Source: "{#StageDir}\codebase-memory-cli-windows-amd64.zip"; DestDir: "{tmp}"; Flags: deleteafterinstall
#endif
#ifdef HaveArm64
Source: "{#StageDir}\codebase-memory-cli-windows-arm64.zip"; DestDir: "{tmp}"; Flags: deleteafterinstall
#endif
Source: "{#StageDir}\checksums.txt"; DestDir: "{tmp}"; Flags: deleteafterinstall
Source: "{#StageDir}\install.ps1"; DestDir: "{tmp}"; Flags: deleteafterinstall

[UninstallRun]
; The CLI removes agent configuration, indexes' registrations and its own entry
; on the current-user PATH. -y answers prompts without deleting project indexes.
Filename: "{app}\codebase-memory-cli.exe"; Parameters: "uninstall -y ""--dir={app}"""; Flags: runhidden skipifdoesntexist; RunOnceId: "CbmUninstall"

[UninstallDelete]
; Files install.ps1 leaves in the install directory, in case the CLI could not
; remove them (a running image cannot be deleted).
Type: files; Name: "{app}\codebase-memory-cli.exe"
Type: files; Name: "{app}\codebase-memory-cli.exe.retired-*"
Type: files; Name: "{app}\install.ps1"
Type: dirifempty; Name: "{app}"

[Code]
function MachineArch: String;
begin
  if IsArm64 then
    Result := 'arm64'
  else
    Result := 'amd64';
end;

function BundleHasMachineArch: Boolean;
begin
#ifdef HaveAmd64
  if not IsArm64 then begin Result := True; Exit; end;
#endif
#ifdef HaveArm64
  if IsArm64 then begin Result := True; Exit; end;
#endif
  Result := False;
end;

function InitializeSetup: Boolean;
begin
  Result := BundleHasMachineArch;
  if not Result then
    SuppressibleMsgBox(
      'This installer does not contain a build for this machine (' +
      MachineArch + '). Download the installer built for both architectures, ' +
      'or one that includes ' + MachineArch + '.',
      mbCriticalError, MB_OK, IDOK);
end;

procedure RunInstallScript;
var
  Tmp, Cmd, LogFile, Output: String;
  Rc: Integer;
  Ansi: AnsiString;
begin
  Tmp := RemoveBackslash(ExpandConstant('{tmp}'));
  LogFile := Tmp + '\install.log';
  Cmd := Tmp + '\run-install.cmd';
  { A batch file keeps the environment and quoting simple. CBM_ARCH keeps
    install.ps1 on the same archive this installer selected. }
  if not SaveStringToFile(Cmd,
      '@echo off' + #13#10 +
      'set "CBM_ARCHIVE_DIR=' + Tmp + '"' + #13#10 +
      'set "CBM_ARCH=' + MachineArch + '"' + #13#10 +
      'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "' + Tmp +
        '\install.ps1" > "' + LogFile + '" 2>&1' + #13#10 +
      'exit /b %ERRORLEVEL%' + #13#10, False) then
    RaiseException('Could not write the installer helper script.');

  WizardForm.StatusLabel.Caption := 'Installing Codebase Memory CLI...';
  if not Exec(ExpandConstant('{cmd}'), '/C ""' + Cmd + '""', Tmp, SW_HIDE,
      ewWaitUntilTerminated, Rc) then
    RaiseException('Could not start powershell.exe: ' + SysErrorMessage(Rc));

  Output := '';
  if LoadStringFromFile(LogFile, Ansi) then
    Output := String(Ansi);
  Log('install.ps1 exit code ' + IntToStr(Rc));
  Log('install.ps1 output:' + #13#10 + Output);

  if Rc <> 0 then
  begin
    { Keep the tail so the message box stays readable; the full text is in
      the setup log (run with /LOG). }
    if Length(Output) > 1500 then
      Output := '...' + Copy(Output, Length(Output) - 1496, 1497);
    RaiseException('install.ps1 failed with exit code ' + IntToStr(Rc) +
      '.' + #13#10#13#10 + Output);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    RunInstallScript;
end;
