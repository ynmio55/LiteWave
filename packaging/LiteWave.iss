#define AppName "LiteWave"
#define AppVersion "0.9.0"
#define AppPublisher "ynmio55"
#define AppExeName "LiteWave.exe"

[Setup]
OutputDir=..\dist
AppId={{A9B8C7D6-E5F4-4321-9876-LITEWAVE0001}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\LiteWave
DefaultGroupName=LiteWave
OutputBaseFilename=LiteWave-Setup-Windows-x64
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
UninstallDisplayIcon={app}\{#AppExeName}
WizardStyle=modern

[Files]
Source: "..\package\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\LiteWave"; Filename: "{app}\{#AppExeName}"
Name: "{commondesktop}\LiteWave"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "สร้างไอคอน LiteWave บน Desktop"; GroupDescription: "ไอคอนเพิ่มเติม:"

[Run]
Filename: "{app}\{#AppExeName}"; Description: "เปิด LiteWave"; Flags: nowait postinstall skipifsilent
