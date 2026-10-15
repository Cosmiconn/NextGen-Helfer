# Sichtbare Drops/Rohstoffe hervorheben – Welt-Scanner 4.1

## Funktionsprinzip

Diese erste C++20-Version liest **nur Bildschirm-Pixel** aus einer einmal von dir gewählten festen Region der Fiesta-Spielansicht. Tesseract liefert Koordinaten und Vertrauenswerte für erkannte Textzeilen. Wenn die sichtbare Textzeile **exakt** einem Eintrag in deiner Bedarfs-/Suchliste entspricht, zeichnet ein klickdurchlässiges, eigenständiges Windows-Fenster einen grünen Rahmen um den **sichtbaren Namen**.

- Auflösung: fest kalibrierte Rechtecke; bei Auflösungs-/UI-Wechsel erneut kalibrieren.
- Suchziele: dieselbe Liste, die bei Isya Gold 3.1/4.0 bereits für Rohstoffe genutzt wird.
- Für die Platzierung wird kein Spielerstandort, keine interne Minimap-Koordinate und kein Game-Memory verwendet.
- Bei Namen, die das Spiel erst nach Mouseover einblendet, muss der Nutzer die Maus selbst bewegen.
- **Kein** Erkennen von 3D-Assets ohne Textetikett, keine verdeckten Drops, keine versteckten Spawns, keine Welt-/Minimap-Koordinaten und keine automatischen Interaktionen.

## Anwendung

1. `app/STARTEN_CPP_SCANNER.bat` starten, im Browser **Shop-Analyse** öffnen und die **Benötigte Items/Sammelrohstoffe**-Liste füllen.
2. In der Scan-Sektion eine Bildschirmaufnahme anfordern und nach fünf Sekunden Fiesta anzeigen.
3. **Spielansicht / sichtbare Objektnamen** als Kalibrierregion wählen und ein möglichst enges Rechteck um den 3D-Spielbereich ziehen (ohne Chat, Inventar und HUD, soweit möglich).
4. Im Feld **Scanmodus** `Spielansicht: sichtbare Drops und Rohstoffe markieren` auswählen und **Scan starten** klicken.
5. Zurück zu Fiesta wechseln. Beschriftungen passender Suchziele werden nach erfolgreicher OCR in Grün markiert. Die Treffer stehen zusätzlich in der Browserübersicht.
6. Mit **Stoppen** werden die Markierungen beendet.

## Datenschutz, Spielregeln, Fehlerquellen

Passiv, nur Screen-Capture/OCR/externes Fenster. Keine Game-API, keine Eingaben, keine Hooking-/Memory-/Packet-Zugriffe. Gamigo hat das Tool nicht autorisiert; vor dem Einsatz die aktuellen Regeln beachten.

Text-Labels können anders heißen als in der Datenbank; viele 3D-Rohstoffobjekte haben keine dauerhafte Beschriftung. Die Erkennung ist deshalb **ein Prototyp**, kein garantierter 3D-Objektdetektor. Grenzfälle werden absichtlich nicht hervorgehoben. Fenster-/randloser Modus empfohlen.
