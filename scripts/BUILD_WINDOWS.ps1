# Build IsyaGoldNative (C++20, MSVC x64, vcpkg). No Python is installed or used.
[CmdletBinding()]
param([switch]$SkipVcpkgBootstrap)
$ErrorActionPreference='Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$src = Join-Path $root 'native'
$build = Join-Path $root 'build-win'
Write-Host 'Isya Gold 4.1 - Windows C++20 Build' -ForegroundColor Cyan
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {throw 'CMake fehlt. Installiere CMake und starte das Skript neu.'}
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {throw 'Git fehlt. Installiere Git for Windows.'}
if (-not (Test-Path "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe")) {throw 'Visual Studio 2022 C++ Build Tools (MSVC/Windows SDK) fehlen.'}
$vwhere = Join-Path ${env:ProgramFiles} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vwhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) {throw 'Visual Studio C++ Build Tools fehlen: MSVC x64 installieren.'}
$vcpkg = $env:VCPKG_ROOT
if (-not $vcpkg) { $vcpkg = Join-Path $root '.tools\vcpkg' }
if (-not (Test-Path (Join-Path $vcpkg 'scripts\buildsystems\vcpkg.cmake'))) {
  if ($SkipVcpkgBootstrap) { throw "vcpkg fehlt: $vcpkg" }
  New-Item -ItemType Directory -Path (Split-Path $vcpkg -Parent) -Force | Out-Null
  Write-Host 'vcpkg wird aus dem offiziellen Microsoft-Repository geholt ...'
  & git clone --depth 1 https://github.com/microsoft/vcpkg.git $vcpkg
  if ($LASTEXITCODE -ne 0) {throw 'Git clone fuer vcpkg fehlgeschlagen.'}
  & (Join-Path $vcpkg 'bootstrap-vcpkg.bat') -disableMetrics
  if ($LASTEXITCODE -ne 0) {throw 'vcpkg Bootstrap fehlgeschlagen.'}
}
$toolchain = Join-Path $vcpkg 'scripts\buildsystems\vcpkg.cmake'
Write-Host 'Konfiguriere Release (x64-windows-static) ...'
& cmake -S $src -B $build -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_TOOLCHAIN_FILE=$toolchain" '-DVCPKG_TARGET_TRIPLET=x64-windows-static'
if ($LASTEXITCODE -ne 0) {throw 'CMake Konfiguration fehlgeschlagen. Build Tools und vcpkg Prerequisites pruefen.'}
& cmake --build $build --config Release --parallel 4
if ($LASTEXITCODE -ne 0) {throw 'C++ Build fehlgeschlagen.'}
& ctest --test-dir $build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) {throw 'C++ Tests fehlgeschlagen.'}
$exe = Join-Path $root 'app\IsyaGoldNative.exe'
if (-not (Test-Path $exe)) {throw "EXE nicht gefunden: $exe"}
$cli = Join-Path $build 'Release\IsyaShnInspect.exe'
if (Test-Path $cli) {Copy-Item -Path $cli -Destination (Join-Path $root 'app\IsyaShnInspect.exe') -Force}
Write-Host "Build erfolgreich: $exe" -ForegroundColor Green
Write-Host 'Als naechstes scripts\SETUP_TESSDATA.ps1 ausfuehren und app\STARTEN_CPP_SCANNER.bat starten.'
