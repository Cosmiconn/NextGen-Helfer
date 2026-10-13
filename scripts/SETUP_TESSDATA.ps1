# Download OCR models from the official Tesseract OCR tessdata_fast repository.
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$out=Join-Path $root 'app\tessdata'
New-Item -ItemType Directory -Path $out -Force | Out-Null
foreach($lang in @('deu','eng')) {
  $dst=Join-Path $out "$lang.traineddata"
  $uri="https://raw.githubusercontent.com/tesseract-ocr/tessdata_fast/main/$lang.traineddata"
  Write-Host "Lade $lang OCR-Sprachdatei ..."
  Invoke-WebRequest -Uri $uri -OutFile $dst
  if ((Get-Item $dst).Length -lt 1000000) {throw "Ungueltige/zu kleine Sprachdatei: $dst"}
}
Write-Host "OCR-Sprachdateien vorhanden: $out" -ForegroundColor Green
