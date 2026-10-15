# Lokaler Import. EN-2016-Testdaten niemals mit aktueller DE-Version vermischen.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$SourceDirectory,[string]$DestinationDirectory='')
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$SourceDirectory=(Resolve-Path -LiteralPath $SourceDirectory).Path
$exe=Join-Path $root 'app\IsyaShnCatalog.exe'
if (-not (Test-Path $exe)) {$exe=Join-Path $root 'build-win\Release\IsyaShnCatalog.exe'}
if (-not (Test-Path $exe)) {throw 'IsyaShnCatalog.exe fehlt. GitHub Windows-Build herunterladen.'}
if (-not $DestinationDirectory) {$DestinationDirectory=Join-Path $root 'exports\EN2016-Test'}
New-Item -ItemType Directory -Path $DestinationDirectory -Force | Out-Null
$produce=Join-Path $SourceDirectory 'Produce.shn'
if (-not (Test-Path -LiteralPath $produce)) {$produce=Join-Path $SourceDirectory 'Production.shn'}
$item=Join-Path $SourceDirectory 'ItemInfo.shn'
$gather=Join-Path $SourceDirectory 'Gather.shn'
foreach ($f in @($produce,$item,$gather)) {if (-not (Test-Path -LiteralPath $f -PathType Leaf)) {throw "SHN fehlt: $f"}}
& $exe $produce $item $gather $DestinationDirectory
if ($LASTEXITCODE -ne 0) {throw 'Unbekanntes SHN-Schema oder Verknuepfungsfehler. Kein Import.'}
Write-Host 'Testrezepte exportiert; IsyaGold-Import-EN2016.csv in Isya Gold unter Rezeptdatenbank importieren.'
Write-Host 'Historische EN 2016 Testdaten, keine aktuellen deutschen Spielwerte.'