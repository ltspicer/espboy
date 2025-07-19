# espboy
DIY Gameboy with ESP8266

Dieses kleine Amateurprojekt soll zeigen, was mit einem kostengünstigen ESP8266 (ESP-12E) möglich ist.

Die Materialkosten belaufen sich auf unter 20 €

------------------------
------------------------

Enthaltene Spiele (V1.2):

- Reversi
- 4 gewinnt
- Mini Mastermind
- Minesweeper
- Senso

------------------------
------------------------

Micro SD Karte mit **FAT32** formatieren.

mp3 Dateien im Verzeichnis **content_for_sdcard** in den Root (ohne Verzeichnis) der SD Karte kopieren (Dateien 0001 bis 0004 **einzeln** kopieren. 
Der Rest kann dann gemeinsam darauf geschoben werden). 
Dies beschleunigt den Zugriff auf die ersten 4 mp3 Dateien (wichtig für das Spiel Senso).

1 Stück Dupond Draht am Akku+ beim Akkuhalter anlöten (wird am PCB Pin **+ Batterie** angeschlossen).

1 Stück JST XH 4p Kabelsatz am Akkuhalter anlöten (GND, 3V, 5V).

2 Stück JST XH 8p Kabelsatz je an Tastatur und Display anlöten.

2 Stück JST XH 2p Kabelsatz je an Schalter und Lautsprecher anlöten.

Bei den 4 und 8-poligen JST XH Kabelsätzen beim Anlöten auf die Richtung achten (Pin 1 auf der richtigen Seite)!

Für ESP8266 und DF_Player_mini einen Pinheader weiblich 1-reihig verwenden (quasi als Sockel).

**ESPboy_V1.2.ino** mittels Arduino IDE auf den ESP8266 flashen.

Stückliste ist in der Datei **parts_list.txt** zu finden.

Im Verzeichnis **gehaeuse** sind die 2 stl Dateien für den 3D Druck.

Im Verzeichnis **XGerber_v1.4** sind die Gerber Dateien um das PCB zu erstellen.

Die Datei **gameboy_esp8266.T3001** kann mit **Target3001!** geöffnet werden (rein informativ).

Jumper K4 kann normalerweise auf 2-3 gesetzt werden.

Jumper K5 kann normalerweise auf 1-2 gesetzt werden.

------------------------
------------------------

## Changelog

### V1.2 (2025-07-19)

- Initial release

