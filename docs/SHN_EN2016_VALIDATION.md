# SHN-Validierung: englische Fiesta-Online-Testdaten (2016)

**Stand: 13. Oktober 2026.** Dies ist ausdrücklich **NICHT** die aktuelle deutsche gamigo-Version.

Die drei lokal vom Benutzer bereitgestellten Dateien wurden mit dem C++20-SHN-Reader erfolgreich entschlüsselt:

| Tabelle | Datensätze | Spalten | Inhalt |
|---|---:|---:|---|
| `ItemInfo.shn` | 14.999 | 57 | IDs, englische Namen und Attribute |
| `Produce.shn` | 216 | 25 | Produkte, Zutaten, Mengen, Lot und Mastery |
| `Gather.shn` | 6 | 9 | Herbing/Mining/WoodCutting, Werkzeugprofile |

**Vollständiger Join-Test:** 216/216 Rezeptprodukte eindeutig über `Produce.Product` → `ItemInfo.InxName`. Ebenso 575/575 Zutatenreferenzen über `Raw0..Raw7`. 101 verschiedene Zutatenschlüssel; null unaufgelöste oder mehrdeutige referenzierte Items.

15 Stein-, 45 Trank-, 30 Rollen-, 63 Materialaufwertungs- und 63 Materialzerlegungsrezepte. Bei 126 Rezepten ist kein expliziter Tier in der Bezeichnung angegeben. Importwert ist dann `Tier=0` (nicht spezifiziert), nicht ein geschätzter Tier.

`Gather.shn` beschreibt **Sammelaktionen und Werkzeuganforderungen**, keine Karten-/Spawnkoordinaten oder NIF-Pfade.

Das aus den Originalen erzeugte ZIP `IsyaGold_EN2016_SHN_Testdaten_Export.zip` liegt in der ChatGPT-Dateibibliothek. Es enthält Master- und App-Import-CSV sowie Gather-Aktionen; die Original-SHN-Dateien werden nicht ins öffentliche GitHub eingecheckt.

Sobald die aktuellen deutschen Dateien vorliegen, werden Version, Schemas, Referenzen, Itemnamen und Mengen neu abgeglichen. Beide Datenbestände bleiben strikt getrennt.
