<#
.SYNOPSIS
  Installs Avestan Editor on Windows and registers the .ave file type with an icon.
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File install-windows.ps1 -AppDir .\deploy -Exe AvestanEditor.exe -Icon .\avestan.ico
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File install-windows.ps1 -Uninstall
.NOTES
  -AppDir  folder with your built app (run windeployqt on it first)
  -Icon    .ico file (include 16, 32, 48 and 256 px sizes)
  -AllUsers installs into Program Files for everyone (needs an administrator shell)
#>
param([string]$AppDir, [string]$Exe, [string]$Icon, [switch]$AllUsers, [switch]$Uninstall)
$ErrorActionPreference = 'Stop'

$Id = 'AvestanEditor'; $ProgId = 'AvestanEditor.Document'; $Ext = '.ave'; $Title = 'Avestan Editor'
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
           ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($AllUsers -and -not $isAdmin) { throw '-AllUsers needs an elevated PowerShell (Run as administrator).' }
if ($AllUsers) {
  $Root = 'HKLM:'; $InstallDir = Join-Path $env:ProgramFiles $Id
  $Menu = Join-Path $env:ProgramData 'Microsoft\Windows\Start Menu\Programs'
} else {
  $Root = 'HKCU:'; $InstallDir = Join-Path $env:LOCALAPPDATA "Programs\$Id"
  $Menu = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs'
}
$Classes = "$Root\Software\Classes"
$UninstKey = "$Root\Software\Microsoft\Windows\CurrentVersion\Uninstall\$Id"

function Set-Default($path, $value) {
  New-Item -Path $path -Force | Out-Null
  Set-ItemProperty -Path $path -Name '(default)' -Value $value
}
function Update-Shell {   # makes Explorer refresh icons and associations immediately
  if (-not ('Win32.ShellNotify' -as [type])) {
    Add-Type -Namespace Win32 -Name ShellNotify -MemberDefinition '[DllImport("shell32.dll")] public static extern void SHChangeNotify(int e, uint f, IntPtr a, IntPtr b);'
  }
  [Win32.ShellNotify]::SHChangeNotify(0x08000000, 0, [IntPtr]::Zero, [IntPtr]::Zero)
}

if ($Uninstall) {
  $exeName = (Get-ItemProperty $UninstKey -ErrorAction SilentlyContinue).ExeName
  $k = "$Classes\$Ext"
  if (Test-Path $k) {
    if ((Get-ItemProperty $k).'(default)' -eq $ProgId) { Remove-Item $k -Recurse -Force }
    else { Remove-ItemProperty "$k\OpenWithProgids" -Name $ProgId -ErrorAction SilentlyContinue }
  }
  Remove-Item "$Classes\$ProgId" -Recurse -Force -ErrorAction SilentlyContinue
  if ($exeName) { Remove-Item "$Classes\Applications\$exeName" -Recurse -Force -ErrorAction SilentlyContinue }
  Remove-Item $UninstKey -Recurse -Force -ErrorAction SilentlyContinue
  Remove-Item "$Menu\$Title.lnk" -Force -ErrorAction SilentlyContinue
  Update-Shell
  # this script may be running from inside the install folder, so delete it a moment later
  Start-Process cmd.exe -WindowStyle Hidden -ArgumentList "/c ping -n 3 127.0.0.1 >nul & rmdir /s /q `"$InstallDir`""
  Write-Host "$Title uninstalled."; return
}

if (-not (Test-Path $AppDir -PathType Container)) { throw '-AppDir must be the folder that contains your built app.' }
if (-not $Exe -or -not (Test-Path (Join-Path $AppDir $Exe))) { throw "-Exe not found inside $AppDir" }
if (-not $Icon -or -not (Test-Path $Icon)) { throw '-Icon must point to a .ico file.' }

# 1. copy program files
if (Test-Path $InstallDir) { Remove-Item $InstallDir -Recurse -Force }
New-Item $InstallDir -ItemType Directory -Force | Out-Null
Copy-Item (Join-Path $AppDir '*') $InstallDir -Recurse -Force
Copy-Item $Icon (Join-Path $InstallDir 'ave-file.ico') -Force
Copy-Item $PSCommandPath (Join-Path $InstallDir 'setup.ps1') -Force   # used by the uninstaller
$ExePath = Join-Path $InstallDir $Exe
$IcoPath = Join-Path $InstallDir 'ave-file.ico'
$Open = "`"$ExePath`" `"%1`""

# 2. file type: .ave -> AvestanEditor.Document (icon + open command)
Set-Default "$Classes\$Ext" $ProgId
Set-ItemProperty "$Classes\$Ext" -Name 'Content Type' -Value 'application/x-ave'
New-Item "$Classes\$Ext\OpenWithProgids" -Force | Out-Null
New-ItemProperty "$Classes\$Ext\OpenWithProgids" -Name $ProgId -Value '' -PropertyType String -Force | Out-Null
Set-Default "$Classes\$ProgId" "$Title Document"
Set-Default "$Classes\$ProgId\DefaultIcon" "`"$IcoPath`",0"
Set-Default "$Classes\$ProgId\shell\open\command" $Open

# 3. make the app show up in "Open with"
Set-Default "$Classes\Applications\$Exe\shell\open\command" $Open
Set-ItemProperty "$Classes\Applications\$Exe" -Name FriendlyAppName -Value $Title
New-Item "$Classes\Applications\$Exe\SupportedTypes" -Force | Out-Null
New-ItemProperty "$Classes\Applications\$Exe\SupportedTypes" -Name $Ext -Value '' -PropertyType String -Force | Out-Null

# 4. Start-menu shortcut and an entry in Settings > Apps
$lnk = (New-Object -ComObject WScript.Shell).CreateShortcut("$Menu\$Title.lnk")
$lnk.TargetPath = $ExePath; $lnk.WorkingDirectory = $InstallDir; $lnk.Save()
New-Item $UninstKey -Force | Out-Null
$scope = if ($AllUsers) { ' -AllUsers' } else { '' }
$props = @{
  DisplayName = $Title; DisplayIcon = $IcoPath; InstallLocation = $InstallDir; ExeName = $Exe
  Publisher = 'ArdeshirV'; NoModify = 1; NoRepair = 1
  UninstallString = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$InstallDir\setup.ps1`" -Uninstall$scope"
}
foreach ($p in $props.GetEnumerator()) { Set-ItemProperty $UninstKey -Name $p.Key -Value $p.Value }

Update-Shell
Write-Host "$Title installed to $InstallDir and .ave files are registered."
Write-Host 'If Windows asks how to open a .ave file the first time, choose Avestan Editor and "Always".'
