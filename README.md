# NextGen-Helfer / Isya Gold

**Status:** initialer nativer **C++20-Windows-Build** mit einem kleinen Wiki-Datenbrowser. Der umfangreiche Live-Scanner aus Isya Gold 4.1 ist **noch nicht** in dieses Repository migriert. Diese Basis soll zuerst reproduzierbar unter Windows bauen und testbar sein.

## Windows-EXE direkt von GitHub laden

1. Im Repo **Actions** öffnen.
2. **Windows x64 – Native Build** wählen; den aktuellen erfolgreichen Run öffnen.
3. Unter **Artifacts** die Datei `NextGen-Helfer-Windows-x64` herunterladen.
4. ZIP entpacken und `NextGenHelfer.exe` starten. `data/resources.tsv` neben der EXE belassen.

Neue Commits und Pull Requests lösen automatisiert einen Windows-Build und Tests aus. Das Build-Artefakt ist **kein offizielles Release** und kann gemäß GitHub-Aufbewahrungsfristen ablaufen.

## Lokaler Windows-Build (ohne vcpkg, Python oder Qt)

Benötigt: Visual Studio 2022 mit Desktopentwicklung C++ und CMake.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Die ausführbare Datei liegt unter `build/Release/NextGenHelfer.exe`. Für einen manuellen Test den Ordner `data` neben die EXE kopieren.

## Was bereits enthalten ist

- Native Win32-Oberfläche zum Durchsuchen ausgewählter **Fiesta-Wiki-Rohstoffe und -Rezepte** und Öffnen der Belegquellen.
- CSV-/TSV-Datenimport aus `data/resources.tsv` (UTF-8).
- Automatischer MSVC-x64-Build mit Tests und herunterladbarem ZIP.
- Kein automatisiertes Spielen, keine Mausklicks und keine Manipulation des Fiesta-Clients.

Die Wiki-Daten sind historische Community-Angaben und **nicht** als Oktober-2026-Live-Stand zu verstehen. Genaue Herkunft steht in [docs/DATA_SOURCES.md](docs/DATA_SOURCES.md).

## Ausblick

Siehe [docs/ROADMAP.md](docs/ROADMAP.md). Der bisherige 4.1-Quellstand ist ein separates Archiv und wird erst nach bestandenen Build-/Kompatibilitätstests modular übernommen. Künftige SHN-Importe brauchen echte aktuelle `Produce.shn`-/`ItemInfo.shn`-Dateien.
