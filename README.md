# ESPboy
DIY Gameboy with ESP8266

Dieses kleine Amateurprojekt soll zeigen, was mit einem kostengünstigen ESP8266 (ESP-12E) möglich ist.

Die Materialkosten belaufen sich auf unter 20 €

![ESPboy PCB Layout](gameboy_esp8266_PCB.PDF)

![ESPboy Schaltplan](gameboy_esp8266_SCH.PDF)

------------------------
------------------------

### Enthaltene Spiele:

- Reversi
- 4 gewinnt
- Mini Mastermind
- Minesweeper
- Senso
- Snake
- Tic Tac Toe

------------------------
------------------------

### Flashen:

**Variante 1:**

Mittels Arduino IDE die ino Datei hochladen.

**Variante 2:**

Mittels **esptool.exe (Windows)** oder **esptool (Linux)** die bin Datei hochladen.

Beispiel für Windows:

**esptool.exe -vv -cd nodemcu -cb 115200 -bz 4M -cp COM7 -ca 0x00000 -cf ESPboy_V1.4.bin**

Beispiel für Linux:

**sudo ./esptool -vv -cd nodemcu -cb 115200 -bz 4M -cp /dev/ttyUSB0 -ca 0x00000 -cf ESPboy_V1.4.bin**


**Port** ( /dev/ttyUSB0 bzw COM7 ) und **bin** ( ESPboy_V1.4.bin ) ggf. **anpassen**.

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

**ESPboy_V1.4.ino** mittels Arduino IDE auf den ESP8266 flashen.

Stückliste ist in der Datei **parts_list.txt** zu finden.

Im Verzeichnis **gehaeuse** sind die 2 stl Dateien für den 3D Druck.

Im Verzeichnis **XGerber_v1.4** sind die Gerber Dateien um das PCB zu erstellen.

Die Datei **gameboy_esp8266.T3001** kann mit **Target3001!** geöffnet werden (rein informativ).

Jumper K4 kann normalerweise auf 2-3 gesetzt werden.

Jumper K5 kann normalerweise auf 1-2 gesetzt werden.

------------------------
------------------------

## Changelog

### V2.0 (2026-01-10)
- Tic Tac Toe added

### V1.9 (2026-01-08)
- Pin7 for jumper added. Now is pin1 or pin7 supported

### V1.8 (2025-12-29)
- Game info display standardized

### V1.7 (2025-12-28)
- Snake game added

### V1.6 (2025-12-23)
- 1.8 or 2.8 inch display support

### V1.5 (2025-12-13)
- View changelog added

### V1.4 (2025-12-13)
- Over-the-Air (OTA) Update added

### V1.3 (2025-12-12)
- Improved button operation
- Minesweeper runs more smoothly

### V1.2 (2025-07-19)
- Reversi
- 4 gewinnt
- Mini Mastermind
- Minesweeper
- Senso
- Update info

