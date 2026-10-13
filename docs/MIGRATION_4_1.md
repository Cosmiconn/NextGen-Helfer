# Migration von 4.1

Stand: 13. Oktober 2026.

Die Quelldateien stammen unverändert oder nur zur Korrektur der Testdaten aus dem zuvor erstellten `IsyaGold_4.1_CPP_Weltmarker_SHN_Importer.zip`.

- `native/tests/test_shn.cpp`: Synthetische Spaltenbreite von `RecipeName` von 12 auf 13 Byte und Zeilenlänge entsprechend geändert. Die Beispielzeichenfolge `Bronze Hammer` umfasst 13 Zeichen.
- `native/tests/test_shn.cpp` und `native/tests/test_logic.cpp`: Assertions durch verpflichtende Laufzeitprüfungen ersetzt, sodass Release-CI echte Tests durchführt.
- Native Version und Weboberfläche bleiben als eigenständiger C++-Dienst und Browserseite erhalten. Qt wird nicht benötigt.
- Abhängigkeiten werden von GitHub Actions durch vcpkg kompiliert. Das Paket ist **nicht** vor einem grünen CI und einem Windows-Praxistest freigegeben.
