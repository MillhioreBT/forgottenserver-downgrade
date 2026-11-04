$ErrorActionPreference = 'Stop'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$visualStudio = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $visualStudio) {
    Write-Error 'Visual Studio with the C++ x64 tools was not found.'
}

$userVcpkgRoot = $env:VCPKG_ROOT
Import-Module (Join-Path $visualStudio 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $visualStudio -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
if ($userVcpkgRoot) {
    $env:VCPKG_ROOT = $userVcpkgRoot
}

Set-Location (Split-Path $PSScriptRoot)

cmake --preset test-vcpkg
if ($LASTEXITCODE) { exit $LASTEXITCODE }

cmake --build --preset test-vcpkg
if ($LASTEXITCODE) { exit $LASTEXITCODE }

ctest --preset test-vcpkg
exit $LASTEXITCODE
