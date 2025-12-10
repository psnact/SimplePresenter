[Setup]
AppId={{8C4B7F6D-2D74-4B1E-9B86-1234567890AB}  ; KEEP THIS GUID THE SAME FOR ALL FUTURE VERSIONS
AppName=SimplePresenter
AppVersion=1.0.0
DefaultDirName={pf}\SimplePresenter
DefaultGroupName=SimplePresenter
OutputDir=.
OutputBaseFilename=SimplePresenter-Setup
Compression=lzma
SolidCompression=yes
DisableDirPage=no
SetupIconFile=C:\SimplePresenter\resources\Logo.ico

[Dirs]
; No explicit Bible dir here; the app and installer will create the
; cross-platform AppData Bible directory as needed.

[Files]
; Main application (everything from deploy folder)
Source: "C:\SimplePresenter\deploy\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

; Bible XMLs – copy to the cross-platform AppData Bible directory
; Windows AppDataLocation: %LOCALAPPDATA%\SimplePresenter\SimplePresenter\Bible
; Copy ONLY if they don't already exist, and never remove on uninstall
Source: "C:\SimplePresenter\Bible\*.xml"; DestDir: "{localappdata}\SimplePresenter\SimplePresenter\Bible"; Flags: recursesubdirs createallsubdirs ignoreversion onlyifdoesntexist uninsneveruninstall

[Icons]
Name: "{group}\SimplePresenter"; Filename: "{app}\SimplePresenter.exe"
Name: "{commondesktop}\SimplePresenter"; Filename: "{app}\SimplePresenter.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"