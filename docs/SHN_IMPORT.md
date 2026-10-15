# Produce.shn / Production.shn: geprüfter Importablauf

**Status 4.1:** Ein C++20 SHN-Reader extrahiert Tabellen aus vom Benutzer ausgewählten lokalen Dateien. Die **echte aktuelle Tabelle liegt noch nicht vor**, daher sind aktuell **null live verifizierte Rezepte** aus SHN eingelesen. Ausgabespalten werden absichtlich **nicht ohne Schema-Kontrolle** in die bestehende Rezeptdatenbank gemischt.

## Schritte auf Windows

1. Vorhandene `Production.shn` **oder** `Produce.shn` aus deiner lokalen, regulär erworbenen und installierten Client-Version bereithalten. Die abweichenden Dateinamen sind historisch in Umlauf. Nicht in Clientdateien schreiben.
2. `scripts/BUILD_WINDOWS.ps1` ausführen (Windows 10/11, MSVC, CMake, vcpkg).
3. In PowerShell `./scripts/SHN_IMPORTIEREN.ps1 -ShnPath 'C:\Pfad\zu\Produce.shn'` starten.
4. Unter `exports/` entstehen `SHN-Rohdaten-*.csv` und `SHN-Spalten-*.txt`. Die Eingabedatei bleibt unverändert.
5. Erst anhand der **tatsächlichen Spalten** Produkt-ID, Zutaten-IDs, Mengen, Skill-Anforderung und ggf. Herstellmenge zuordnen. Falls der Datensatz ausschließlich IDs enthält, sind zusätzliche, passende Namenstabellen (z. B. `ItemInfo.shn` und Lokalisierungsdaten) nötig.
6. Erst nach Zuordnung und Kontrollen gegen den aktuellen Client die Rezepte als bestätigte Produktionsdaten importieren. Ein entsprechendes Mapping/Join ist der nächste Arbeitsschritt mit den echten Dateien.

## Reader-Grenzen

- Unterstützt die in älteren Fiesta-SHN-Editoren dokumentierte SHN-Container- und Tabellenstruktur; andere Versionen melden einen Fehler, statt Werte zu erraten.
- Dekodiert übliche numerische SHN-Typen und Strings einschließlich variabler Textfelder.
- Für die 8-Bit-Texte wird derzeit Latin-1 angenommen. Die tatsächliche aktuelle Client-Codierung kann davon abweichen.
- Dateigrenze 64 MiB, Zeilen-/Spaltenlimits zur Absicherung. Unbekannte Datentypen werden **nicht** übersprungen.
- Ein Export bestätigt **nicht**, dass das SHN-File dem aktuellen offiziellen DE-Serverstand entspricht.
- Die Nutzung lokal installierter Clientdaten unterliegt den gamigo-Nutzungsbedingungen; für Eingriffe, Reverse Engineering oder Veränderung von Dateien liegt keine Freigabe vor.

## Referenz für die historische Datei-Struktur

- https://github.com/skeleten/SHN-Editor/blob/master/src/SHNDecryptor/Classes/SHNFile.cs
- https://www.elitepvpers.com/forum/fiesta-online/3751736-alchemy-produktion.html
