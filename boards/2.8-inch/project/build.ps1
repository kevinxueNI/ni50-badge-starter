. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ProjectRoot
idf.py fullclean
idf.py build
