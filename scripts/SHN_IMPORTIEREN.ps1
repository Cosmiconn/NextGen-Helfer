# Read-only data export from a user-selected SHN file. Never alters the source.
[CmdletBinding()]
param([string]$ShnPath)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$cli = Join-Path $root 'app\IsyaShnInspect.exe'
if (-not (Test-Path $cli)) {$cli = Join-Path $root 'build-win\Release\IsyaShnInspect.exe'}
if (-not (Test-Path $cli)) { throw 'IsyaShnInspect.exe fehlt. Bitte zuerst scripts\BUILD_WINDOWS.ps1 ausfuehren.' }
if (-not $ShnPath) {$ShnPath = Read-Host 'Pfad zu Produce.shn oder Production.shn (ohne Anfuehrungszeichen)'}
$ShnPath = $ShnPath.Trim('"')
if (-not (Test-Path -LiteralPath $ShnPath -PathType Leaf)) {throw 'SHN-Datei nicht gefunden. Bitte gueltigen lokalen Pfad angeben.'}
$inputFile = (Resolve-Path -LiteralPath $ShnPath).Path
$outDir = Join-Path $root 'exports'
New-Item -ItemType Directory -Path $outDir -Force | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$csv = Join-Path $outDir ("SHN-Rohdaten-$stamp.csv")
$schema = Join-Path $outDir ("SHN-Spalten-$stamp.txt")
& $cli $inputFile $csv $schema
if ($LASTEXITCODE -ne 0) {throw 'SHN konnte nicht geoeffnet werden. Bitte Spaltenschema/Formatversion pruefen.'}
Write-Host "SHN-Tabelle exportiert: $csv" -ForegroundColor Green
Write-Host "Spaltenuebersicht: $schema"
Write-Host 'Die Rohdaten sind noch KEINE bestaetigten Rezepte. Item-IDs und Mengenfelder muessen anhand echter Spalten zugeordnet werden.'
