# espboy
DIY Gameboy with ESP8266

Dieses kleine Amateurprojekt zeigt, was alles mit einem kostengünstigen ESP8266 (ESP-12e) möglich ist.

Materialkosten belaufen sich auf unter 20 €

------------------------
------------------------

## Changelog

### V1.2 (2025-07-19)

- Initial release

------------------------
------------------------

Enthaltene Spiele:

- Reversi
- 4 gewinnt
- Mini Mastermind
- Minesweeper
- Senso

------------------------
------------------------

Micro SD Karte mit FAT32 formattieren.

mp3 Dateien im Verzeichnis **content_for_sdcard** auf die SD Karte kopieren (Dateien 0001 bis 0004 einzeln kopieren. 
Der Rest kann dann gemeinsam darauf geschoben werden). 
Dies beschleunigt den Zugriff auf die ersten 4 mp3 Dateien.

1 Dupond Draht am Akku+ beim Akkuhalter anlöten (wird am Pin **+ Batterie** angeschlossen).

1 JST XH 4p am Akkuhalter anlöten.

2 JST XH an Tastatur und Display anlöten.

Bei den 4 und 8-poligen JST XH Kabeln beim Anlöten auf die Richtung achten!

**ESPboy_V1.2.ino** mittels Arduino IDE auf den ESP8266 flashen.

Stückliste ist in der Datei parts_list.txt zu finden.

Im Verzeichnis **gehaeuse** sind die 2 stl Dateien für den 3D Druck.

Im Verzeichnis **XGerber_v1.3** sind die Gerber Dateien um das PCB zu erstellen.
