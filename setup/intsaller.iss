#define MyAppName "Tekst"
#define MyAppVersion "2.1.2"
#define MyAppPublisher "Tekst"
#define MyAppExeName "tekst.exe"

[Setup]
AppId={{8D0E6A8B-6E5D-4C8A-B9D5-TEKST0000001}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\Tekst
DefaultGroupName=Tekst
OutputDir=Output
OutputBaseFilename=Tekst-{#MyAppVersion}-Setup
SetupIconFile=icon.ico
UninstallDisplayIcon={app}\bin\tekst.exe
ArchitecturesInstallIn64BitMode=x64
ChangesEnvironment=yes
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Dirs]
Name: "{app}\bin"
Name: "{app}\lib"
Name: "{app}\packages"
Name: "{localappdata}\Tekst"
Name: "{localappdata}\Tekst\packages"

[Files]
Source: "..\build\release\tekst.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\build\release\tk.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\build\release\runtime.cpp"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\build\release\runtime.h"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "icon.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "file.ico"; DestDir: "{app}"; Flags: ignoreversion

[Registry]
Root: HKCU; Subkey: "Software\Classes\.tk"; ValueType: string; ValueName: ""; ValueData: "TekstFile"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\TekstFile"; ValueType: string; ValueName: ""; ValueData: "Tekst Source File"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\TekstFile"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "Tekst Source File"
Root: HKCU; Subkey: "Software\Classes\TekstFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\file.ico"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\TekstFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\tekst.exe"" ""%1"""; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\.tekst"; ValueType: none; ValueName: ""; ValueData: ""; Flags: deletekey
Root: HKCU; Subkey: "Software\Classes\tekstfile"; ValueType: none; ValueName: ""; ValueData: ""; Flags: deletekey

[Tasks]
Name: "addtopath"; Description: "Add Tekst and tk to PATH"; Flags: unchecked
Name: "startmenu"; Description: "Create Start Menu shortcuts"
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked

[Code]
procedure AddToPath();
var
  Path: string;
  BinPath: string;
begin
  BinPath := ExpandConstant('{app}\bin');

  if RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', Path) then
  begin
    if Pos(BinPath, Path) = 0 then
      Path := Path + ';' + BinPath;
  end
  else
    Path := BinPath;

  RegWriteStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', Path);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    if WizardIsTaskSelected('addtopath') then
      AddToPath();
  end;
end;

[Icons]
Name: "{group}\Tekst"; Filename: "{app}\bin\tekst.exe"; IconFilename: "{app}\icon.ico"; Tasks: startmenu
Name: "{group}\Tekst tk"; Filename: "{app}\bin\tk.exe"; IconFilename: "{app}\icon.ico"; Tasks: startmenu
Name: "{group}\Uninstall Tekst"; Filename: "{uninstallexe}"; Tasks: startmenu
Name: "{autodesktop}\Tekst"; Filename: "{app}\bin\tekst.exe"; IconFilename: "{app}\icon.ico"; Tasks: desktopicon