@echo off
setlocal
cd /d "%~dp0"
title Isya Gold 4.1 - Native C++ Scanner
if not exist "IsyaGoldNative.exe" (
  echo Der native C++ Scanner wurde noch nicht gebaut.
  echo Starte zuerst ..\scripts\BUILD_WINDOWS.ps1 in PowerShell.
  echo Anleitung: ..\README.md
  pause
  exit /b 1
)
if not exist "tessdata\eng.traineddata" (
  echo Tesseract OCR Sprachdaten fehlen.
  echo Starte ..\scripts\SETUP_TESSDATA.ps1 in PowerShell.
  pause
  exit /b 1
)
"%~dp0IsyaGoldNative.exe"
if errorlevel 1 (
  echo IsyaGoldNative ist mit einem Fehler beendet worden.
  pause
)
endlocal
