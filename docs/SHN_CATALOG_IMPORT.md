# SHN-Katalogimport – englische 2016er Testdateien

Die SHN-Dateien werden nur lokal, schreibgeschützt gelesen. Sie sind NICHT die aktuelle deutsche Fiesta-Version. Originale nicht im öffentlichen Repo speichern.

GitHub Actions liefert nach erfolgreichem Native-Windows-Build IsyaShnCatalog.exe im app-Verzeichnis.

Alle drei Dateien derselben Version in einen Ordner kopieren: Produce.shn, ItemInfo.shn und Gather.shn.

PowerShell-Befehl: powershell -ExecutionPolicy Bypass -File scripts/SHN_REZEPTE_IMPORTIEREN.ps1 -SourceDirectory 'C:\Fiesta-EN2016'

Die Ergebnisse liegen dann im Ordner exports/EN2016-Test:
- recipes-master-2016.csv: Rezept-/Item-IDs, Mengen, Meisterschaft
- IsyaGold-Import-EN2016.csv: importierbare Semikolon-CSV für die Rezeptdatenbank
- gather-actions-2016.csv: Werkzeuge und Sammelaktionen, keine Fundorte
- import-report-2016.txt: technische Zusammenfassung

Später die deutschen Dateien separat importieren und die Datenbestände ausdrücklich getrennt halten.