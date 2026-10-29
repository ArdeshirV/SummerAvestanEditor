param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$Executable,
    [Parameter(Mandatory=$true, Position=1)]
    [string]$Icon
)
$exe = (Resolve-Path $Executable).Path
$ico = (Resolve-Path $Icon).Path
$extensionKey = 'HKCU:\Software\Classes\.ave'
$programKey = 'HKCU:\Software\Classes\AvestanEditor.Document'
New-Item -Path $programKey -Force | Out-Null
Set-Item -Path $programKey -Value 'Avestan text document'
New-ItemProperty -Path $programKey -Name 'FriendlyTypeName' -Value 'Avestan text document' -PropertyType String -Force | Out-Null
New-Item -Path "$programKey\DefaultIcon" -Force | Out-Null
Set-Item -Path "$programKey\DefaultIcon" -Value "`"$ico`",0"
New-Item -Path "$programKey\shell\open\command" -Force | Out-Null
Set-Item -Path "$programKey\shell\open\command" -Value "`"$exe`" `"%1`""
New-Item -Path $extensionKey -Force | Out-Null
Set-Item -Path $extensionKey -Value 'AvestanEditor.Document'
New-ItemProperty -Path $extensionKey -Name 'Content Type' -Value 'application/x-avestan' -PropertyType String -Force | Out-Null
Write-Host 'Registered .ave for the current Windows user. Select Avestan Editor as default if Windows prompts.'
