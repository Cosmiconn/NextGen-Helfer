# Roadmap

## Phase 1 — GitHub Build (dieser Branch)

- [x] Native, leichte C++20-/Win32-Basis ohne Python und externe Paketmanager
- [x] CMake + GitHub Actions Windows x64
- [x] Automatische Tests und ZIP-Buildartefakt
- [x] Kleiner Wiki-Rohstoff-/Rezeptkatalog mit Quellen
- [ ] Windows-CI tatsächlich erfolgreich verifizieren
- [ ] Ursprüngliches Isya Gold 4.1 sauber importieren und Kern-Tests reparieren

## Phase 2 — bestehende Funktionen übernehmen

- [ ] Live-Shop-Scan: Bildschirmbereiche, Tooltip-OCR, Stückpreis, Preisverlauf, Lager
- [ ] Bedarfsliste und klickdurchlässige Welt-/Shop-Hinweise
- [ ] Produktdatenbank mit `Produce.shn`/`Production.shn` und ItemInfo-IDs verknüpfen
- [ ] Rohstoff-NIF-Referenzgalerie aus `reschar`, wo Modelle tatsächlich Rohstoffe darstellen
- [ ] Sichtbare Gegenstände per eingeblendeter Itemnamen erkennen; 3D-Modellabgleich optional
- [ ] Keine Automatisierung von Spielhandlungen und keine versteckten Datenzugriffe

## Phase 3 — Release

- [ ] Realer Windows-Client-Test, Datenverifikation auf deutschem offiziellen Server
- [ ] Portables Release, Signierung falls verfügbar, reproduzierbare Versionsnummer
- [ ] Erst nach bestandenen Tests: `main` aktualisieren

`IsyaGold_4.1_CPP_Weltmarker_SHN_Importer.zip` existiert als separates vorheriges Entwicklungsarchiv in der ChatGPT-Dateibibliothek; seine Quellcode-Migration ist **noch nicht** erfolgt. Dieses Repository enthält **noch nicht** dessen Live-Scanner.
