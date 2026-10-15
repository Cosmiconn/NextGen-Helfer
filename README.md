# NextGen-Helfer – Isya Gold 4.1 (C++20)

Dieses Repository enthält jetzt den ursprünglichen Isya-Gold-4.1-Entwicklungsstand als **native C++20 Windows-Anwendung mit lokaler Weboberfläche**, zusätzlich zum bisherigen kleinen Win32-Wiki-Datenbrowser.

## Build herunterladen

Öffne **Actions → Isya Gold 4.1 - Native Windows OCR Build → erfolgreicher Lauf → Artifacts → IsyaGold-4.1-Windows-x64**. ZIP entpacken und **`app/STARTEN_CPP_SCANNER.bat`** starten. Der Browser öffnet **http://127.0.0.1:8765**.

**Der Build muss erfolgreich durchlaufen, bevor die EXE als verfügbar gilt.** Der Quellcode und der CI-Workflow allein sind kein Beweis, dass der komplette Windows-Live-Scanner funktioniert.

- Inklusive Bildschirm-OCR und Shop-Preisprüfung (manuell zu bestätigende Ergebnisse)
- Bedarfsliste, Einkaufsgrenzen, Rohstoff- und Drop-Namenmarker über sichtbaren Beschriftungen
- Vollständige ältere Web-Wirtschaftsoberfläche: Rezepte, Crafting-Kalkulation, Ressourcen- und NPC-Finder, Shop-Journal, Klassen- und Level-Guides
- `IsyaShnInspect.exe`: SHN-Tabelle lesend exportieren und Schema inspizieren; aktuelle `Produce.shn` benötigt, um echte Rezepte zuordnen zu können

## Architektur und Build

- `app/` – Shop- und Wirtschaftsdashboard, Live-Scanner-Frontend
- `native/` – OCR, Bildschirmaufnahme, lokaler HTTP-Dienst, Weltmarker, SHN-Reader und Tests
- `scripts/` – lokaler Windows-Build, OCR-Setup, SHN-Datenexport
- `.github/workflows/isya-gold-native.yml` – Linux-Kerntests und MSVC-Windows-Build mit vcpkg
- `src/`, `tests/`, `data/` – erste experimentelle Win32-Wiki-Browser-Basis, separat in `.github/workflows/windows-build.yml` gebaut

Zum lokalen Bauen: **Visual Studio 2022 C++ Desktop, CMake, Git** installieren, dann `scripts/BUILD_WINDOWS.ps1` starten; vcpkg stellt die weiteren Abhängigkeiten bereit.

## Test- und Datenstatus

**Wichtig:** Erst die Tests und ein manueller Windows-Test entscheiden, ob die Migration vollständig funktioniert. Der Linux-SHN-Test aus 4.1 wurde wegen falscher synthetischer Testdaten berichtigt; er prüft weiterhin **keine echte aktuelle SHN-Datei**. Die 3D-Rohstoffmodell-Erkennung aus NIFs ist noch nicht umgesetzt. Der Marker erkennt sichtbare Textbeschriftungen, keine verborgenen Drops oder Spawnpunkte.

Die älteren Fiesta-Wiki-Daten sind *nicht* auf Aktualität für deutsche offizielle Server verifiziert. Es gibt keine automatisierten Spielaktionen, Client-Injektion oder Speicher-/Netzwerkmanipulation. Für externe Overlays liegt keine ausdrückliche Betreiberfreigabe vor.

Siehe [docs/ROADMAP.md](docs/ROADMAP.md) und [docs/WELT_MARKIERUNG.md](docs/WELT_MARKIERUNG.md).
