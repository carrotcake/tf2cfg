# Package a runnable folder: build/release/tf2cfgreal.exe plus the Qt runtime and QML
# modules windeployqt pulls in. Run from anywhere; paths are relative to this script.
#
#   pwsh -File deploy.ps1                  # release build -> dist/
#   pwsh -File deploy.ps1 -Config debug
#   pwsh -File deploy.ps1 -QtRoot C:\Qt\6.12.0\mingw_64 -Output D:\tf2
#
# The result is self-contained: the exe runs with no Qt bin directory on PATH.

[CmdletBinding()]
param(
    [ValidateSet('release', 'debug')]
    [string] $Config = 'release',

    # Qt kit root. Defaults to the kit this repo is built with.
    [string] $QtRoot = 'C:\Qt\6.12.0\mingw_64',

    [string] $Output = ''
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $Output) { $Output = Join-Path $repo 'dist' }

$exe = Join-Path $repo "build\$Config\tf2cfgreal.exe"
if (-not (Test-Path $exe)) {
    throw "No build at $exe. Build it first: cd build; qmake -spec win32-g++ ..\tf2cfgreal.pro CONFIG+=$Config; mingw32-make $Config"
}

$deploy = Join-Path $QtRoot 'bin\windeployqt.exe'
if (-not (Test-Path $deploy)) {
    throw "windeployqt not found at $deploy. Pass -QtRoot <kit path>."
}

# A clean target: windeployqt copies into it and never removes stale files, so leftovers
# from an older build would ship.
if (Test-Path $Output) { Remove-Item -Recurse -Force $Output }
New-Item -ItemType Directory -Force $Output | Out-Null

Copy-Item $exe $Output
& $deploy --$Config --compiler-runtime --qmldir (Join-Path $repo 'qml') $Output

Write-Host "Deployed to $Output"
Get-ChildItem $Output | Select-Object Name, Length