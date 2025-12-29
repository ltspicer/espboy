///////////////////////////
//  ESPboy Taschenspiel  //
// (C) 2025 D. Luginbühl //
//  www.ltspiceusers.ch  //
///////////////////////////

bool smallDisplay = true;

#define D0 16
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <Updater.h>
#include <PCF8574.h>
#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"
#include <SoftwareSerial.h>
#include <iostream>
#include <vector>
#include <limits.h>

// Version
const String VERSION = "V1.8";

// Aliase
#define D0 16  // D0 alias für GPIO16
#define D8 15  // D8 alias für GPIO15
#define D4 2   // D4 alias für GPIO2

// SPI-Pins für Display
#define TFT_CS D8
#define TFT_RST D0  // -1 falls nicht angeschlossen
#define TFT_DC D4

// PCF8574 für Tastatur (I2C-Adresse: 0x20)
#define PCF8574_ADDR_1 0x20
// PCF8574 für Auto-Power-Off (I2C-Adresse: 0x21)
#define PCF8574_ADDR_2 0x21

// Instanzen für die zwei PCF8574 Expander
PCF8574 pcf1(PCF8574_ADDR_1);  // Erster Expander
PCF8574 pcf2(PCF8574_ADDR_2);  // Zweiter Expander

// ST7735 Display initialisieren
Adafruit_ST7735 tft7735 = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_ST7789 tft7789 = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_GFX* tft = nullptr;

// Displaygröße
uint16_t TFT_WIDTH = 0;
uint16_t TFT_HEIGHT = 0;

// Spielfeld-Parameter
byte radius = 0;
uint16_t xStart = 0;
uint16_t yStart = 0;
uint16_t xEnd = 0;
uint16_t yEnd = 0;

uint8_t scale = 1;

byte SPALTEN = 7;
byte ZEILEN = 6;

uint16_t xStep = 0;
byte yStep = 0;

// Hi-Score return Werte
struct Hiscore {
  String name;
  int winner;
  int looser;
};

struct Zug {
  int x, y;
};

void initDisplay(bool smallDisplay) {
  if (smallDisplay) {
    tft = &tft7735;
    tft7735.initR(INITR_BLACKTAB);
    tft7735.setRotation(1);

    TFT_WIDTH = 128;
    TFT_HEIGHT = 160;

    radius = 7;
    xStart = 11;
    yStart = 19;
    xEnd = 143;
    yEnd = 99;

    scale = 1;

  } else {
    tft = &tft7789;
    tft7789.init(240, 320);
    tft->invertDisplay(false);
    tft7789.setRotation(3);

    TFT_WIDTH = 240;
    TFT_HEIGHT = 320;

    radius = 14;
    xStart = 22;
    yStart = 38;
    xEnd = 286;
    yEnd = 198;

    scale = 2;
  }
  xStep = (xEnd - xStart) / (SPALTEN - 1);
  yStep = (yEnd - yStart) / (ZEILEN - 1);
  Wire.begin(D3, D6);  // SDA = D3 (GPIO0), SCL = D6 (GPIO12) erneut setzen
}

// FTP
String ssid = "";
String passwd = "";
const char* ftp_server = "espboy.istmein.de";
const int ftp_port = 4321;
const char* ftp_user = "espboy";
const char* ftp_pass = "esp8266";
String loadVersionFromLittleFS();
String infoFile = "";
bool newVersionAvailable = false;
float newVersion = 0.00;
float version = 0.00;
WiFiClient client;  // Globale Variable für die FTP-Steuerverbindung

// Symbole
// 8x8 Pfeil nach links
const byte backspaceBitmap[] PROGMEM = {
  0b00000000,
  0b00100000,
  0b01000000,
  0b11111111,
  0b01000000,
  0b00100000,
  0b00000000,
  0b00000000
};

// 8x8 Pfeil nach rechts
const byte rightBitmap[] PROGMEM = {
  0b00000000,
  0b00000100,
  0b00000010,
  0b11111111,
  0b00000010,
  0b00000100,
  0b00000000,
  0b00000000
};

// 8x8 Pfeil nach unten
const byte downBitmap[] PROGMEM = {
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b10010010,
  0b01010100,
  0b00111000,
  0b00010000
};

// 8x8 Pfeil nach unten
const byte upBitmap[] PROGMEM = {
  0b00010000,
  0b00111000,
  0b01010100,
  0b10010010,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000
};

// 8x8 RETURN-Pfeil
const byte returnBitmap[] PROGMEM = {
  0b00000000,
  0b00100001,
  0b01000001,
  0b11111111,
  0b01000000,
  0b00100000,
  0b00000000,
  0b00000000
};

// 10x10 Taste 0
const byte button0Bitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10011110, 0b01000000,
  0b10100001, 0b01000000,
  0b10100001, 0b01000000,
  0b10100001, 0b01000000,
  0b10100001, 0b01000000,
  0b10011110, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste 1
const byte button1Bitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10000010, 0b01000000,
  0b10000110, 0b01000000,
  0b10001010, 0b01000000,
  0b10000010, 0b01000000,
  0b10000010, 0b01000000,
  0b10000010, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste 2 - Korrigierte Bitmap
const byte button2Bitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10011110, 0b01000000,
  0b10100001, 0b01000000,
  0b10000011, 0b01000000,
  0b10001100, 0b01000000,
  0b10110000, 0b01000000,
  0b10111111, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste 3
const byte button3Bitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10111110, 0b01000000,
  0b10000001, 0b01000000,
  0b10011110, 0b01000000,
  0b10000001, 0b01000000,
  0b10000001, 0b01000000,
  0b10111110, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste 4
const byte button4Bitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10000010, 0b01000000,
  0b10100010, 0b01000000,
  0b10100010, 0b01000000,
  0b10111111, 0b01000000,
  0b10000010, 0b01000000,
  0b10000010, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste 5
const byte button5Bitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10111110, 0b01000000,
  0b10100000, 0b01000000,
  0b10111110, 0b01000000,
  0b10000001, 0b01000000,
  0b10000001, 0b01000000,
  0b10111110, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste 6
const byte button6Bitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10011100, 0b01000000,
  0b10100000, 0b01000000,
  0b10111110, 0b01000000,
  0b10100001, 0b01000000,
  0b10100001, 0b01000000,
  0b10011110, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste 7
const byte button7Bitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10111111, 0b01000000,
  0b10000010, 0b01000000,
  0b10000100, 0b01000000,
  0b10001000, 0b01000000,
  0b10010000, 0b01000000,
  0b10100000, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste 8
const byte button8Bitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10011110, 0b01000000,
  0b10100001, 0b01000000,
  0b10011110, 0b01000000,
  0b10100001, 0b01000000,
  0b10100001, 0b01000000,
  0b10011110, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste 9
const byte button9Bitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10011110, 0b01000000,
  0b10100001, 0b01000000,
  0b10011111, 0b01000000,
  0b10000001, 0b01000000,
  0b10000010, 0b01000000,
  0b10011100, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste A
const byte buttonABitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10011110, 0b01000000,
  0b10100001, 0b01000000,
  0b10111111, 0b01000000,
  0b10100001, 0b01000000,
  0b10100001, 0b01000000,
  0b10100001, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste B
const byte buttonBBitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10111110, 0b01000000,
  0b10100001, 0b01000000,
  0b10111110, 0b01000000,
  0b10100001, 0b01000000,
  0b10100001, 0b01000000,
  0b10111110, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste C
const byte buttonCBitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10011111, 0b01000000,
  0b10100000, 0b01000000,
  0b10100000, 0b01000000,
  0b10100000, 0b01000000,
  0b10100000, 0b01000000,
  0b10011111, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste D
const byte buttonDBitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10111110, 0b01000000,
  0b10100001, 0b01000000,
  0b10100001, 0b01000000,
  0b10100001, 0b01000000,
  0b10100001, 0b01000000,
  0b10111110, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste *
const byte buttonStarBitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10010010, 0b01000000,
  0b10001100, 0b01000000,
  0b10111111, 0b01000000,
  0b10001100, 0b01000000,
  0b10010010, 0b01000000,
  0b10000000, 0b01000000,
  0b10000000, 0b01000000,
  0b11111111, 0b11000000
};

// 10x10 Taste #
const byte buttonHashBitmap[] PROGMEM = {
  0b11111111, 0b11000000,
  0b10000000, 0b01000000,
  0b10001010, 0b01000000,
  0b10001010, 0b01000000,
  0b10111111, 0b11000000,
  0b10001010, 0b01000000,
  0b10111111, 0b11000000,
  0b10001010, 0b01000000,
  0b10001010, 0b01000000,
  0b11111111, 0b11000000
};

// Allgemeine Variabeln
uint16_t bgColor = ST77XX_BLACK;
uint16_t spielerColor = ST77XX_RED;
uint16_t computerColor = ST77XX_YELLOW;
uint16_t iconColor = ST77XX_WHITE;
uint16_t textColor = ST77XX_WHITE;
const byte LEER = 0;
const byte SPIELER = 1;
const byte COMPUTER = 2;
byte** spielfeld;
bool spielerDran = true;  // Wechselt nach jedem Zug
// Für Akkuberechnung
const int anzahlAkkuWerte = 20;  // Anzahl der letzten Werte für den Durchschnitt
int akkuWerte[anzahlAkkuWerte];
int batterieSpannung = 240;       // Akkuspannung an A0 Variabel Init
const int akkuObererWert = 243;   // 100% Akku Wert an A0
const int akkuUntererWert = 170;  // 0% Akku Wert an A0
bool online = false;              // Wifi verbunden?

// Für Karussell
byte ziffer1, ziffer2, ziffer3;
byte messageY = 0;

// Für 4 gewinnt
byte pfeilPos = SPALTEN / 2;  // Startposition des Pfeils in der Mitte
const int MAX_DEPTH = 4;      // Maximale Tiefe des Minimax

// Für Reversi
int xPos = 0;
int yPos = 0;

const int richtungen[8][2] = {  // Richtungen für das Überprüfen von Flips
  { -1, -1 },
  { 0, -1 },
  { 1, -1 },
  { -1, 0 },
  { 1, 0 },
  { -1, 1 },
  { 0, 1 },
  { 1, 1 }
};

// Für Minesweeper
#define FIELD_WIDTH 10  // 10x8 Felder
#define FIELD_HEIGHT 8
uint16_t TILE_SIZE = 0;

#define ST77XX_LIGHTGREY 0xCE79   // Hellgrau
#define ST77XX_MIDGREY 0x8C71     // Mittelgrau
#define ST77XX_DARKGREY 0x52AA    // Dunkelgrau
#define ST77XX_DARKBLUE 0x0011    // Dunkelblau
#define ST77XX_DARKGREEN 0x03E0   // Dunkelgrün
#define ST77XX_WINERED 0x780F     // Weinrot
#define ST77XX_DARKRED 0x6000     // Dunkelrot
#define ST77XX_DARKYELLOW 0xA540  // Dunkelgelb

byte grid[FIELD_HEIGHT][FIELD_WIDTH];      // 0-8 = Zahl, 9 = Mine
bool revealed[FIELD_HEIGHT][FIELD_WIDTH];  // Aufgedeckte Felder
bool flagged[FIELD_HEIGHT][FIELD_WIDTH];   // Gesetze Fahnen
byte NUM_MINES = 10;

int cursorX = 0;
int cursorY = 0;
int oldCursorX = 0;
int oldCursorY = 0;

// Für Senso
#define GAME_STATUS_END 1
#define GAME_STATUS_CANCELED 3
#define GAME_STATUS_RUNNING 2
#define BUTTON_MAX_INTERVAL 4000  // Antwortzeit bis Game over
#define DELAY_INIT 800            // Geschwindigkeit (Verzögerung) Startwert
#define DELAY_MIN 300             // Geschwindigkeit (Verzögerung) Minimum (300)
#define DELAY_STEP 50             // Geschwindigkeit (Verzögerung) Verkleinerungsschritte

#define SENSO_FELD_ROT 0
#define SENSO_FELD_GELB 1
#define SENSO_FELD_GRUEN 2
#define SENSO_FELD_BLAU 3

#define SENSO_NO_PIN 255  // Keine Taste gedrückt

// --- Layout-Konstanten ---
uint16_t LOGICAL_WIDTH = 0;
uint16_t LOGICAL_HEIGHT = 0;

uint16_t BTN_SIZE = 0;
uint16_t BTN_RADIUS = 0;

uint16_t CENTER_X = 0;
uint16_t CENTER_Y = 0;

uint16_t OFFSET_X = 0;
uint16_t OFFSET_Y = 0;

uint16_t POS_ROT_X = 0;
uint16_t POS_ROT_Y = 0;

uint16_t POS_GELB_X = 0;
uint16_t POS_GELB_Y = 0;

uint16_t POS_GRUEN_X = 0;
uint16_t POS_GRUEN_Y = 0;

uint16_t POS_BLAU_X = 0;
uint16_t POS_BLAU_Y = 0;

bool isWaitingForPlayer, isColorPlayed;
byte sensoScore, sensoGameStatus, sensoSequenceIndex, pressedButton, sensoSequence[100], sensoTon[4];
int gameDelay;
unsigned long lastPlayerAction, lastButtonPressed;

void drawBitmapScaled(int16_t x, int16_t y,
                      const uint8_t* bitmap,
                      int16_t w, int16_t h,
                      uint16_t color) {
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {

      uint8_t byte = pgm_read_byte(bitmap + (j * ((w + 7) / 8)) + (i / 8));
      bool pixelOn = byte & (0x80 >> (i & 7));

      if (pixelOn) {
        tft->fillRect(x + i * scale, y + j * scale, scale, scale, color);
      }
    }
  }
}

// Für Snake
#define snakesize 250
//Snake's speed between movement(miliseconds)
#define velocity 250

#define gridx 30
#define gridy 20

byte snakeBuffer[snakesize][2];
byte snakeGrid[gridy][gridx];

// Serielle Schnittstelle für DF Player
SoftwareSerial softSerial(D1, D2);  // RX, TX Pins
#define FPSerial softSerial

// DF Player
DFRobotDFPlayerMini myDFPlayer;
void printDetail(byte type, byte value);

// Für Auto-Power-Off
#define SLEEP_TIME 1800000  // 30 Minuten
//#define SLEEP_TIME 30000   // 30 Sekunden zum testen

// Für Sound
byte trackNummer = 0;

// Für Menu
byte auswahl = 0;

// Für Aktivität (Tastendruck)
unsigned long lastActivity;

// Tasten-Matrix
const char keys[4][4] = {
  { '1', '4', '7', '*' },
  { '2', '5', '8', '0' },
  { '3', '6', '9', '#' },
  { 'A', 'B', 'C', 'D' }
};

// Pin-Zuordnung PCF8574 <-> 4x4-Tastatur
const byte COLUMN_PINS[4] = { 0, 1, 2, 3 };  // P0-P3 als Spalten
const byte ROW_PINS[4] = { 4, 5, 6, 7 };     // P4-P7 als Reihen

const char allowedChars[] = "0123456789ABCD*#";  // Erlaubte Zeichen (Tastaturfilter)

// Characters für Wifi Eingabe
const String characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz 1234567890!@#$%^&*()_-+=<>?[]{}|\\:;,.";
int currentIndex = 0;  // Startindex, der anzeigt, welches Zeichen gerade ausgewählt ist

// Characters für Namen Eingabe
const String characters2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.";
int currentIndex2 = 0;  // Startindex, der anzeigt, welches Zeichen gerade ausgewählt ist

// Funktionen für Spielsteuerungs-Info Bildschirm

void printCentered(const String &text, uint16_t y, uint8_t textSize) {
  tft->setTextSize(textSize);

  uint16_t charWidth = 6 * textSize;
  uint16_t textWidth = text.length() * charWidth;

  uint16_t screenWidth = smallDisplay ? 160 : 320;
  uint16_t x = (screenWidth - textWidth) / 2;

  tft->setCursor(x, y);
  tft->print(text);
}

void gameControllInfo(String title, String infoLine1, String infoLine2, bool showKey5) {

  uint16_t titleSize = smallDisplay ? 2 : 4;
  uint16_t titleX = smallDisplay ? 40 : 80;
  uint16_t titleY = 10;

  uint16_t textSize = smallDisplay ? 1 : 2;
  uint16_t textY = smallDisplay ? 34 : 44;

  uint16_t abortX = smallDisplay ? 88 : 88;
  uint16_t abortY = smallDisplay ? 119 : 218;

  uint16_t leftBtnX = smallDisplay ? 29 : 58;
  uint16_t leftArrowX = smallDisplay ? 20 : 40;
  uint16_t leftArrowY = smallDisplay ? 80 : 120;

  uint16_t rightBtnX = smallDisplay ? 120 : 240;
  uint16_t rightArrowX = smallDisplay ? 132 : 264;
  uint16_t rightArrowY = smallDisplay ? 80 : 120;

  uint16_t setBtnX = smallDisplay ? 10 : 20;
  uint16_t setBtnY = smallDisplay ? 119 : 174;

  uint16_t upBtnX = smallDisplay ? 76 : 152;
  uint16_t upBtnY = smallDisplay ? 67 : 97;
  uint16_t upArrowX = smallDisplay ? 78 : 156;
  uint16_t upArrowY = smallDisplay ? 57 : 77;

  uint16_t downBtnX = smallDisplay ? 76 : 152;
  uint16_t downBtnY = smallDisplay ? 93 : 139;
  uint16_t downArrowX = smallDisplay ? 78 : 156;
  uint16_t downArrowY = smallDisplay ? 104 : 161;

  printCentered(title, titleY, titleSize);
  printCentered(infoLine1, textY, textSize);
  printCentered(infoLine2, textY + (smallDisplay ? 9 : 18), textSize);

  // Taste A (Abbruch)
  displayButtonAndText(abortX, abortY, "Abbruch", 'A');

  // Taste 4 (links)
  displayButtonAndText(leftBtnX, leftArrowY, "", '4');
  drawBitmapScaled(leftArrowX, leftArrowY, backspaceBitmap, 8, 8, iconColor);

  // Taste 6 (rechts)
  displayButtonAndText(rightBtnX, rightArrowY, "", '6');
  drawBitmapScaled(rightArrowX, rightArrowY, rightBitmap, 8, 8, iconColor);

  // Taste 5 (setzen)
  if (showKey5) {
    displayButtonAndText(setBtnX, setBtnY, "setzen", '5');
  }

  // Taste 2 (auf)
  displayButtonAndText(upBtnX, upBtnY, "", '2');
  drawBitmapScaled(upArrowX, upArrowY, upBitmap, 8, 8, iconColor);

  // Taste 8 (ab)
  displayButtonAndText(downBtnX, downBtnY, "", '8');
  drawBitmapScaled(downArrowX, downArrowY, downBitmap, 8, 8, iconColor);

  myDFPlayer.reset();
  delay(500);
  myDFPlayer.volume(loadVolumeFromLittleFS());
}

void setup() {

  Serial.begin(115200);

  pcf2.write(0, HIGH);  // Display einschalten

  delay(400);

  FPSerial.begin(9600);

  // SPI initialisieren
  SPI.begin();

  // Alle Akkuwerte auf Mittelwert setzen
  for (int i = 0; i < anzahlAkkuWerte; i++) {
    akkuWerte[i] = (akkuObererWert + akkuUntererWert) / 2;
  }

  // TFT-Display initialisieren
  initDisplay(smallDisplay);

  tft->fillScreen(bgColor);
  tft->setCursor(0, 0);
  tft->setTextColor(textColor, bgColor);
  tft->setTextSize(1);

  tft->print(F("Starte Version "));
  tft->println(VERSION);
  tft->println(F(""));

  // I2C starten
  Wire.begin(D3, D6);  // SDA = D3 (GPIO0), SCL = D6 (GPIO12)
  pcf1.begin();        // Ersten PCF8574 initialisieren
  pcf2.begin();        // Zweiten PCF8574 initialisieren

  // Grosses oder kleines Display? (Jumper PCF2-Pin1 --> GND)
  bool readSmallDisplay = pcf2.read(1);
  Serial.print(F("Small display: "));
  Serial.println(readSmallDisplay);
  if (smallDisplay != readSmallDisplay) {
    smallDisplay = readSmallDisplay;
    initDisplay(smallDisplay);
    tft->fillScreen(bgColor);
  }

  loadWifiFromLittleFS(ssid, passwd);
  if (!passwd.isEmpty()) {
    Serial.println(F("WLAN Daten;"));
    Serial.println(ssid);
    Serial.println(passwd);
    WiFi.begin(ssid, passwd);
    byte attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(1000);
      attempts++;
      Serial.print(F("Verbinde mit Wifi. Versuch "));
      Serial.println(attempts);
      tft->print(F("Verbinde Wifi. Versuch "));
      tft->println(attempts);
    }
    if (WiFi.status() != WL_CONNECTED) {
      tft->fillScreen(bgColor);
      tft->setCursor(0, 20);
      Serial.println(F("Wifi-Verbindung fehlgeschlagen!"));
      tft->println(F("Wifi fehlgeschlagen"));
      goto end;  // Springt ans Ende der Funktion
    }
    tft->fillScreen(bgColor);
    tft->setCursor(0, 20);
    tft->println(F("Wifi verbunden"));
    Serial.println(F("Wifi verbunden"));
    online = true;
    if (connectToFTP()) {
      infoFile = downloadFile("/info.txt");
    }
    Serial.println(F("Remote info.txt:"));
    Serial.println(F("----------------"));
    Serial.println(infoFile);
    Serial.println(F("----------------"));

    String versionLocal = loadVersionFromLittleFS();  // -1 = Fehler, -2 = File nicht vorhanden

    int newlineIndex2 = versionLocal.indexOf('\n');  // Erstes Zeilenende suchen
    if (versionLocal.startsWith("-1") || versionLocal.startsWith("-2")) {
      version = 0.00;  // Falls keine Zeile gefunden wurde, Standardwert zurückgeben
    } else {
      version = versionLocal.substring(1, newlineIndex2).toFloat();  // "V" entfernen und nach float wandeln
    }

    int newlineIndex = infoFile.indexOf('\n');  // Erstes Zeilenende suchen
    if (infoFile.startsWith("-1")) {
      newVersion = 0.00;  // Falls keine Zeile gefunden wurde, Standardwert zurückgeben
    } else {
      newVersion = infoFile.substring(1, newlineIndex).toFloat();  // "V" entfernen und nach float wandeln
    }

    // Debug-Ausgaben
    Serial.print(F("Erkannte lokale Version (LittleFS): "));
    Serial.println(version);

    Serial.print(F("Erkannte remote Version: "));
    Serial.println(newVersion);

    Serial.print(F("Erkannte remote Version formattiert: "));
    Serial.println(formatVersion(newVersion));

    float inoVersion = VERSION.substring(1).toFloat();
    Serial.print(F("Version im Sketch (kompiliert): "));
    Serial.println(inoVersion);

    if (inoVersion < newVersion) {  // Wenn online nicht der compilierten Version entspricht
      newVersionAvailable = true;
      Serial.println(F("Neue Version online verfügbar!"));
    }
    if (version != inoVersion) {  // Wenn LittleFS nicht der compilierten Version entspricht
      Serial.println(F("Version in LittleFS entspricht nicht der compilierten Version!"));
      Serial.println(F("Speichere online Version in LittleFS..."));
      saveVersionToLittleFS(infoFile);
    }
  }
end:

  Serial.println();
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  // DFPlayer Initialisierung mit Timeout (max. 8 Sekunden)
  bool dfPlayerOK = false;
  uint32_t startTime = millis();
  int attempt = 0;

  while (millis() - startTime < 8000) {  // max. 8 Sekunden warten
    attempt++;
    Serial.printf("DFPlayer Versuch %d...\n", attempt);
    tft->printf("DFPlayer Versuch %d...", attempt);
    tft->println(F(""));

    if (myDFPlayer.begin(FPSerial, true, true)) {  // isACK=true, doReset=true
      Serial.println(F("DFPlayer Mini online!"));
      tft->println(F("DFPlayer Mini online!"));
      dfPlayerOK = true;
      break;
    }

    Serial.println(F("Fehlgeschlagen – 1 Sekunde warten..."));
    delay(1000);
  }

  if (dfPlayerOK) {
    myDFPlayer.reset();
    delay(800);
    myDFPlayer.volume(loadVolumeFromLittleFS());  // Lautstärke setzen
    delay(200);
    myDFPlayer.setTimeOut(500);
    delay(200);
    myDFPlayer.EQ(DFPLAYER_EQ_POP);
  } else {
    Serial.println(F("DFPlayer gibt auf – fahre ohne Sound weiter!"));
    tft->println(F("Kein Sound – OK!"));
    delay(2000);
  }

  if (newVersionAvailable) {
    Serial.print(F("Neue Version verfügbar. Lokal: V"));
    Serial.print(version);
    Serial.print(F(", verfügbar: V"));
    Serial.println(newVersion);
    tft->fillScreen(bgColor);
    tft->setCursor(0, 0);
    tft->setTextColor(textColor);
    tft->println(F("Neue Version verfuegbar!"));
    tft->print(F("Installiert: "));
    tft->println(version);
    tft->print(F("Verfuegbar:  "));
    tft->println(newVersion);
    tft->println(F(""));
    displayInfoFile(infoFile, 9);
    //tft->println(infoFile);
    //tft->println(F("Download:"));
    //tft->println(F("ftp://espboy:esp8266@espboy.istmein.de:4321"));
    //tft->println(F(""));
    //tft->println(F("https://github.com/ltspicer/espboy"));
    tft->println(F(""));
    tft->println(F("Update Taste B druecken"));
    char key;
    while (true) {
      autoPowerOff();
      key = scanKeypad();

      if (key == 'B') {
        if (connectToFTP()) {
          tft->fillScreen(bgColor);
          tft->setCursor(0, 0);
          tft->println(F("Starte Update..."));
          String versionStr = "/ESPboy_V" + formatVersion(newVersion) + ".bin";
          Serial.print(F("Suche File: "));
          Serial.println(versionStr);
          performFTPUpdate(versionStr);
          client.stop();
        } else {
          tft->println(F("FTP-Verbindung fehlgeschlagen"));
        }
      }

      if (key != '\0') {
        break;
      }
    }
  }

  displayMenu();

  boolean menuActive = true;
  lastActivity = millis();

  while (menuActive) {
    autoPowerOff();
    displayBatteriestand();
    char pressedKey = scanKeypad();  // Tastaturabfrage
    if (pressedKey >= '1' && pressedKey <= '8') {
      lastActivity = millis();
      auswahl = pressedKey - '0';  // Zeichen in Zahl umwandeln
      switch (auswahl) {
        case 1:
          Serial.println(F("Reversi gewählt"));
          //menuActive = false;
          reversi();
          displayMenu();
          myDFPlayer.volume(loadVolumeFromLittleFS());
          delay(500);
          break;
        case 2:
          Serial.println(F("4 gewinnt gewählt"));
          //menuActive = false;
          vierGewinnt();
          displayMenu();
          myDFPlayer.volume(loadVolumeFromLittleFS());
          delay(500);
          break;
        case 3:
          Serial.println(F("Mini Mastermind gewählt"));
          //menuActive = false;
          karussell();
          displayMenu();
          myDFPlayer.volume(loadVolumeFromLittleFS());
          delay(500);
          break;
        case 4:
          Serial.println(F("Minesweeper gewählt"));
          //menuActive = false;
          minesweeper();
          displayMenu();
          myDFPlayer.volume(loadVolumeFromLittleFS());
          delay(500);
          break;
        case 5:
          Serial.println(F("Senso gewählt"));
          //menuActive = false;
          senso();
          displayMenu();
          myDFPlayer.volume(loadVolumeFromLittleFS());
          delay(500);
          break;
        case 6:
          Serial.println(F("Snake gewählt"));
          //menuActive = false;
          snake();
          displayMenu();
          myDFPlayer.volume(loadVolumeFromLittleFS());
          delay(500);
          break;
        case 7:
          break;
        case 8:
          Serial.println(F("Einstellungen gewählt"));
          settingsMenu();
          displayMenu();
          delay(500);
          break;
        default:
          Serial.println(F("Ungültige Eingabe"));
          displayMenu();
          break;
      }
    } else if (pressedKey == '*') {
      displayHiscores();
      displayMenu();
    }
  }
}

void loop() {
  delay(500);
}

// Einstellungs-Menu
void settingsMenu() {
  displaySettingsMenu();
  delay(500);
  while (true) {
    autoPowerOff();
    char pressedKey = scanKeypad();  // Tastaturabfrage
    if (pressedKey >= '1' && pressedKey <= '5') {
      lastActivity = millis();
      auswahl = pressedKey - '0';  // Zeichen in Zahl umwandeln
      switch (auswahl) {
        case 1:
          Serial.println(F("Minesweeper Minen gewählt"));
          readMinesNumberFromKeypad();
          displaySettingsMenu();
          break;
        case 2:
          Serial.println(F("Hiscore löschen gewählt"));
          deleteHiscores();
          displaySettingsMenu();
          break;
        case 3:
          Serial.println(F("Volume gewählt"));
          readVolumeNumberFromKeypad();
          displaySettingsMenu();
          break;
        case 4:
          Serial.println(F("WLAN gewählt"));
          configureWiFi();
          displaySettingsMenu();
          break;
        case 5:
          Serial.println(F("Changelog gewählt"));
          tft->fillScreen(bgColor);
          tft->setCursor(0, 0);
          displayInfoFile(infoFile, smallDisplay ? 16 : 30);
          delay(500);
          char key;
          while (true) {
            autoPowerOff();
            key = scanKeypad();
            if (key != '\0') {
              break;
            }
          }
          displaySettingsMenu();
          break;
      }
    } else if (pressedKey == 'A') {
      Serial.println(F("Abbruch gewählt"));
      return;
    }
  }
}

// Funktion für Auto-Power-Off
void autoPowerOff() {
  int batterieSpannung = addValue();  // Neuen Wert zur Warteschlange hinzufügen
  //Serial.print(F("Analog Eingang: "));
  //Serial.println(batterieSpannung);
  if (millis() - lastActivity >= SLEEP_TIME) {
    Serial.println(F("Keine Aktivität – Gehe in den Deep Sleep-Modus..."));
    myDFPlayer.sleep();
    delay(1000);
    pcf2.write(0, LOW);  // Display ausschalten
    delay(1000);
    ESP.deepSleep(0);  // Unendlich schlafen
  }
}

// Fügt einen neuen Wert hinzu und entfernt den ältesten
int addValue() {
  int newValue = analogRead(A0);  // Neuen Sensorwert einlesen
  if (newValue < akkuUntererWert) {
    newValue = akkuUntererWert;
  }
  if (newValue > akkuObererWert) {
    newValue = akkuObererWert;
  }
  // Alle Werte nach links verschieben (ältester Wert fällt raus)
  for (int i = 0; i < anzahlAkkuWerte - 1; i++) {
    akkuWerte[i] = akkuWerte[i + 1];
  }
  akkuWerte[anzahlAkkuWerte - 1] = newValue;  // Neuen Wert an letzter Position speichern
  return newValue;
}

// Berechnet den Durchschnitt der gespeicherten Werte
int berechneDurchschnitt() {
  int sum = 0;
  for (int i = 0; i < anzahlAkkuWerte; i++) {
    sum += akkuWerte[i];
  }

  int durchschnitt = sum / anzahlAkkuWerte;

  // Negative Werte verhindern
  if (durchschnitt < 0) return 0;

  return durchschnitt;
}

int berechneProzentAkku() {
  static int letzterProzentWert = -1;
  static int untereSchwelle = -1;
  static int obereSchwelle = -1;

  int akkuDurchschnittsWert = berechneDurchschnitt();
  const int skala = akkuObererWert - akkuUntererWert;
  int analogWertBasisNull = akkuDurchschnittsWert - akkuUntererWert;
  int prozentWert = (analogWertBasisNull * 100) / skala;

  // Beim ersten Durchlauf Initialisierung
  if (letzterProzentWert == -1) {
    letzterProzentWert = (prozentWert / 5) * 5;  // Initial auf 5er-Wert setzen
    untereSchwelle = letzterProzentWert - 5;     // Hysterese nach unten
    obereSchwelle = letzterProzentWert + 5;      // Hysterese nach oben
  }

  // **Hysterese: Wechsel nur bei ±5 % Differenz**
  if (prozentWert >= obereSchwelle) {
    letzterProzentWert = ((prozentWert + 2) / 5) * 5;  // Neu runden
    untereSchwelle = letzterProzentWert - 5;           // Neue untere Grenze
    obereSchwelle = letzterProzentWert + 5;            // Neue obere Grenze
  } else if (prozentWert <= untereSchwelle) {
    letzterProzentWert = ((prozentWert + 2) / 5) * 5;
    untereSchwelle = letzterProzentWert - 5;
    obereSchwelle = letzterProzentWert + 5;
  }
  return letzterProzentWert;
}

//////////////////////////////////
// Funktionen für Tastaturabfragen
//////////////////////////////////

// Funktion zur Tastatur-Abfrage mit Zeichenfilterung
char scanKeypad() {
  for (byte row = 0; row < 4; row++) {
    // Setze die aktuelle Reihe (P0 bis P3) auf LOW, die anderen auf HIGH
    for (byte pin = 0; pin < 4; pin++) {
      pcf1.write(pin, HIGH);
    }
    pcf1.write(row, LOW);

    // Lese die Spalten (P4 bis P7) als Eingänge vom ersten PCF8574
    byte inputData = 0;
    for (byte col = 4; col < 8; col++) {
      inputData |= pcf1.read(col) << (col - 4);  // Lese die Spalten P4 bis P7
    }

    // Überprüfe, ob einer der Pins auf LOW gezogen wurde
    for (byte col = 0; col < 4; col++) {
      if (!(inputData & (1 << col))) {  // Wenn der Pin auf LOW gezogen wurde
        lastActivity = millis();        // Autopower-off verzögern
        char key = keys[row][col];      // Gelesene Taste

        // Prüfen, ob das Zeichen erlaubt ist
        for (byte i = 0; i < sizeof(allowedChars) - 1; i++) {
          if (key == allowedChars[i]) {
            return key;  // Nur gültige Zeichen zurückgeben
          }
        }
      }
    }
    delay(5);  // Kurze Verzögerung, bevor zur nächsten Reihe gewechselt wird
  }
  return 0;  // Keine Taste gedrückt oder ungültige Taste
}

// Lautstärke Eingabe
void readVolumeNumberFromKeypad() {
  byte value = loadVolumeFromLittleFS();
  displayVolume(value);
  char key;
  while (true) {
    key = scanKeypad();
    autoPowerOff();
    if (key == 'A') {
      saveVolumeToLittleFS(value);  // Speichern, wenn Eingabe abgeschlossen
      break;
    }
    if (key == '*') {
      value = max(0, value - 1);  // Decrement, aber nicht unter 0
      displayVolume(value);
      myDFPlayer.volume(value);
    } else if (key == 'D') {
      value = min(30, value + 1);  // Increment, aber nicht über 30
      displayVolume(value);
      myDFPlayer.volume(value);
    }
    delay(200);  // Entprellung
  }
}

// Minesweeper Minen eingeben
void readMinesNumberFromKeypad() {
  int value = loadMinesFromLittleFS();
  displayMines(value);
  char key;
  while (true) {
    key = scanKeypad();  // Taste einlesen
    autoPowerOff();      // Power-Off prüfen

    if (key == 'A') {
      if (value > 0 && value < 80) {
        saveMinesToLittleFS(value);  // Speichern, wenn Eingabe abgeschlossen
        break;
      } else {
        Serial.println(F("Nur Werte von 1 bis 79 erlaubt!"));
      }
    } else if (key >= '0' && key <= '9') {
      Serial.print(F("Eingabe: "));
      Serial.println(key);
      value = value * 10;           // Verschiebe nach links und füge Ziffer rechts hinzu
      value = value % 100;          // Hunderter Stelle abschneiden
      value = value + (key - '0');  // Aktuelle Ziffer hinzufügen
      displayMines(value);
    }
    delay(200);  // Entprellung
  }
}

// Wifi Eingabe
String wlanKeypadInput(String& ssid, String& passwd) {
  delay(1000);  // Entprellpause
  currentIndex = 0;
  String input = ssid;
  bool ssidInput = true;  // Flag, um zu bestimmen, ob SSID oder Passwort eingegeben wird
  char key = '\0';        // Verwenden von '\0' als Platzhalter für keine Taste

  Serial.println(F("Enter SSID:"));
  displayWifi(ssid, passwd, input + "_", ssidInput ? 1 : 2);

  while (true) {
    key = scanKeypad();  // Warten auf Tasteneingabe
    autoPowerOff();

    if (key != '\0') {   // Wenn eine Taste gedrückt wurde
      if (key == '2') {  // Rauf (Zeichen in der Liste nach oben bewegen)
        currentIndex++;
        if (currentIndex >= characters.length()) {
          currentIndex = 0;  // Zurück zum Anfang, wenn das Ende erreicht wurde
        }
        displayWifi(ssid, passwd, input + characters[currentIndex], ssidInput ? 1 : 2);
        Serial.print(F("\rCurrent input: "));
        Serial.print(characters[currentIndex]);  // Zeige das aktuelle Zeichen
        delay(200);                              // Entprellpause
      } else if (key == '8') {                   // Runter (Zeichen in der Liste nach unten bewegen)
        currentIndex--;
        if (currentIndex < 0) {
          currentIndex = characters.length() - 1;  // Zurück zum Ende, wenn wir am Anfang sind
        }
        displayWifi(ssid, passwd, input + characters[currentIndex], ssidInput ? 1 : 2);
        Serial.print(F("\rCurrent input: "));
        Serial.print(characters[currentIndex]);  // Zeige das aktuelle Zeichen
        delay(200);                              // Entprellpause
      } else if (key == 'D') {                   // Wifi deaktivieren (PW löschen)
        passwd = "";                             // Passwort löschen
        displayWifi(ssid, passwd, "Wifi inaktiv! Reboot...", 0);
        delay(3000);
        return "Done";          // Eingabe abgeschlossen
      } else if (key == '4') {  // Backspace (Löschen des letzten Zeichens)
        if (input.length() > 0) {
          input.remove(input.length() - 1);  // Lösche das letzte Zeichen
          Serial.print(F("\rInput: "));
          Serial.print(input);  // Zeige das Eingabefeld
          displayWifi(ssid, passwd, input + "_", ssidInput ? 1 : 2);
          delay(400);  // Entprellpause
        }
      } else if (key == '*') {  // Enter (Bestätigen der Eingabe)
        if (ssidInput) {
          if (!input.isEmpty()) {
            ssid = input;       // SSID speichern
            ssidInput = false;  // Zum Passwort wechseln
            input = passwd;     // Eingabefeld zurücksetzen
            displayWifi(ssid, passwd, input + "_", ssidInput ? 1 : 2);
            Serial.println(F("\nEnter Password:"));
            delay(400);  // Entprellpause
          } else {
            Serial.println(F("Error: SSID cannot be empty."));
            displayWifi(ssid, passwd, "SSID cannot be empty!", 0);
            delay(3000);  // Entprellpause
            input = ssid;
            displayWifi(ssid, passwd, input + "_", 1);
          }
        } else {
          if (!input.isEmpty()) {
            passwd = input;  // Passwort speichern
            displayWifi(ssid, passwd, "Wifi aktiviert! Reboot...", 0);
            delay(3000);
            return "Done";  // Eingabe abgeschlossen
          } else {
            Serial.println(F("Error: Password cannot be empty."));
            displayWifi(ssid, passwd, "Password cannot be empty!", 0);
            delay(3000);  // Entprellpause
            input = passwd;
            displayWifi(ssid, passwd, input + "_", 2);
          }
        }
      } else if (key == '6') {              // Fügt das Zeichen zur Eingabe hinzu
        input += characters[currentIndex];  // Füge das aktuell ausgewählte Zeichen zur Eingabe hinzu
        Serial.print(F("\rInput: "));
        Serial.print(input);  // Zeige das Eingabefeld
        displayWifi(ssid, passwd, input + "_", ssidInput ? 1 : 2);
        delay(400);             // Entprellpause
      } else if (key == 'A') {  // Abbruch (Taste A)
        Serial.println(F("\nAbbruch. Keine Eingaben gespeichert."));
        return "Cancelled";  // Vorgang abbrechen
      }
      Serial.println("");
    }
  }
}

// Namen Eingabe
String nameKeypadInput() {
  delay(1000);  // Entprellpause
  String input = "";
  currentIndex2 = 0;
  char key = '\0';  // Verwenden von '\0' als Platzhalter für keine Taste

  Serial.println("Enter name");
  displayName(input + "_", true);

  while (true) {
    key = scanKeypad();  // Warten auf Tasteneingabe
    autoPowerOff();

    if (key != '\0') {   // Wenn eine Taste gedrückt wurde
      if (key == '2') {  // Rauf (Zeichen in der Liste nach oben bewegen)
        currentIndex2++;
        if (currentIndex2 >= characters2.length()) {
          currentIndex2 = 0;  // Zurück zum Anfang, wenn das Ende erreicht wurde
        }
        displayName(input + characters2[currentIndex2], false);
        delay(200);             // Entprellpause
      } else if (key == '8') {  // Runter (Zeichen in der Liste nach unten bewegen)
        currentIndex2--;
        if (currentIndex2 < 0) {
          currentIndex2 = characters2.length() - 1;  // Zurück zum Ende, wenn wir am Anfang sind
        }
        displayName(input + characters2[currentIndex2], false);
        delay(200);             // Entprellpause
      } else if (key == '4') {  // Backspace (Löschen des letzten Zeichens)
        if (input.length() > 0) {
          input.remove(input.length() - 1);  // Lösche das letzte Zeichen
          displayName(input + "_ ", false);
          delay(400);  // Entprellpause
        }
      } else if (key == '*') {  // Eingabe fertig
        if (!input.isEmpty()) {
          return input;
        } else {
          input = "???";
          return input;
        }
      } else if (key == '6') {                // Fügt das Zeichen zur Eingabe hinzu
        input += characters2[currentIndex2];  // Füge das aktuell ausgewählte Zeichen zur Eingabe hinzu
        displayName(input + "_", false);
        delay(400);  // Entprellpause
      }
    }
  }
}

//////////////////////////
// Funktionen für DFPLayer
//////////////////////////

// Debug Infos von DFPlayer
void printDetail(byte type, byte value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println(F("USB Inserted!"));
      break;
    case DFPlayerUSBRemoved:
      Serial.println(F("USB Removed!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

// Spiele Track (DFPlayer)
void playTrack(byte trackNumber) {
  if (trackNumber >= 1 && trackNumber <= 7) {
    myDFPlayer.play(trackNumber);
    Serial.print(F("Spiele Track: "));
    Serial.println(trackNumber);
  }
}

// Track fertig? (DFPlayer)
bool isTrackFinished() {
  if (myDFPlayer.available()) {
    byte type = myDFPlayer.readType();
    byte value = myDFPlayer.read();
    if (type == DFPlayerPlayFinished) {
      Serial.print(F("Track "));
      Serial.print(value);
      Serial.println(F(" beendet."));
      return true;
    }
  }
  return false;
}

/////////////////////////
// Funktionen für Display
/////////////////////////

// Batteriestand anzeigen
void displayBatteriestand() {
  tft->setCursor(smallDisplay ? 88 : 176, 1);
  tft->setTextSize(smallDisplay ? 1 : 2);

  byte prozent = berechneProzentAkku();
  if (prozent <= 10) {
    tft->setTextColor(ST77XX_RED, bgColor);
  } else {
    tft->setTextColor(textColor, bgColor);
  }
  tft->print(F("Akku: "));
  tft->print(prozent);
  tft->print(F("  "));
  tft->setCursor(smallDisplay ? 145 : 290, 1);
  tft->print(F("%"));
  tft->setTextColor(textColor, bgColor);
  //  tft->setCursor(88, 10);
  //  tft->print(berechneDurchschnitt());
}

// Menu Display initialisieren
void menuDisplayInit() {
  bgColor = ST77XX_BLACK;
  iconColor = ST77XX_WHITE;
  textColor = ST77XX_WHITE;
}

// Spielfeld initialisieren
void initSpielfeld() {
  // Speicher reservieren
  spielfeld = (byte**)malloc(SPALTEN * sizeof(byte*));
  for (int i = 0; i < SPALTEN; i++) {
    spielfeld[i] = (byte*)malloc(ZEILEN * sizeof(byte));
  }

  // Spielfeld mit 0 initialisieren
  for (int i = 0; i < SPALTEN; i++) {
    for (int j = 0; j < ZEILEN; j++) {
      spielfeld[i][j] = 0;
    }
  }
}

// Icon + Text auf Display
void displayButtonAndText(uint16_t iconX, byte iconY, String text, char taste) {
  if (taste == '0') {
    drawBitmapScaled(iconX, iconY - 1, button0Bitmap, 10, 10, iconColor);
  }
  if (taste == '1') {
    drawBitmapScaled(iconX, iconY - 1, button1Bitmap, 10, 10, iconColor);
  }
  if (taste == '2') {
    drawBitmapScaled(iconX, iconY - 1, button2Bitmap, 10, 10, iconColor);
  }
  if (taste == '3') {
    drawBitmapScaled(iconX, iconY - 1, button3Bitmap, 10, 10, iconColor);
  }
  if (taste == '4') {
    drawBitmapScaled(iconX, iconY - 1, button4Bitmap, 10, 10, iconColor);
  }
  if (taste == '5') {
    drawBitmapScaled(iconX, iconY - 1, button5Bitmap, 10, 10, iconColor);
  }
  if (taste == '6') {
    drawBitmapScaled(iconX, iconY - 1, button6Bitmap, 10, 10, iconColor);
  }
  if (taste == '7') {
    drawBitmapScaled(iconX, iconY - 1, button7Bitmap, 10, 10, iconColor);
  }
  if (taste == '8') {
    drawBitmapScaled(iconX, iconY - 1, button8Bitmap, 10, 10, iconColor);
  }
  if (taste == '9') {
    drawBitmapScaled(iconX, iconY - 1, button9Bitmap, 10, 10, iconColor);
  }
  if (taste == 'A') {
    drawBitmapScaled(iconX, iconY - 1, buttonABitmap, 10, 10, iconColor);
  }
  if (taste == 'B') {
    drawBitmapScaled(iconX, iconY - 1, buttonBBitmap, 10, 10, iconColor);
  }
  if (taste == 'C') {
    drawBitmapScaled(iconX, iconY - 1, buttonCBitmap, 10, 10, iconColor);
  }
  if (taste == 'D') {
    drawBitmapScaled(iconX, iconY - 1, buttonDBitmap, 10, 10, iconColor);
  }
  if (taste == '*') {
    drawBitmapScaled(iconX, iconY - 1, buttonStarBitmap, 10, 10, iconColor);
  }
  if (taste == '#') {
    drawBitmapScaled(iconX, iconY - 1, buttonHashBitmap, 10, 10, iconColor);
  }

  int16_t x1, y1;
  uint16_t textWidth, textHeight;

  tft->setTextSize(scale);

  if (text.length() > 0) {
    // Textgröße berechnen und Hintergrund exakt über den Text zeichnen
    tft->getTextBounds(
      text,
      iconX + (smallDisplay ? 20 : 40),
      iconY + (smallDisplay ? 1 : 2),
      &x1, &y1, &textWidth, &textHeight);

    tft->fillRect(x1, y1, textWidth, textHeight, bgColor);
  }

  tft->setCursor(
    iconX + (smallDisplay ? 20 : 40),
    iconY + (smallDisplay ? 1 : 2));

  tft->println(text);
}

// Volume Eingabe auf Display
void displayVolume(byte volume) {
  tft->fillScreen(bgColor);
  tft->setCursor(1, 1);
  tft->setTextColor(textColor);
  tft->setTextSize(smallDisplay ? 2 : 4);
  tft->println("Volume: " + String(volume));
  tft->setTextSize(smallDisplay ? 1 : 2);

  displayButtonAndText(
    8,
    smallDisplay ? 32 : 42,
    "-",
    '*');

  displayButtonAndText(
    smallDisplay ? 88 : 110,
    smallDisplay ? 32 : 42,
    "+",
    'D');

  displayButtonAndText(
    8,
    smallDisplay ? 56 : 66,
    "Speichern/Save",
    'A');
}

// Minen Eingabe auf Display
void displayMines(int mines) {
  tft->fillScreen(bgColor);
  tft->setCursor(1, 1);
  tft->setTextColor(textColor);
  tft->setTextSize(smallDisplay ? 2 : 4);
  tft->print(F("Minen: "));
  tft->printf("%02d\n", mines);  // Formatierung mit führenden Nullen

  tft->setTextSize(smallDisplay ? 1 : 2);
  tft->setCursor(4, smallDisplay ? 32 : 42);

  tft->print(F("Nur 1 - 79 Minen erlaubt!"));
  displayButtonAndText(4, smallDisplay ? 56 : 76, "Speichern/Save", 'A');
}

// Wifi Eingabe auf Display
void displayWifi(String ssid, String passwd, String input, byte eingabeTyp) {

  tft->fillRect(0, 0,
                smallDisplay ? 159 : 319,
                smallDisplay ? 27 : 54,
                bgColor);

  tft->setCursor(0, 1);
  tft->setTextColor(textColor);
  tft->setTextSize(smallDisplay ? 1 : 2);

  //tft->println("SSID: " + ssid);
  //tft->println("PASS: " + passwd);

  tft->setCursor(0, smallDisplay ? 25 : 17);

  if (eingabeTyp == 1) {
    tft->print(F("SSID "));
  }
  if (eingabeTyp == 2) {
    tft->print(F("Passwort "));
  }
  if (eingabeTyp != 0) {
    tft->println(F("Eingabe:"));
    tft->println(input);
  } else {
    tft->println(input);
  }
  //tft->println(F(""));
}

void displayWifiMenu() {
  tft->fillScreen(bgColor);

  // Taste 2 (Zeichen+)
  displayButtonAndText(
    smallDisplay ? 60 : 80,
    smallDisplay ? 47 : 57,
    "Zeichen+",
    '2');

  // Taste 4 (links)
  displayButtonAndText(
    smallDisplay ? 28 : 18,
    smallDisplay ? 59 : 79,
    "",
    '4');

  // Pfeil-Icon
  drawBitmapScaled(
    42,
    smallDisplay ? 60 : 80,
    backspaceBitmap, 8, 8, iconColor);

  // Taste 6 (rechts)
  displayButtonAndText(
    smallDisplay ? 88 : 142,
    smallDisplay ? 59 : 79,
    "",
    '6');

  // RETURN-Icon
  drawBitmapScaled(
    smallDisplay ? 102 : 166,
    smallDisplay ? 60 : 80,
    returnBitmap, 8, 8, iconColor);

  // Taste 8 (Zeichen-)
  displayButtonAndText(
    smallDisplay ? 60 : 80,
    smallDisplay ? 73 : 103,
    "Zeichen-",
    '8');

  // Weiter / Abbruch / Wifi deaktivieren
  displayButtonAndText(0, smallDisplay ? 89 : 129, "Weiter/Next", '*');
  displayButtonAndText(0, smallDisplay ? 100 : 150, "Abbruch/Cancel", 'A');
  displayButtonAndText(0, smallDisplay ? 111 : 181, "Wifi deaktivieren", 'D');
}

// Namen Eingabe auf Display
void displayName(String name, bool init) {
  if (init) {
    tft->fillScreen(bgColor);

    tft->setCursor(1, 10);
    tft->setTextColor(textColor);
    tft->setTextSize(smallDisplay ? 1 : 2);
    tft->print(F("Name: "));
    tft->print(name);

    // Taste 2 (Zeichen+)
    displayButtonAndText(
      smallDisplay ? 60 : 80,
      smallDisplay ? 47 : 57,
      "Zeichen+",
      '2');

    // Taste 4 (links)
    displayButtonAndText(
      smallDisplay ? 28 : 18,
      smallDisplay ? 59 : 79,
      "",
      '4');

    // Pfeil-Icon
    drawBitmapScaled(
      42,
      smallDisplay ? 60 : 80,
      backspaceBitmap, 8, 8, iconColor);

    // Taste 6 (rechts)
    displayButtonAndText(
      smallDisplay ? 88 : 142,
      smallDisplay ? 59 : 79,
      "",
      '6');

    // RETURN-Icon
    drawBitmapScaled(
      smallDisplay ? 102 : 166,
      smallDisplay ? 60 : 80,
      returnBitmap, 8, 8, iconColor);

    // Taste 8 (Zeichen-)
    displayButtonAndText(
      smallDisplay ? 60 : 80,
      smallDisplay ? 73 : 103,
      "Zeichen-",
      '8');

    displayButtonAndText(1, smallDisplay ? 89 : 129, "Fertig", '*');
  } else {
    tft->setCursor(1, 10);
    tft->setTextColor(textColor, bgColor);
    tft->setTextSize(smallDisplay ? 1 : 2);
    tft->print(F("Name: "));
    tft->print(name);
  }
}

// Menu auf Display
void displayMenu() {
  myDFPlayer.loop(8);  //Menu Hintergrundmusik
  menuDisplayInit();
  tft->fillScreen(bgColor);
  tft->setCursor(1, 1);
  tft->setTextColor(textColor);

  tft->setTextSize(smallDisplay ? 2 : 4);
  tft->println(F("MENU"));
  tft->setTextSize(smallDisplay ? 1 : 2);
  tft->setCursor(smallDisplay ? 88 : 176,
                 smallDisplay ? 9 : 18);

  if (online) {
    tft->setTextColor(ST77XX_GREEN);
    tft->println(F("online"));
  } else {
    tft->setTextColor(ST77XX_RED);
    tft->println(F("offline"));
  }

  tft->setTextColor(textColor);
  displayButtonAndText(1, smallDisplay ? 28 : 46, "Reversi", '1');
  displayButtonAndText(1, smallDisplay ? 39 : 68, "4 gewinnt", '2');
  displayButtonAndText(1, smallDisplay ? 50 : 90, "Mini Mastermind", '3');
  displayButtonAndText(1, smallDisplay ? 61 : 112, "Minesweeper", '4');
  displayButtonAndText(1, smallDisplay ? 72 : 134, "Senso", '5');
  displayButtonAndText(1, smallDisplay ? 83 : 156, "Snake", '6');
  displayButtonAndText(1, smallDisplay ? 106 : 184, "Hiscores", '*');
  displayButtonAndText(1, smallDisplay ? 117 : 206, "Einstellungen", '8');
}

// Einstellungsmenu auf Display
void displaySettingsMenu() {
  tft->fillScreen(bgColor);
  tft->setCursor(1, 1);
  tft->setTextColor(textColor);
  tft->setTextSize(smallDisplay ? 2 : 4);
  tft->println(F("Einstellungen"));
  tft->setTextSize(smallDisplay ? 1 : 2);

  displayButtonAndText(1, smallDisplay ? 44 : 88, "Minesweeper Minen", '1');
  displayButtonAndText(1, smallDisplay ? 55 : 110, "Hiscores loeschen", '2');
  displayButtonAndText(1, smallDisplay ? 66 : 132, "Volume", '3');
  displayButtonAndText(1, smallDisplay ? 77 : 154, "WLAN", '4');
  displayButtonAndText(1, smallDisplay ? 88 : 176, "Changelog", '5');
  displayButtonAndText(1, smallDisplay ? 99 : 198, "Abbruch", 'A');
}

// Hi-Scores aus LittleFS lesen und auf Display ausgeben
void displayHiscores() {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS Mount Failed"));
    return;
  }

  Serial.println(F("=== Hi-Scores ==="));
  tft->fillScreen(bgColor);
  tft->setCursor(0, 1);
  tft->setTextColor(textColor);
  tft->setTextSize(smallDisplay ? 1 : 2);
  tft->println(F("======= Hi-Scores ======="));
  tft->println(F(""));

  // Liste aller Dateien abrufen
  File dir = LittleFS.open("/", "r");
  File file = dir.openNextFile();

  while (file) {
    String fileName = file.name();

    // Prüfen, ob der Dateiname NICHT mit ".txt" endet
    if (!fileName.endsWith(".txt")) {
      Serial.print(F("Game: "));
      Serial.println(fileName);
      //tft->print(F("Game: "));
      tft->print(fileName);
      tft->println(F(":"));

      // Dateiinhalt lesen
      String data = file.readStringUntil('\n');
      // Daten parsen (angenommen: "Spieler;10;5")
      int firstSep = data.indexOf(';');
      int secondSep = data.lastIndexOf(';');

      if (firstSep != -1 && secondSep != -1 && firstSep != secondSep) {
        String winnerName = data.substring(0, firstSep);
        int winnerScore = data.substring(firstSep + 1, secondSep).toInt();
        int looserScore = data.substring(secondSep + 1).toInt();

        // Ausgabe im gewünschten Format
        Serial.print(F("Winner: "));
        Serial.print(winnerName);
        Serial.print(F(" Score "));
        Serial.print(winnerScore);
        Serial.print(F(" (Looser score "));
        Serial.print(looserScore);
        Serial.println(F(")"));
        tft->print(winnerName);
        tft->print(F(" Score "));
        tft->println(winnerScore);
        if (looserScore > 0) {
          tft->print(F("(Looser Score "));
          tft->print(looserScore);
          tft->println(F(")"));
        }
        tft->println(F(""));
      }
    }
    file = dir.openNextFile();
  }
  Serial.println(F("================="));
  delay(1000);
  char key;
  while (true) {
    autoPowerOff();
    key = scanKeypad();
    if (key != '\0') {
      break;
    }
  }
}

//////////////////////////
// Funktionen für LittleFS
//////////////////////////

// Hi-Scores löschen
void deleteHiscores() {
  File root = LittleFS.open("/", "r");
  File file = root.openNextFile();
  tft->fillScreen(bgColor);
  displayButtonAndText(1, 40, "Hiscores loeschen", 'B');
  displayButtonAndText(1, smallDisplay ? 52 : 62, "Abbruch", 'A');
  while (true) {
    autoPowerOff();
    char pressedKey = scanKeypad();  // 1. Bestätigung
    if (pressedKey == 'B') {
      tft->fillScreen(bgColor);
      Serial.println(F("1. Bestätigung ok"));
      displayButtonAndText(1, 40, "Wirklich loeschen", 'C');
      displayButtonAndText(1, smallDisplay ? 52 : 62, "Abbruch", 'A');
      while (true) {
        autoPowerOff();
        char pressedKey = scanKeypad();  // 2. Bestätigung
        if (pressedKey == 'C') {
          Serial.println(F("Hiscores gelöscht"));
          while (file) {
            String fileName = file.name();
            if (!fileName.endsWith(".txt")) {
              Serial.print("Deleting: ");
              Serial.println(fileName);
              if (LittleFS.remove(fileName)) {
                Serial.println("Deleted successfully.");
              } else {
                Serial.println("Failed to delete.");
              }
            }
            file = root.openNextFile();
          }
          return;
        }
        if (pressedKey == 'A') {
          return;
        }
      }
    }
    if (pressedKey == 'A') {
      return;
    }
  }
}

// Lautstärke in LittleFS speichern
void saveVolumeToLittleFS(byte number) {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS Mount Failed"));
    return;
  }
  File file = LittleFS.open("/volume.txt", "w");
  if (!file) {
    Serial.println(F("File open failed"));
    return;
  }
  file.print(number);
  file.close();
  Serial.println("Volume saved: " + String(number));
}

// Minen in LittleFS speichern
void saveMinesToLittleFS(byte number) {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS Mount Failed"));
    return;
  }
  File file = LittleFS.open("/mines.txt", "w");
  if (!file) {
    Serial.println(F("File open failed"));
    return;
  }
  file.print(number);
  file.close();
  Serial.println("Mines saved: " + String(number));
}

// Lautstärke aus LittleFS laden
byte loadVolumeFromLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS Mount Failed"));
    return 20;  // Standardwert, falls Mount fehlschlägt
  }
  if (!LittleFS.exists("/volume.txt")) {
    Serial.println(F("File not found, using default value"));
    return 20;  // Standardwert, falls Datei nicht existiert
  }
  File file = LittleFS.open("/volume.txt", "r");
  if (!file) {
    Serial.println(F("File open failed"));
    return 20;
  }
  String valueStr = file.readString();
  file.close();
  return valueStr.toInt();
}

// Anzahl Minen aus LittleFS laden
byte loadMinesFromLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS Mount Failed"));
    return 10;  // Standardwert, falls Mount fehlschlägt
  }
  if (!LittleFS.exists("/mines.txt")) {
    Serial.println(F("File not found, using default value"));
    return 10;  // Standardwert, falls Datei nicht existiert
  }
  File file = LittleFS.open("/mines.txt", "r");
  if (!file) {
    Serial.println(F("File open failed"));
    return 10;
  }
  String valueStr = file.readString();
  file.close();
  return valueStr.toInt();
}

// WiFi-Daten aus LittleFS laden
void loadWifiFromLittleFS(String& ssid, String& passwd) {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS Mount Failed"));
    return;
  }
  if (!LittleFS.exists("/wifi.txt")) {
    Serial.println(F("File not found"));
    return;
  }
  File file = LittleFS.open("/wifi.txt", "r");
  if (!file) {
    Serial.println(F("Wifi file open failed"));
    return;
  }

  String valueStr = file.readString();
  file.close();

  // SSID und Passwort trennen
  int newlineIndex = valueStr.indexOf('\n');
  if (newlineIndex == -1) {
    Serial.println(F("Invalid file format"));
    return;
  }

  ssid = valueStr.substring(0, newlineIndex);
  passwd = valueStr.substring(newlineIndex + 1);

  // Entferne \r und \n aus den Strings
  ssid = entferneUnerwuenschteZeichen(ssid);
  passwd = entferneUnerwuenschteZeichen(passwd);
}

// Version aus LittleFS laden
String loadVersionFromLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS Mount Failed"));
    return "-1";
  }
  if (!LittleFS.exists("/info.txt")) {
    Serial.println(F("Info Datei nicht vorhanden. Wird angelegt (LittleFS)"));

    // Speichern der Versions-Daten im LittleFS
    File file = LittleFS.open("/info.txt", "w");
    if (!file) {
      Serial.println(F("Info file open failed"));
      return "-2";
    }

    // Schreibe standard info Datei
    file.println(VERSION);
    file.println("- Reversi");
    file.println("- 4 gewinnt");
    file.println("- Mini Mastermind");
    file.println("- Minesweeper");
    file.println("- Senso");
    file.println("- Snake");
    file.close();

    Serial.println(F("Standard info.txt Datei gespeichert."));
  }
  File file = LittleFS.open("/info.txt", "r");
  if (!file) {
    Serial.println(F("Fehler beim lesen der Info Datei (LittleFS)"));
  }
  String info = file.readString();
  file.close();

  Serial.println(F("----------------"));
  Serial.println(F("Lokale info.txt:"));
  Serial.println(info);
  Serial.println(F("----------------"));

  return info;
}

// Version in LittleFS speichern
bool saveVersionToLittleFS(String info) {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS Mount Failed"));
    return false;
  }
  // Speichern der Versions-Daten im LittleFS
  File file = LittleFS.open("/info.txt", "w");

  // Schreibe standard info Datei
  Serial.println(F("Schreibe info.txt neu"));
  file.println(info);
  file.close();
  return true;
}

// Entferne \r und \n aus String
String entferneUnerwuenschteZeichen(String string) {
  Serial.println("String vor Bereinigung: " + string);
  string.trim();
  string.replace("\r", "");
  string.replace("\n", "");
  Serial.println("String nach Bereinigung: " + string);
  return string;
}

// Hi-Score aus LittleFS laden
Hiscore loadHiscoreFromLittleFS(const String& gameName) {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS Mount Failed"));
    return {};  // Standardwert zurückgeben
  }

  String filePath = "/" + gameName;
  File file = LittleFS.open(filePath, "r");
  if (!file) {
    Serial.println(F("File not found or open failed"));
    return {};  // Standardwert zurückgeben
  }

  String data = file.readStringUntil('\n');
  file.close();

  // Daten parsen (Format: "Spieler;5;3")
  int firstSep = data.indexOf(';');
  int secondSep = data.lastIndexOf(';');

  if (firstSep == -1 || secondSep == -1 || firstSep == secondSep) {
    Serial.println(F("Invalid file format"));
    return {};  // Standardwert zurückgeben
  }

  return {
    data.substring(0, firstSep),
    data.substring(firstSep + 1, secondSep).toInt(),
    data.substring(secondSep + 1).toInt()
  };
}

// Hi-Score in LittleFS speichern
void saveHiscoreToLittleFS(const String& gameName, const String& name, int winner, int looser) {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS Mount Failed"));
    return;
  }

  String filePath = "/" + gameName;
  File file = LittleFS.open(filePath, "w");
  if (!file) {
    Serial.println(F("File open failed"));
    return;
  }

  // Hi-Score als CSV-Format speichern: "Name;Winner;Looser"
  file.print(name);
  file.print(";");
  file.print(winner);
  file.print(";");
  file.println(looser);

  file.close();
  Serial.println(F("Hi-Score gespeichert!"));
  Serial.print("Name: ");
  Serial.print(name);
  Serial.print(", Winner: ");
  Serial.print(winner);
  Serial.print(", Looser: ");
  Serial.println(looser);
}

// Hi-Score verarbeitung
bool hiScore(const String& gameName, int scoreWinner, int scoreLooser, String winnerName) {
  Hiscore daten = loadHiscoreFromLittleFS(gameName);
  if (daten.name.isEmpty() || scoreWinner > daten.winner) {
    if (winnerName.isEmpty()) {
      winnerName = nameKeypadInput();
    }
    saveHiscoreToLittleFS(gameName, winnerName, scoreWinner, scoreLooser);
    return true;
  }
  return false;
}

//////////////////////////////
// Funktionen für Wifi und FTP
//////////////////////////////

// Zu FTP verbinden
bool connectToFTP() {
  Serial.println("Verbinde mit FTP...");
  if (!client.connect(ftp_server, ftp_port)) {
    Serial.println("Verbindung fehlgeschlagen");
    return false;
  }
  waitForResponse();

  sendFTPCommand("USER " + String(ftp_user));
  sendFTPCommand("PASS " + String(ftp_pass));
  sendFTPCommand("TYPE I");  // Binärmodus setzen

  return true;
}

// Download einer Datei
String downloadFile(String filename) {
  sendFTPCommand("PASV");                   // Passiven Modus aktivieren
  String pasvResponse = waitForResponse();  // Antwort auf den PASV-Befehl abwarten

  // Parsen des Datenports aus der PASV-Antwort
  int dataPort = parsePasvResponse(pasvResponse);

  if (dataPort == -1) {
    Serial.println(F("Fehler beim Parsen des Datenports!"));
    return "";
  }

  // Verbindung zum Datenport für den Dateitransfer herstellen
  WiFiClient dataClient;
  if (dataClient.connect(ftp_server, dataPort)) {
    Serial.println(F("Verbindung zum Datenport hergestellt."));

    // Dateitransfer starten
    sendFTPCommand("RETR " + filename);  // RETR Befehl zum Herunterladen der Datei

    String fileContent = "";
    while (dataClient.connected() || dataClient.available()) {
      while (dataClient.available()) {
        fileContent += (char)dataClient.read();  // Dateiinhalt sammeln
      }
    }
    dataClient.stop();
    Serial.println(F("Dateiempfang abgeschlossen."));
    return fileContent;  // Dateiinhalt zurückgeben
  } else {
    Serial.println(F("Fehler beim Verbinden mit dem Datenport!"));
    return "";
  }
}

// Parsen der PASV-Antwort, um den Datenport zu extrahieren
int parsePasvResponse(String response) {
  Serial.print(F("PASV Antwort: "));
  Serial.println(response);

  // Suche nach der Zeichenfolge "Entering Passive Mode ("
  int start = response.indexOf('(');
  int end = response.indexOf(')');

  if (start == -1 || end == -1) {
    Serial.println(F("Fehler: Keine gültige PASV-Antwort erhalten."));
    return -1;  // Fehler, keine gültige PASV-Antwort
  }

  // Extrahiere den Text zwischen den Klammern
  String data = response.substring(start + 1, end);
  Serial.print(F("Extrahierte Daten: "));
  Serial.println(data);

  // Teile die Antwort auf, um die IP-Adresse und den Port zu extrahieren
  char* token = strtok((char*)data.c_str(), ",");
  int parts[6];
  int count = 0;

  while (token != nullptr && count < 6) {
    parts[count++] = atoi(token);  // Umwandlung der Teile in Zahlen
    token = strtok(nullptr, ",");
  }

  if (count == 6) {
    // Berechne den Datenport
    int dataPort = (parts[4] * 256) + parts[5];
    Serial.print(F("Datenport: "));
    Serial.println(dataPort);
    return dataPort;
  } else {
    Serial.println(F("Fehler: Ungültige PASV-Antwort."));
    return -1;  // Fehler, ungültige PASV-Antwort
  }
}

void sendFTPCommand(String command) {
  client.println(command);
  Serial.println("> " + command);

  if (command.startsWith("SIZE ")) {
    return;
  }

  waitForResponse();  // Warten auf Antwort
}

// Funktion, um auf eine Antwort des FTP-Servers zu warten
String waitForResponse() {
  String response = "";
  unsigned long startTime = millis();
  bool multiLine = false;
  String statusCode = "";

  while (millis() - startTime < 5000) {
    while (client.available()) {
      char c = client.read();
      response += c;

      // Falls erste Zeile, hole den Statuscode (z.B. "220", "230", "227", ...)
      if (response.indexOf("\r\n") != -1 && statusCode == "") {
        statusCode = response.substring(0, 3);  // Hole die ersten 3 Zeichen als Code
        if (response[3] == '-') {               // Mehrzeilige Antwort erkannt
          multiLine = true;
        }
      }

      // Falls `227 Entering Passive Mode (...)` erkannt wird, direkt zurückgeben
      if (response.indexOf("227 Entering Passive Mode") != -1) {
        Serial.print(F("PASV Antwort: "));
        Serial.println(response);
        return response;
      }

      // Falls normale Antwort oder gültige Antwort erkannt, abbrechen
      if (response.endsWith("\r\n")) {
        if (!multiLine || response.indexOf(statusCode + " ") != -1) {
          Serial.print(F("Antwort: "));
          Serial.println(response);
          return response;
        }
      }
    }
    delay(10);
  }

  Serial.println(F("Timeout beim Warten auf Antwort!"));
  return "TIMEOUT";
}

bool performFTPUpdate(String remoteFile) {

  tft->fillScreen(bgColor);
  tft->setCursor(0, 0);

  Serial.println("=== Starte Firmware-Update von: " + remoteFile);
  tft->println("Starte Firmware-Update...");
  tft->println("Firmware-File:");
  tft->println(remoteFile);

  sendFTPCommand("SIZE " + remoteFile);

  String temp = "";
  unsigned long stop = millis() + 4000;
  while (millis() < stop) {
    if (client.available()) temp += (char)client.read();
    delay(1);
  }

  uint32_t expectedSize = 0;
  if (temp.indexOf("213") != -1) {
    expectedSize = temp.substring(temp.indexOf("213") + 3).toInt();
  }

  Serial.printf(">>> FINAL ERGEBNIS: %lu Bytes <<<\n", expectedSize);

  sendFTPCommand("PASV");
  String pasvResponse = waitForResponse();
  int dataPort = parsePasvResponse(pasvResponse);
  if (dataPort == -1) return false;

  WiFiClient dataClient;
  if (!dataClient.connect(ftp_server, dataPort)) {
    Serial.println("Datenverbindung fehlgeschlagen");
    tft->println("Datenverbindung fehlgeschlagen");
    return false;
  }

  // Flash-Bereich reservieren
  uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  if (!Update.begin(maxSketchSpace)) {
    Serial.println("Nicht genug Flash-Speicher");
    tft->println("Nicht genug Flash");
    dataClient.stop();
    return false;
  }

  // Download starten
  sendFTPCommand("RETR " + remoteFile);

  // Direkt ins Flash schreiben → kein RAM-Verbrauch
  uint8_t buff[1024];
  size_t total = 0;
  static uint8_t lastPercent = 255;

  while (dataClient.connected() || dataClient.available()) {
    while (dataClient.available()) {
      int len = dataClient.read(buff, sizeof(buff));
      if (len > 0) {
        if (Update.write(buff, len) != len) {
          Serial.println("Flash write error");
          tft->println("Flash write error");
          Update.end();
          dataClient.stop();
          return false;
        }
        total += len;

        // Fortschritt auf Display alle 5 %
        uint8_t percent = (expectedSize > 0) ? (total * 100 / expectedSize) : 0;
        percent = constrain(percent, 0, 100);

        if (percent / 5 != lastPercent / 5 || percent == 100) {
          lastPercent = percent;

          tft->fillScreen(bgColor);
          tft->setCursor(30, 20);
          if (expectedSize > 0) {
            tft->print(F("Fortschritt: "));
            tft->print(percent);
            tft->print(F(" %"));
          }
          tft->setCursor(0, 40);
          tft->println(F("Empfangen: "));
          tft->print(total);
          tft->print(F(" / "));
          tft->print(expectedSize);
          tft->print(F(" Bytes"));

          Serial.printf("Update %d%% (%u/%u)\n", percent, total, expectedSize);
        }
      }
    }
    yield();
  }

  dataClient.stop();
  waitForResponse();  // 226 Transfer complete abholen

  if (Update.end(true)) {
    tft->fillScreen(bgColor);
    tft->setCursor(0, 0);
    Serial.printf("UPDATE ERFOLGREICH – %u Bytes – Neustart!\n", total);
    tft->println(F("UPDATE ERFOLGREICH!"));
    tft->print(total);
    tft->println(F(" Bytes"));
    tft->println(F(""));
    tft->println(F("Starte neu!"));
    delay(2000);
    ESP.restart();
    return true;
  } else {
    Update.printError(Serial);
    tft->fillScreen(bgColor);
    tft->setCursor(0, 0);
    tft->print(F("Update fehlgeschlagen"));
    return false;
  }
}

String formatVersion(float ver) {
  String s = String(ver);
  while (s.endsWith("0") && s.indexOf(".") != -1) {
    s = s.substring(0, s.length() - 1);
  }

  if (s.endsWith(".")) {
    s = s.substring(0, s.length() - 1);
  }
  return s;
}

void displayInfoFile(String content, int numLines) {

  int lineCount = 0;
  int startPos = 0;

  tft->setTextSize(1);

  while (lineCount < numLines && startPos < content.length()) {
    // Nächstes \n finden (oder Ende des Strings)
    int endPos = content.indexOf('\n', startPos);
    if (endPos == -1) endPos = content.length();

    // Zeile extrahieren
    String line = content.substring(startPos, endPos);
    line.trim();  // \r oder Leerzeichen am Ende weg

    // Auf maximal 26 Zeichen kürzen
    if (smallDisplay) {
      if (line.length() > 26) {
        line = line.substring(0, 26);  // nur erste 26 Zeichen
        // Optional: mit "…" kennzeichnen, wenn abgeschnitten
        // line = line.substring(0, 23) + "...";
      }
    } else {
      if (line.length() > 52) {
        line = line.substring(0, 52);  // nur erste 52 Zeichen
        // Optional: mit "…" kennzeichnen, wenn abgeschnitten
        // line = line.substring(0, 23) + "...";
      }
    }

    // Ausgeben
    tft->println(line);

    // Nächste Zeile
    lineCount++;
    startPos = endPos + 1;

    // Falls String zu Ende ist → Schleife beenden
    if (startPos >= content.length()) break;
  }
  if (!smallDisplay) {
    tft->setTextSize(2);
  }
}

// WiFi konfigurieren
void configureWiFi() {
  String ssid, passwd;

  // Lade die gespeicherten WiFi-Daten (falls vorhanden)
  loadWifiFromLittleFS(ssid, passwd);

  // Menu zeichnen
  displayWifiMenu();

  // Erhalte Benutzereingaben für SSID und Passwort
  if (wlanKeypadInput(ssid, passwd) != "Done") {
    return;
  }

  // Entferne \r und \n aus den Strings
  ssid = entferneUnerwuenschteZeichen(ssid);
  passwd = entferneUnerwuenschteZeichen(passwd);

  // Versuche, LittleFS zu initialisieren
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS Mount Failed"));
    return;
  }

  // Speichern der WiFi-Daten in der Datei
  File file = LittleFS.open("/wifi.txt", "w");
  if (!file) {
    Serial.println(F("Wifi file open failed"));
    return;
  }

  // Schreibe SSID und Passwort in die Datei
  file.println(ssid);
  file.println(passwd);
  file.close();

  Serial.println(F("WiFi credentials saved."));
  Serial.println(F("Reboot..."));

  delay(3000);

  ESP.restart();
}

//////////////////////////////////////////////////
// Funktionen und Hauptroutine für Karussell Spiel
//////////////////////////////////////////////////

void generateNumbers();
void printMessage(String msg);
void printErrorMessage(String msg);
void printInput(String msg1, String msg2, String msg3);
String evaluateInput(byte num1, byte num2, byte num3);

void karussell() {
  myDFPlayer.loop(13);
  tft->fillScreen(bgColor);
  tft->setTextColor(textColor);
  tft->setTextSize(smallDisplay ? 1 : 2);
  randomSeed(analogRead(0));
  generateNumbers();
  messageY = 0;

  tft->setCursor(0, 0);
  printMessage("Finde die 3-stellige Zahl!");
  printMessage("Du hast 9 Versuche!");
  if (!smallDisplay) {
    displayButtonAndText(0, 220, "Loeschen", 'C');
    displayButtonAndText(160, 220, "Abbruch", 'A');
    tft->setCursor(160, 200);
    tft->print(F("Eingabe: "));
  }
  delay(1000);
  kcLoop();
  delay(2000);
  // Spiel fertig. Zurück ins Menu
}

void kcLoop() {
  byte versuche = 1;
  while (true) {
    byte num1, num2, num3;
    String msg1 = " ";
    String msg2 = " ";
    String msg3 = " ";
    while (true) {
      num1 = num2 = num3 = -1;  // Initialisierung
      byte i = 0;
      printInput(msg1, msg2, msg3);
      while (i < 3) {
        autoPowerOff();
        char eingabe = scanKeypad();
        if (eingabe >= '0' && eingabe <= '9') {
          if (i == 0) {
            num1 = eingabe - '0';
            msg1 = eingabe;
          }
          if (i == 1) {
            num2 = eingabe - '0';
            msg2 = eingabe;
          }
          if (i == 2) {
            num3 = eingabe - '0';
            msg3 = eingabe;
          }
          i++;
          delay(300);
          printInput(msg1, msg2, msg3);
        } else if (eingabe == 'C') {
          i = 0;
          num1 = num2 = num3 = -1;
          msg1 = msg2 = msg3 = " ";
          //printMessage("Eingabe gelöscht");
          printInput(msg1, msg2, msg3);
          delay(500);
        } else if (eingabe == 'A') {
          i = 0;
          num1 = num2 = num3 = -1;
          tft->fillScreen(bgColor);
          messageY = 0;
          printMessage("Abgebrochen");
          printMessage("Gesucht war: " + String(ziffer1) + String(ziffer2) + String(ziffer3));
          delay(3000);
          return;
        }
      }
      if (num1 != num2 && num1 != num3 && num2 != num3) break;
      msg1 = msg2 = msg3 = " ";
      printErrorMessage("Eingabe ungueltig!");
    }

    msg1 = msg2 = msg3 = " ";
    printInput(msg1, msg2, msg3);
    String antwort = evaluateInput(num1, num2, num3);
    printMessage("Spiel " + String(versuche) + ": " + String(num1) + String(num2) + String(num3) + " " + antwort);

    if (antwort == "As As As ") {
      printMessage("Spiel gewonnen!          ");
      delay(3000);
      return;
    }

    versuche++;
    if (versuche > 9) {
      tft->fillScreen(bgColor);
      messageY = smallDisplay ? 100 : 183;
      printMessage("Spiel verloren!");
      printMessage("Gesucht war: " + String(ziffer1) + String(ziffer2) + String(ziffer3));
      delay(3000);
      return;
    }
  }
  delay(3000);
}

void generateNumbers() {
  randomSeed(micros() + analogRead(A0) + millis() + ESP.getCycleCount());
  ziffer1 = random(10);
  do { ziffer2 = random(10); } while (ziffer2 == ziffer1);
  do { ziffer3 = random(10); } while (ziffer3 == ziffer1 || ziffer3 == ziffer2);

  Serial.print(F("Generierte Zahl: "));
  Serial.print(ziffer1);
  Serial.print(ziffer2);
  Serial.println(ziffer3);
}

String evaluateInput(byte num1, byte num2, byte num3) {
  String result = "";
  if (ziffer1 == num1) result += "As ";
  if (ziffer2 == num2) result += "As ";
  if (ziffer3 == num3) result += "As ";
  if (ziffer1 == num2 || ziffer1 == num3) result += "Gut ";
  if (ziffer2 == num1 || ziffer2 == num3) result += "Gut ";
  if (ziffer3 == num1 || ziffer3 == num2) result += "Gut ";
  return result;
}

void printMessage(String msg) {
  tft->setCursor(0, messageY);
  tft->print(msg);
  if (smallDisplay) {
    messageY += 10;
    if (messageY > 120) {
      messageY = 0;
      tft->fillScreen(bgColor);
    }
  } else {
    messageY += 20;
    if (messageY > 230) {
      messageY = 0;
      tft->fillScreen(bgColor);
    }
  }
}

void printErrorMessage(String msg) {
  tft->setTextColor(textColor, bgColor);
  int16_t x1, y1;
  uint16_t textWidth, textHeight;

  // Textgröße berechnen
  tft->getTextBounds(
    msg,
    0,
    smallDisplay ? 120 : 224,
    &x1, &y1, &textWidth, &textHeight);

  // Hintergrund exakt über den Text zeichnen
  tft->fillRect(
    x1,
    y1 - 3,
    (smallDisplay ? 160 : 320) - x1,
    textHeight + (smallDisplay ? 6 : 12),
    0);

  tft->setCursor(0, smallDisplay ? 120 : 224);
  tft->print(msg);

  delay(2000);

  // Hintergrund erneut löschen
  tft->fillRect(
    x1,
    y1 - 3,
    (smallDisplay ? 160 : 320) - x1,
    textHeight + (smallDisplay ? 6 : 12),
    0);

  // Buttons
  displayButtonAndText(
    0,
    smallDisplay ? 118 : 220,
    "Loeschen",
    'C');

  displayButtonAndText(
    smallDisplay ? 80 : 160,
    smallDisplay ? 118 : 220,
    "Abbruch",
    'A');
}

void printInput(String msg1, String msg2, String msg3) {
  tft->setTextColor(textColor, bgColor);
  tft->setCursor(smallDisplay ? 80 : 272,
                 smallDisplay ? 108 : 200);
  //tft->print(F("Eingabe: "));
  tft->print(msg1);
  tft->print(msg2);
  tft->print(msg3);
}

//////////////////////////////////////////////////
// Funktionen und Hauptroutine für 4 gewinnt Spiel
//////////////////////////////////////////////////

void vierGewinnt() {

  SPALTEN = 7;
  ZEILEN = 6;
  pfeilPos = SPALTEN / 2;  // Startposition des Pfeils in der Mitte
  spielerDran = true;      // Wechselt nach jedem Zug
  bgColor = ST77XX_BLACK;
  spielerColor = ST77XX_RED;
  computerColor = ST77XX_YELLOW;
  iconColor = ST77XX_WHITE;
  textColor = ST77XX_WHITE;
  if (smallDisplay) {
    radius = 7;
    xStart = 11;
    yStart = 19;
    xEnd = 143;
    yEnd = 99;
  } else {
    radius = 14;
    xStart = 22;
    yStart = 38;
    xEnd = 286;
    yEnd = 198;
  }
  xStep = (xEnd - xStart) / (SPALTEN - 1);
  yStep = (yEnd - yStart) / (ZEILEN - 1);

  // Spielfeld zurücksetzen (alle Werte auf 0 setzen)
  initSpielfeld();

  tft->fillScreen(bgColor);
  tft->setTextColor(textColor);

  tft->setTextSize(smallDisplay ? 2 : 4);
  tft->setCursor(smallDisplay ? 24 : 48, 10);
  tft->print(F("4 gewinnt"));

  tft->setTextSize(smallDisplay ? 1 : 2);
  tft->setCursor(8, smallDisplay ? 39 : 59);
  tft->print(F("Du hast die roten Steine"));

  displayButtonAndText(
    smallDisplay ? 50 : 100,
    smallDisplay ? 64 : 84,
    "Abbruch",
    'A');

  delay(3000);
  tft->fillScreen(bgColor);
  tft->setTextSize(smallDisplay ? 1 : 2);
  myDFPlayer.reset();
  delay(500);
  myDFPlayer.volume(loadVolumeFromLittleFS());

  // Taste 4
  displayButtonAndText(
    smallDisplay ? 9 : 23,
    smallDisplay ? 119 : 220,
    "",
    '4');

  // Linkspfeil
  drawBitmapScaled(
    0,
    smallDisplay ? 120 : 223,
    backspaceBitmap, 8, 8, iconColor);

  // Taste 6
  displayButtonAndText(
    smallDisplay ? 140 : 279,
    smallDisplay ? 119 : 220,
    "",
    '6');

  // Rechtspfeil
  drawBitmapScaled(
    smallDisplay ? 152 : 304,
    smallDisplay ? 120 : 223,
    rightBitmap, 8, 8, iconColor);

  // Taste 5 (setzen)
  displayButtonAndText(
    smallDisplay ? 50 : 102,
    smallDisplay ? 119 : 220,
    "setzen",
    '5');

  // Spielfeld zeichnen

  for (byte i = 0; i < SPALTEN; i++) {
    for (byte j = 0; j < ZEILEN; j++) {
      zeichneLeeresFeld(i, j);
    }
  }

  delay(500);
  vierGewinntLoop();
  delay(1000);
  // Spiel fertig. Zurück ins Menu
}

void vierGewinntLoop() {
  while (true) {
    zeichnePfeil(pfeilPos);

    if (spielerDran) {
      if (!spielerEingabe()) {
        return;
      }
    } else {
      computerZug();
    }

    if (pruefeGewinn(SPIELER)) {
      Serial.println(F("Spieler gewinnt!"));
      return;
    } else if (pruefeGewinn(COMPUTER)) {
      Serial.println(F("Computer gewinnt!"));
      return;
    }

    spielerDran = !spielerDran;  // Nächster Zug
  }
}

bool spielerEingabe() {
  zeichnePfeil(pfeilPos);
  char key;
  char lastKey = '\0';
  bool keyReleased = true;

  while (true) {
    autoPowerOff();
    key = scanKeypad();

    if (key != '\0' && key != lastKey && keyReleased) {
      if (key == 'A') {
        return false;
      }
      if (key == '4') {
        pfeilPos = (pfeilPos + SPALTEN - 1) % SPALTEN;
        zeichnePfeil(pfeilPos);
      } else if (key == '6') {
        pfeilPos = (pfeilPos + 1) % SPALTEN;
        zeichnePfeil(pfeilPos);
      } else if (key == '5') {
        if (setzeStein(pfeilPos)) {
          return true;
        }
      }
      lastKey = key;
      keyReleased = false;
    }

    if (key == '\0' || key == ' ') {
      keyReleased = true;
      lastKey = '\0';
    }

    delay(50);
  }
}

void computerZug() {
  int besterWert = -10000;
  byte besteSpalte = 0;
  zeichnePfeil(8);

  for (byte spalte = 0; spalte < SPALTEN; spalte++) {
    if (istGueltigerZug(spalte)) {
      macheZug(spalte, COMPUTER);
      int zugWert = minimaxVierGewinnt(MAX_DEPTH, false);
      rueckgaengigMachen(spalte);

      if (zugWert > besterWert) {
        besterWert = zugWert;
        besteSpalte = spalte;
      }
    }
  }
  // Setze und zeichne besten Zug
  for (byte zeile = ZEILEN - 1; zeile >= 0; zeile--) {
    if (spielfeld[besteSpalte][zeile] == LEER) {
      myDFPlayer.play(7);
      spielfeld[besteSpalte][zeile] = COMPUTER;
      zeichneStein(besteSpalte, zeile, COMPUTER);
      zeichnePfeil(besteSpalte);
      Serial.print(F("Computer setzt in Spalte "));
      Serial.println(besteSpalte);
      delay(1000);
      break;
    }
  }
}

// Macht einen Zug (setzt den Stein in die unterste freie Zeile)
void macheZug(byte spalte, byte spieler) {
  for (int zeile = ZEILEN - 1; zeile >= 0; zeile--) {
    if (spielfeld[spalte][zeile] == LEER) {
      spielfeld[spalte][zeile] = spieler;
      break;
    }
  }
}

// Macht einen Zug rückgängig (entfernt den obersten Stein)
void rueckgaengigMachen(byte spalte) {
  for (int zeile = 0; zeile < ZEILEN; zeile++) {
    if (spielfeld[spalte][zeile] != LEER) {
      spielfeld[spalte][zeile] = LEER;
      break;
    }
  }
}

// Setze Spieler Stein
bool setzeStein(byte spalte) {
  for (int zeile = ZEILEN - 1; zeile >= 0; zeile--) {
    if (spielfeld[spalte][zeile] == LEER) {
      myDFPlayer.play(7);
      spielfeld[spalte][zeile] = SPIELER;
      zeichneStein(spalte, zeile, SPIELER);
      Serial.print(F("Spieler hat gesetzt! Spalte "));
      Serial.println(spalte);
      return true;
    }
  }
  Serial.println(F("Spalte nicht möglich"));
  return false;
}

bool pruefeGewinn(byte spieler) {
  // Vertikale Prüfung
  for (int spalte = 0; spalte < SPALTEN; spalte++) {
    for (int zeile = 0; zeile <= ZEILEN - 4; zeile++) {
      if (spielfeld[spalte][zeile] == spieler && spielfeld[spalte][zeile + 1] == spieler && spielfeld[spalte][zeile + 2] == spieler && spielfeld[spalte][zeile + 3] == spieler) {
        myDFPlayer.play(18);
        for (int i = 0; i < 4; i++) {
          zeichneLeeresFeld(spalte, zeile);
          zeichneLeeresFeld(spalte, zeile + 1);
          zeichneLeeresFeld(spalte, zeile + 2);
          zeichneLeeresFeld(spalte, zeile + 3);
          delay(500);
          zeichneStein(spalte, zeile, spieler);
          zeichneStein(spalte, zeile + 1, spieler);
          zeichneStein(spalte, zeile + 2, spieler);
          zeichneStein(spalte, zeile + 3, spieler);
          delay(500);
        }
        //Serial.println(F("Vertikale Prüfung"));
        printSpielfeld();
        return true;
      }
    }
  }

  // Horizontale Prüfung
  for (int zeile = 0; zeile < ZEILEN; zeile++) {
    for (int spalte = 0; spalte <= SPALTEN - 4; spalte++) {
      if (spielfeld[spalte][zeile] == spieler && spielfeld[spalte + 1][zeile] == spieler && spielfeld[spalte + 2][zeile] == spieler && spielfeld[spalte + 3][zeile] == spieler) {
        myDFPlayer.play(18);
        for (int i = 0; i < 4; i++) {
          zeichneLeeresFeld(spalte, zeile);
          zeichneLeeresFeld(spalte + 1, zeile);
          zeichneLeeresFeld(spalte + 2, zeile);
          zeichneLeeresFeld(spalte + 3, zeile);
          delay(500);
          zeichneStein(spalte, zeile, spieler);
          zeichneStein(spalte + 1, zeile, spieler);
          zeichneStein(spalte + 2, zeile, spieler);
          zeichneStein(spalte + 3, zeile, spieler);
          delay(500);
        }
        //Serial.println(F("Horizontale Prüfung"));
        printSpielfeld();
        return true;
      }
    }
  }

  // Diagonale Prüfung (↘ von links oben nach rechts unten)
  for (int zeile = 0; zeile <= ZEILEN - 4; zeile++) {
    for (int spalte = 0; spalte <= SPALTEN - 4; spalte++) {
      if (spielfeld[spalte][zeile] == spieler && spielfeld[spalte + 1][zeile + 1] == spieler && spielfeld[spalte + 2][zeile + 2] == spieler && spielfeld[spalte + 3][zeile + 3] == spieler) {
        myDFPlayer.play(18);
        for (int i = 0; i < 4; i++) {
          zeichneLeeresFeld(spalte, zeile);
          zeichneLeeresFeld(spalte + 1, zeile + 1);
          zeichneLeeresFeld(spalte + 2, zeile + 2);
          zeichneLeeresFeld(spalte + 3, zeile + 3);
          delay(500);
          zeichneStein(spalte, zeile, spieler);
          zeichneStein(spalte + 1, zeile + 1, spieler);
          zeichneStein(spalte + 2, zeile + 2, spieler);
          zeichneStein(spalte + 3, zeile + 3, spieler);
          delay(500);
        }
        //Serial.println(F("Diagonale Prüfung (↘ von links oben nach rechts unten)"));
        printSpielfeld();
        return true;
      }
    }
  }

  // Diagonale Prüfung (↗ von links unten nach rechts oben)
  for (int zeile = 3; zeile < ZEILEN; zeile++) {
    for (int spalte = 0; spalte <= SPALTEN - 4; spalte++) {
      if (spielfeld[spalte][zeile] == spieler && spielfeld[spalte + 1][zeile - 1] == spieler && spielfeld[spalte + 2][zeile - 2] == spieler && spielfeld[spalte + 3][zeile - 3] == spieler) {
        myDFPlayer.play(18);
        for (int i = 0; i < 4; i++) {
          zeichneLeeresFeld(spalte, zeile);
          zeichneLeeresFeld(spalte + 1, zeile - 1);
          zeichneLeeresFeld(spalte + 2, zeile - 2);
          zeichneLeeresFeld(spalte + 3, zeile - 3);
          delay(500);
          zeichneStein(spalte, zeile, spieler);
          zeichneStein(spalte + 1, zeile - 1, spieler);
          zeichneStein(spalte + 2, zeile - 2, spieler);
          zeichneStein(spalte + 3, zeile - 3, spieler);
          delay(500);
        }
        //Serial.println(F("Diagonale Prüfung (↗ von links unten nach rechts oben)"));
        //Serial.println(spielfeld[spalte][zeile]);
        printSpielfeld();
        return true;
      }
    }
  }
  return false;
}

void printSpielfeld() {
  Serial.println();
  for (byte zeile = 0; zeile < ZEILEN; zeile++) {
    for (byte spalte = 0; spalte < SPALTEN; spalte++) {
      Serial.print(spielfeld[spalte][zeile]);
    }
    Serial.println();  // Neue Zeile nach jeder Zeile
  }
  Serial.println();
}

int minimaxVierGewinnt(int tiefe, bool maximierend) {
  int bewertung = bewerten();
  if (bewertung == 1000 || bewertung == -1000 || tiefe == 0) {
    return bewertung;
  }

  if (maximierend) {
    int maxWert = -10000;
    for (byte spalte = 0; spalte < SPALTEN; spalte++) {
      if (istGueltigerZug(spalte)) {
        macheZug(spalte, COMPUTER);
        Serial.print(F(""));
        int wert = minimaxVierGewinnt(tiefe - 1, false);
        rueckgaengigMachen(spalte);
        maxWert = max(maxWert, wert);
      }
    }
    return maxWert;
  } else {
    int minWert = 10000;
    for (byte spalte = 0; spalte < SPALTEN; spalte++) {
      if (istGueltigerZug(spalte)) {
        macheZug(spalte, SPIELER);
        Serial.print(F(""));
        int wert = minimaxVierGewinnt(tiefe - 1, true);
        rueckgaengigMachen(spalte);
        minWert = min(minWert, wert);
      }
    }
    return minWert;
  }
}

// Funktion, um einen Spielstein in der gegebenen x, y Koordinate zu zeichnen
void zeichneStein(byte spalte, byte zeile, byte spieler) {
  // Berechne die Display-Positionen basierend auf den Spielfeld-Koordinaten
  uint16_t x = xStart + spalte * xStep;
  uint16_t y = yStart + zeile * yStep;

  // Wähle die Farbe des Spielsteins basierend darauf, welcher Spieler dran ist
  uint16_t steinFarbe;
  if (spieler == SPIELER) {
    steinFarbe = spielerColor;  // Spieler-Farbe Stein
  } else {
    steinFarbe = computerColor;  // Computer-Farbe Stein
  }

  // Zeichne den gefüllten Kreis (Spielstein) an der gegebenen Position
  tft->fillCircle(x, y, radius, steinFarbe);
}

// Funktion, um ein leeres Spielfeld in der gegebenen x, y Koordinate zu zeichnen
void zeichneLeeresFeld(byte spalte, byte zeile) {
  tft->fillCircle(xStart + spalte * xStep, yStart + zeile * yStep, radius, bgColor);
  tft->drawCircle(xStart + spalte * xStep, yStart + zeile * yStep, radius, iconColor);
}

void zeichnePfeil(byte x) {
  //byte x = xStart + pfeilPos * xStep;
  uint16_t pfeilFarbe;
  tft->fillRect(
    0, 0,
    smallDisplay ? 159 : 319,
    smallDisplay ? 10 : 20,
    0);
  if (spielerDran) {
    pfeilFarbe = spielerColor;  // Spieler - Roter Stein
  } else {
    pfeilFarbe = computerColor;  // Computer - Gelber Stein
  }
  drawBitmapScaled((xStart + x * xStep) - (smallDisplay ? 3 : 6),
                   1, downBitmap, 8, 8, pfeilFarbe);
}

// Prüft, ob ein Zug in der Spalte möglich ist
bool istGueltigerZug(byte spalte) {
  return spielfeld[spalte][0] == LEER;
}

int bewerten() {
  int score = 0;

  // Reihen horizontal überprüfen
  for (byte zeile = 0; zeile < ZEILEN; zeile++) {
    for (byte spalte = 0; spalte < SPALTEN - 3; spalte++) {
      int computerSteine = 0, spielerSteine = 0;
      for (byte i = 0; i < 4; i++) {
        if (spielfeld[spalte + i][zeile] == COMPUTER) computerSteine++;
        if (spielfeld[spalte + i][zeile] == SPIELER) spielerSteine++;
      }
      if (computerSteine == 4) return 1000;
      if (spielerSteine == 4) return -1000;
      score += (computerSteine - spielerSteine);
    }
  }

  // Reihen vertikal überprüfen
  for (byte spalte = 0; spalte < SPALTEN; spalte++) {
    for (byte zeile = 0; zeile < ZEILEN - 3; zeile++) {
      int computerSteine = 0, spielerSteine = 0;
      for (byte i = 0; i < 4; i++) {
        if (spielfeld[spalte][zeile + i] == COMPUTER) computerSteine++;
        if (spielfeld[spalte][zeile + i] == SPIELER) spielerSteine++;
      }
      if (computerSteine == 4) return 1000;
      if (spielerSteine == 4) return -1000;
      score += (computerSteine - spielerSteine);
    }
  }

  // Diagonale von links unten nach rechts oben überprüfen
  for (byte spalte = 0; spalte < SPALTEN - 3; spalte++) {
    for (byte zeile = 0; zeile < ZEILEN - 3; zeile++) {
      int computerSteine = 0, spielerSteine = 0;
      for (byte i = 0; i < 4; i++) {
        if (spielfeld[spalte + i][zeile + i] == COMPUTER) computerSteine++;
        if (spielfeld[spalte + i][zeile + i] == SPIELER) spielerSteine++;
      }
      if (computerSteine == 4) return 1000;
      if (spielerSteine == 4) return -1000;
      score += (computerSteine - spielerSteine);
    }
  }

  // Diagonale von rechts unten nach links oben überprüfen
  for (byte spalte = 0; spalte < SPALTEN - 3; spalte++) {
    for (byte zeile = 3; zeile < ZEILEN; zeile++) {
      int computerSteine = 0, spielerSteine = 0;
      for (byte i = 0; i < 4; i++) {
        if (spielfeld[spalte + i][zeile - i] == COMPUTER) computerSteine++;
        if (spielfeld[spalte + i][zeile - i] == SPIELER) spielerSteine++;
      }
      if (computerSteine == 4) return 1000;
      if (spielerSteine == 4) return -1000;
      score += (computerSteine - spielerSteine);
    }
  }

  return score;
}

////////////////////////////////////////////////
// Funktionen und Hauptroutine für Reversi Spiel
////////////////////////////////////////////////

// Funktionsprototypen (Deklaration)
void reversiLoop();
void zeichneMarkierung(byte xNeu, byte yNeu);
bool setzeSteinReversi(byte xPos, byte yPos);

void reversi() {
  SPALTEN = 8;
  ZEILEN = 8;
  bgColor = 0x03E0;  // Grün
  spielerColor = ST77XX_BLACK;
  computerColor = ST77XX_WHITE;
  iconColor = ST77XX_WHITE;
  textColor = ST77XX_WHITE;
  if (smallDisplay) {
    radius = 7;
    xStart = 22;
    yStart = 8;
    xEnd = 135;
    yEnd = 121;
  } else {
    radius = 12;
    xStart = 44;
    yStart = 14;
    xEnd = 270;
    yEnd = 225;
  }
  xStep = (xEnd - xStart) / (SPALTEN - 1);
  yStep = (yEnd - yStart) / (ZEILEN - 1);
  xPos = 0;
  yPos = 0;
  spielerDran = true;  // Wechselt nach jedem Zug

  tft->fillScreen(bgColor);
  tft->setTextColor(textColor);

  gameControllInfo("Reversi", "Du hast die", "schwarzen Steine", true);

  delay(5000);

  tft->fillScreen(bgColor);

  // Spielfeld zeichnen

  iconColor = 0x8410;  // Mittelgrau
  for (byte i = 0; i < SPALTEN; i++) {
    for (byte j = 0; j < ZEILEN; j++) {
      zeichneLeeresFeld(i, j);
    }
  }

  // Spielfeld init & zeichnen
  initSpielfeld();
  spielfeld[4][3] = SPIELER;
  spielfeld[3][4] = SPIELER;
  zeichneStein(4, 3, SPIELER);
  zeichneStein(3, 4, SPIELER);
  spielfeld[3][3] = COMPUTER;
  spielfeld[4][4] = COMPUTER;
  zeichneStein(3, 3, COMPUTER);
  zeichneStein(4, 4, COMPUTER);

  Serial.println(F("Spielfeld nach init:"));
  printSpielfeld();

  reversiLoop();
  // Spiel fertig. Zurück ins Menu
}

void zeichneSpielfeld() {
  for (int x = 0; x < SPALTEN; x++) {
    for (int y = 0; y < ZEILEN; y++) {
      if (spielfeld[x][y] != 0) {
        zeichneStein(x, y, spielfeld[x][y]);
      }
    }
  }
}

bool reversiSpielerEingabe() {
  zeichneMarkierung(xPos, yPos);
  char key;
  char lastKey = '\0';
  bool keyReleased = true;

  while (true) {
    autoPowerOff();
    key = scanKeypad();
    if (key != '\0' && key != lastKey && keyReleased) {
      if (key == 'A') {
        return false;
      }
      if (key == '4') {
        xPos = (xPos + SPALTEN - 1) % SPALTEN;
        zeichneMarkierung(xPos, yPos);
      } else if (key == '6') {
        xPos = (xPos + 1) % SPALTEN;
        zeichneMarkierung(xPos, yPos);
      } else if (key == '2') {
        yPos = (yPos + ZEILEN - 1) % ZEILEN;
        zeichneMarkierung(xPos, yPos);
      } else if (key == '8') {
        yPos = (yPos + 1) % ZEILEN;
        zeichneMarkierung(xPos, yPos);
      } else if (key == '5') {
        if (setzeSteinReversi(xPos, yPos)) {
          return true;
        }
      }

      lastKey = key;
      keyReleased = false;
    }

    if (key == '\0' || key == ' ') {
      keyReleased = true;
      lastKey = '\0';
    }
    delay(50);
  }
}

// Funktion, um Markierung an der gegebenen x, y Koordinate zu zeichnen
void zeichneMarkierung(byte xNeu, byte yNeu) {
  static byte xPos = 0;  // Merkt sich die letzte X-Position
  static byte yPos = 0;  // Merkt sich die letzte Y-Position

  // Alte Markierung entfernen (z. B. mit Hintergrundfarbe übermalen)
  tft->drawRect(((xStart + xPos * xStep) - radius) - 1, ((yStart + yPos * yStep) - radius) - 1, 2 * radius + 3, 2 * radius + 3, bgColor);

  // Berechne die Display-Positionen basierend auf den Spielfeld-Koordinaten
  xPos = xNeu;
  yPos = yNeu;

  // Neue Markierung zeichnen
  tft->drawRect(((xStart + xPos * xStep) - radius) - 1, ((yStart + yPos * yStep) - radius) - 1, 2 * radius + 3, 2 * radius + 3, ST77XX_RED);
}

bool setzeSteinReversi(byte xPos, byte yPos) {
  // Überprüfen, ob der Zug gültig ist
  if (istGueltigerZugReversi(xPos, yPos, SPIELER)) {
    myDFPlayer.play(7);  // Zug bestätigen
    spielfeld[xPos][yPos] = SPIELER;
    macheZugReversi(xPos, yPos, SPIELER);
    Serial.print(F("Spieler hat gesetzt X, Y: "));
    Serial.print(xPos);
    Serial.print(F(", "));
    Serial.println(yPos);
    printSpielfeld();
    return true;
  } else {
    myDFPlayer.play(12);  // Fehler-Sound abspielen
    return false;
  }
}

// Überprüft, ob ein Zug gültig ist
bool istGueltigerZugReversi(int x, int y, byte spieler) {
  if (spielfeld[x][y] != LEER) return false;
  byte gegner = (spieler == SPIELER) ? COMPUTER : SPIELER;

  for (auto& r : richtungen) {
    int nx = x + r[0], ny = y + r[1];
    bool hatGegner = false;

    while (nx >= 0 && nx < SPALTEN && ny >= 0 && ny < ZEILEN && spielfeld[nx][ny] == gegner) {
      hatGegner = true;
      nx += r[0];
      ny += r[1];
    }

    if (hatGegner && nx >= 0 && nx < SPALTEN && ny >= 0 && ny < ZEILEN && spielfeld[nx][ny] == spieler) {
      return true;
    }
  }
  return false;
}

// Führt einen Zug aus und kehrt die Steine um
void macheZugReversi(byte x, byte y, byte spieler) {
  spielfeld[x][y] = spieler;

  byte gegner = (spieler == SPIELER) ? COMPUTER : SPIELER;

  for (auto& r : richtungen) {
    int nx = x + r[0], ny = y + r[1];
    std::vector<Zug> zuFlippen;

    while (nx >= 0 && nx < SPALTEN && ny >= 0 && ny < ZEILEN && spielfeld[nx][ny] == gegner) {
      zuFlippen.push_back({ nx, ny });
      nx += r[0];
      ny += r[1];
    }

    if (!zuFlippen.empty() && nx >= 0 && nx < SPALTEN && ny >= 0 && ny < ZEILEN && spielfeld[nx][ny] == spieler) {
      for (auto& zug : zuFlippen) {
        spielfeld[zug.x][zug.y] = spieler;
      }
    }
  }
}

// Bewertet das Spielfeld für den Minimax-Algorithmus
int64_t bewerteSpielfeld() {
  int64_t wert = 0;

  for (byte i = 0; i < SPALTEN; i++) {
    for (byte j = 0; j < ZEILEN; j++) {
      int64_t faktor = 1;  // Standardgewichtung

      // Ecksteine stärker gewichten
      if ((i == 0 && j == 0) || (i == 0 && j == ZEILEN - 1) || (i == SPALTEN - 1 && j == 0) || (i == SPALTEN - 1 && j == ZEILEN - 1)) {
        faktor = 5;  // Ecken haben höchste Priorität
      }
      // Randsteine (aber keine Ecken) moderat gewichten
      else if (i == 0 || i == SPALTEN - 1 || j == 0 || j == ZEILEN - 1) {
        faktor = 2;  // Randsteine sind wichtiger als mittlere
      }

      // Wert je nach Spieler setzen
      if (spielfeld[i][j] == COMPUTER) wert += faktor;
      else if (spielfeld[i][j] == SPIELER) wert -= faktor;
    }
  }
  return wert;
}

// Minimax mit Alpha-Beta-Pruning
int64_t minimaxReversi(int tiefe, bool maximierend, int64_t alpha, int64_t beta) {
  if (tiefe == 0) return bewerteSpielfeld();

  bool zugGefunden = false;

  if (maximierend) {
    int64_t maxWert = INT64_MIN;
    for (int x = 0; x < SPALTEN; x++) {
      for (int y = 0; y < ZEILEN; y++) {
        if (istGueltigerZugReversi(x, y, COMPUTER)) {
          zugGefunden = true;
          int backup[SPALTEN][ZEILEN];
          memcpy(backup, spielfeld, sizeof(spielfeld));
          macheZugReversi(x, y, COMPUTER);

          int64_t wert = minimaxReversi(tiefe - 1, false, alpha, beta);
          memcpy(spielfeld, backup, sizeof(spielfeld));

          maxWert = std::max(maxWert, wert);
          alpha = std::max(alpha, maxWert);

          // Pruning
          if (beta <= alpha) {
            return maxWert;
          }
        }
      }
    }
    return zugGefunden ? maxWert : bewerteSpielfeld();
  } else {  // Minimierender Spieler
    int64_t minWert = INT64_MAX;
    for (int x = 0; x < SPALTEN; x++) {
      for (int y = 0; y < ZEILEN; y++) {
        if (istGueltigerZugReversi(x, y, SPIELER)) {
          zugGefunden = true;
          int backup[SPALTEN][ZEILEN];
          memcpy(backup, spielfeld, sizeof(spielfeld));
          macheZugReversi(x, y, SPIELER);

          int64_t wert = minimaxReversi(tiefe - 1, true, alpha, beta);
          memcpy(spielfeld, backup, sizeof(spielfeld));

          minWert = std::min(minWert, wert);
          beta = std::min(beta, minWert);

          // Pruning
          if (beta <= alpha) {
            return minWert;
          }
        }
      }
    }
    return zugGefunden ? minWert : bewerteSpielfeld();
  }
}

// Findet den besten Zug für den Computer
Zug findeBestenZugReversi(int tiefe) {
  int besterWert = INT_MIN;
  Zug besterZug = { -1, -1 };

  // Backup-Array dynamisch erstellen
  byte** backup = (byte**)malloc(SPALTEN * sizeof(byte*));
  for (int i = 0; i < SPALTEN; i++) {
    backup[i] = (byte*)malloc(ZEILEN * sizeof(byte));
  }

  bool keinGueltigerZugGefunden = true;  // Flag, um zu prüfen, ob ein gültiger Zug gefunden wurde

  for (byte x = 0; x < SPALTEN; x++) {
    for (byte y = 0; y < ZEILEN; y++) {
      if (istGueltigerZugReversi(x, y, COMPUTER)) {

        // Kein gültiger Zug mehr gefunden, daher setzen wir das Flag zurück
        keinGueltigerZugGefunden = false;

        // Backup des aktuellen Spielfelds
        for (int i = 0; i < SPALTEN; i++) {
          memcpy(backup[i], spielfeld[i], ZEILEN * sizeof(byte));
        }

        macheZugReversi(x, y, COMPUTER);

        int64_t wert = minimaxReversi(tiefe - 1, false, INT_MIN, INT_MAX);

        // Spielfeld aus dem Backup wiederherstellen
        for (int i = 0; i < SPALTEN; i++) {
          memcpy(spielfeld[i], backup[i], ZEILEN * sizeof(byte));
        }

        if (wert > besterWert) {
          besterWert = wert;
          besterZug = { x, y };
        }
      }
    }
  }

  // Debug: Ausgabe, falls kein gültiger Zug gefunden wurde
  //if (keinGueltigerZugGefunden) {
  //  Serial.println("Kein gültiger Zug gefunden für den Computer.");
  //}

  // Speicher für das Backup-Array freigeben
  for (int i = 0; i < SPALTEN; i++) {
    free(backup[i]);
  }
  free(backup);

  return besterZug;
}

bool istSpielfeldVoll() {
  for (byte x = 0; x < SPALTEN; x++) {
    for (byte y = 0; y < ZEILEN; y++) {
      if (spielfeld[x][y] == 0) {  // Wenn ein Feld leer ist, ist das Spielfeld nicht voll
        return false;
      }
    }
  }
  Serial.println(F("Spielfeld voll"));
  printSpielfeld();
  return true;  // Wenn alle Felder belegt sind, ist das Spielfeld voll
}

void reversiSpielFertig() {
  // Steine zählen und Gewinner Ausgabe
  byte steineSpieler1 = 0;
  byte steineSpieler2 = 0;

  for (byte x = 0; x < SPALTEN; x++) {
    for (byte y = 0; y < ZEILEN; y++) {
      if (spielfeld[x][y] == 1) {
        steineSpieler1++;
      } else if (spielfeld[x][y] == 2) {
        steineSpieler2++;
      }
    }
  }
  tft->fillScreen(bgColor);
  tft->setTextColor(textColor);
  tft->setTextSize(smallDisplay ? 2 : 4);
  tft->setCursor(smallDisplay ? 40 : 80, smallDisplay ? 1 : 2);
  tft->print(F("Reversi"));
  tft->setTextSize(smallDisplay ? 1 : 2);
  tft->setCursor(smallDisplay ? 26 : 52, smallDisplay ? 25 : 50);
  tft->print(F("Steine Spieler:  "));
  tft->println(steineSpieler1);
  tft->setCursor(smallDisplay ? 26 : 52, smallDisplay ? 35 : 70);
  tft->print(F("Steine Computer: "));
  tft->print(steineSpieler2);
  iconColor = ST77XX_WHITE;
  int scoreWinner = steineSpieler2;
  int scoreLooser = steineSpieler1;
  String name = "COMPUTER";
  delay(5000);

  if (steineSpieler1 > steineSpieler2) {
    scoreWinner = steineSpieler1;
    scoreLooser = steineSpieler2;
    name = "";
  }

  if (hiScore("Reversi", scoreWinner, 0, name)) return;

  // Taste A zeichnen (Fertig)
  displayButtonAndText(smallDisplay ? 52 : 104, smallDisplay ? 109 : 218, "Fertig", 'A');
  char key;
  while (true) {
    autoPowerOff();
    key = scanKeypad();
    if (key == 'A') {
      return;
    }
  }
}

bool kannSpielerZugMachen(byte spieler) {
  for (int x = 0; x < SPALTEN; x++) {
    for (int y = 0; y < ZEILEN; y++) {
      if (spielfeld[x][y] == LEER) {  // Nur freie Felder prüfen
        if (istGueltigerZugReversi(x, y, spieler)) {
          return true;  // Ein gültiger Zug wurde gefunden
        }
      }
    }
  }
  return false;  // Kein gültiger Zug gefunden
}

void reversiLoop() {
  while (true) {
    if (spielerDran) {
      if (istSpielfeldVoll()) {
        reversiSpielFertig();
        return;
      }

      // Abfrage, ob zug möglich ist
      if (kannSpielerZugMachen(SPIELER)) {
        // Spieler macht einen Zug
        if (!reversiSpielerEingabe()) {
          break;  // Spiel beenden
        }
        zeichneSpielfeld();
        if (istSpielfeldVoll()) {
          reversiSpielFertig();
          return;
        }
      }
      spielerDran = !spielerDran;
    } else {

      Serial.println(F("Spielfeld vor Computerzug:"));
      printSpielfeld();

      if (istSpielfeldVoll()) {
        reversiSpielFertig();
        return;
      }

      // Abfrage, ob zug möglich ist
      if (kannSpielerZugMachen(COMPUTER)) {
        // Computer macht einen Zug
        Zug besterZug = findeBestenZugReversi(4);  // Such-Tiefe 3 für starke KI

        if (besterZug.x != -1 && besterZug.y != -1) {
          macheZugReversi(besterZug.x, besterZug.y, COMPUTER);

          Serial.println(F("Spielfeld nach Computerzug:"));
          printSpielfeld();

          zeichneSpielfeld();

          if (istSpielfeldVoll()) {
            reversiSpielFertig();
            return;
          }
        } else {
          Serial.println(F("Kein gültiger Zug für Computer möglich"));
          printSpielfeld();
        }
      }
      spielerDran = !spielerDran;
    }
    Serial.print(F("Spieler dran: "));
    Serial.println(spielerDran);
  }
  Serial.println(F("Spiel abgebrochen"));
}

//////////////////////////////////////////////
// Funktionen und Hauptroutine für Minesweeper
//////////////////////////////////////////////

void minesweeper() {
  NUM_MINES = loadMinesFromLittleFS();
  bgColor = ST77XX_BLACK;
  iconColor = ST77XX_WHITE;
  textColor = ST77XX_WHITE;

  TILE_SIZE = TFT_WIDTH / 8;  // Feldgröße 1/8 der Displayhöhe

  tft->fillScreen(bgColor);
  tft->setTextColor(textColor);

  String numOfMines = "Total Minen: " + String(NUM_MINES);

  gameControllInfo("Minesweeper", numOfMines, "", false);

  // * Aufdecken
  displayButtonAndText(
    0,
    smallDisplay ? 97 : 174,
    "Aufdecken",
    '*');

  // # Fahne setzen
  displayButtonAndText(
    0,
    smallDisplay ? 108 : 194,
    "Fahne setzen",
    '#');

  delay(5000);

  tft->fillScreen(bgColor);

  memset(revealed, false, sizeof(revealed));
  memset(flagged, false, sizeof(flagged));
  memset(grid, 0, sizeof(grid));

  cursorX = 0;
  cursorY = 0;
  placeMines();
  calculateNumbers();
  drawField();
  minesweeperLoop();
  // Spiel fertig. Zurück ins Menu
}

bool minesweeperLoop() {
  char key;
  char lastKey = '\0';
  bool keyReleased = true;

  while (true) {
    autoPowerOff();
    key = scanKeypad();

    if (key != '\0' && key != lastKey && keyReleased) {

      bool cursorMoved = false;

      if (key == '4' && cursorX > 0) {
        cursorX--;
        cursorMoved = true;
      } else if (key == '6' && cursorX < FIELD_WIDTH - 1) {
        cursorX++;
        cursorMoved = true;
      } else if (key == '2' && cursorY > 0) {
        cursorY--;
        cursorMoved = true;
      } else if (key == '8' && cursorY < FIELD_HEIGHT - 1) {
        cursorY++;
        cursorMoved = true;
      }

      else if (key == '*') {
        if (!flagged[cursorY][cursorX] && revealed[cursorY][cursorX] == false) {
          revealed[cursorY][cursorX] = true;
          drawField();
          if (grid[cursorY][cursorX] == 9) {  // Verloren
            memset(revealed, true, sizeof(revealed));
            myDFPlayer.play(15);
            drawField();
            delay(5000);
            return false;
          }
          revealEmpty(cursorX, cursorY);
          drawField();
          if (checkWin()) {  // Gewonnen?
            memset(revealed, true, sizeof(revealed));
            myDFPlayer.play(16);
            drawField();
            tft->setCursor(7, 50);
            tft->setTextSize(smallDisplay ? 2 : 4);
            tft->setTextColor(ST77XX_GREEN, bgColor);
            tft->print("Gewonnen!");
            delay(5000);
            return true;
          }
        }
      } else if (key == '#') {
        flagged[cursorY][cursorX] = !flagged[cursorY][cursorX];
        drawField();
      } else if (key == 'A' && cursorY < FIELD_HEIGHT - 1) return false;

      if (cursorMoved) {
        updateCursor();  // Nur aufrufen wenn Cursor bewegt
      }

      lastKey = key;
      keyReleased = false;
    }

    if (key == '\0' || key == ' ') {
      keyReleased = true;
      lastKey = '\0';
    }

    delay(50);
  }
}

void placeMines() {
  byte placed = 0;
  randomSeed(micros() + analogRead(A0) + millis() + ESP.getCycleCount());
  while (placed < NUM_MINES) {
    int x = random(FIELD_WIDTH);
    int y = random(FIELD_HEIGHT);
    if (grid[y][x] != 9) {
      grid[y][x] = 9;
      placed++;
    }
  }
}

void calculateNumbers() {
  for (int y = 0; y < FIELD_HEIGHT; y++) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
      if (grid[y][x] == 9) continue;
      byte count = 0;
      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          int nx = x + dx, ny = y + dy;
          if (nx >= 0 && nx < FIELD_WIDTH && ny >= 0 && ny < FIELD_HEIGHT && grid[ny][nx] == 9) {
            count++;
          }
        }
      }
      grid[y][x] = count;
    }
  }
}

void revealEmpty(int x, int y) {
  if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT || grid[y][x] == 9) return;
  revealed[y][x] = true;
  if (grid[y][x] == 0) {
    for (int dy = -1; dy <= 1; dy++) {
      for (int dx = -1; dx <= 1; dx++) {
        if (!revealed[y + dy][x + dx]) {
          revealEmpty(x + dx, y + dy);
          // Serial.print(F("Aufdecken (X,Y): "));
          // Serial.print(x + dx);
          // Serial.print(F(", "));
          // Serial.println(y + dy);
        }
      }
    }
  }
}

void drawTile(int x, int y) {
  int px = x * TILE_SIZE, py = y * TILE_SIZE;
  if (revealed[y][x]) {
    if (grid[y][x] == 9) {
      drawMine(px, py);
    } else {
      tft->fillRect(px, py, TILE_SIZE, TILE_SIZE, ST77XX_LIGHTGREY);
      if (grid[y][x] > 0) {
        if (grid[y][x] == 1) textColor = ST77XX_BLUE;
        if (grid[y][x] == 2) textColor = ST77XX_DARKGREEN;
        if (grid[y][x] == 3) textColor = ST77XX_RED;
        if (grid[y][x] == 4) textColor = ST77XX_DARKBLUE;
        if (grid[y][x] == 5) textColor = ST77XX_WINERED;
        if (grid[y][x] == 6) textColor = 0x0451;  // Türkis
        if (grid[y][x] == 7) textColor = ST77XX_BLACK;
        if (grid[y][x] == 8) textColor = ST77XX_DARKGREY;
        tft->setCursor(px + (TILE_SIZE / 2.7), py + (TILE_SIZE / 4));
        tft->setTextColor(textColor);
        tft->print(grid[y][x]);
        // Serial.print(F("Aufgedeckt (X,Y): "));
        // Serial.print(x);
        // Serial.print(F(", "));
        // Serial.print(y);
        // Serial.print(F(", Wert: "));
        // Serial.println(grid[y][x]);
      }
    }
  } else if (flagged[y][x]) {
    drawFlag(px, py);
  } else {
    tft->fillRect(px, py, TILE_SIZE, TILE_SIZE, ST77XX_DARKGREY);
  }
  tft->drawRect(px, py, TILE_SIZE, TILE_SIZE, ST77XX_MIDGREY);
}

void drawMine(int x, int y) {  // Mine
  tft->fillRect(x, y, TILE_SIZE, TILE_SIZE, ST77XX_RED);
  tft->fillCircle(x + TILE_SIZE / 2, y + TILE_SIZE / 2, TILE_SIZE / 4, ST77XX_BLACK);
}

void drawFlag(int x, int y) {  // Flagge
  // Serial.print(F("Fahne bei (X,Y): "));
  // Serial.print(x);
  // Serial.print(F(", "));
  // Serial.println(y);
  tft->fillRect(x, y, TILE_SIZE, TILE_SIZE, ST77XX_LIGHTGREY);
  tft->fillRect(x + (TILE_SIZE / 4), y + (TILE_SIZE / 5), TILE_SIZE / 8, TILE_SIZE / 1.6, ST77XX_RED);  // Fahnenstange
  // Dreieck (Fahne)
  tft->fillTriangle(
    x + (TILE_SIZE / 3.2), y + (TILE_SIZE / 5),
    x + (TILE_SIZE / 1.6), y + (TILE_SIZE / 2.7),
    x + (TILE_SIZE / 3.2), y + (TILE_SIZE / 1.8),
    ST77XX_RED);
}

void drawField() {
  for (int y = 0; y < FIELD_HEIGHT; y++) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
      drawTile(x, y);
    }
  }

  tft->drawRect(cursorX * TILE_SIZE, cursorY * TILE_SIZE,
                TILE_SIZE, TILE_SIZE, ST77XX_GREEN);
}

void updateCursor() {
  // --- Alten Cursor entfernen ---
  if (oldCursorX >= 0 && oldCursorY >= 0) {
    drawTile(oldCursorX, oldCursorY);  // Original Feld wiederherstellen
  }

  // --- Neuen Cursor zeichnen ---
  tft->drawRect(cursorX * TILE_SIZE, cursorY * TILE_SIZE,
                TILE_SIZE, TILE_SIZE, ST77XX_GREEN);

  // --- Position für nächstes Mal merken ---
  oldCursorX = cursorX;
  oldCursorY = cursorY;
}

bool checkWin() {
  for (int y = 0; y < FIELD_HEIGHT; y++) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
      if (grid[y][x] != 9 && !revealed[y][x]) return false;
    }
  }
  return true;
}

////////////////////////////////////////
// Funktionen und Hauptroutine für Senso
////////////////////////////////////////

// Funktionsprototypen (Deklaration)
void zeichneFeld(byte value, bool activ);
void sensoInitGame();
void checkButtonPressed();
void isPlayerAction();
void playSequence();
void sensoHiscorePruefung();

void senso() {

  // Querformat durch Rotation 3

  LOGICAL_WIDTH = TFT_HEIGHT;  // 320
  LOGICAL_HEIGHT = TFT_WIDTH;  // 240

  // --- Layout-Konstanten ---

  BTN_SIZE = TFT_WIDTH / 4;
  BTN_RADIUS = BTN_SIZE / 2;

  CENTER_X = LOGICAL_WIDTH / 2;
  CENTER_Y = LOGICAL_HEIGHT / 2;

  OFFSET_X = TFT_WIDTH / 4;
  OFFSET_Y = TFT_WIDTH / 4;

  // Positionen der vier Felder (2×2 Raster)
  POS_ROT_X = CENTER_X + OFFSET_X;
  POS_ROT_Y = CENTER_Y - OFFSET_Y;

  POS_GELB_X = CENTER_X - OFFSET_X;
  POS_GELB_Y = CENTER_Y + OFFSET_Y;

  POS_GRUEN_X = CENTER_X - OFFSET_X;
  POS_GRUEN_Y = CENTER_Y - OFFSET_Y;

  POS_BLAU_X = CENTER_X + OFFSET_X;
  POS_BLAU_Y = CENTER_Y + OFFSET_Y;

  sensoTon[SENSO_FELD_ROT] = 4;    // Track-Nummer
  sensoTon[SENSO_FELD_GELB] = 2;   // Track-Nummer
  sensoTon[SENSO_FELD_GRUEN] = 3;  // Track-Nummer
  sensoTon[SENSO_FELD_BLAU] = 1;   // Track-Nummer
  bgColor = ST77XX_BLACK;
  tft->fillScreen(bgColor);
  for (int i = 0; i < 4; i++) {
    zeichneFeld(i, false);
  }
  sensoInitGame();
  sensoLoop();
  myDFPlayer.volume(loadVolumeFromLittleFS());
  if (sensoGameStatus != GAME_STATUS_CANCELED && sensoScore > 0) {
    sensoHiscorePruefung();
  }
}

void sensoLoop() {
  while (true) {
    checkButtonPressed();
    switch (sensoGameStatus) {
      case GAME_STATUS_RUNNING:
        if (isWaitingForPlayer) {
          isPlayerAction();
        } else {
          playSequence();
        }
        break;
      case GAME_STATUS_END:
        //if(pressedButton < SENSO_NO_PIN) {
        Serial.println(F("GAME_STATUS_END"));
        return;
        //}
        break;
      case GAME_STATUS_CANCELED:
        //if(pressedButton < SENSO_NO_PIN) {
        Serial.println(F("GAME_STATUS_CANCELED"));
        return;
        //}
        break;
    }
  }
  Serial.println(F("sensoLoop beendet"));
}

// Senso Feld zeichnen
void zeichneFeld(byte feld, bool activ) {

  const uint16_t colorRed = activ ? ST77XX_RED : ST77XX_DARKRED;
  const uint16_t colorYellow = activ ? ST77XX_YELLOW : ST77XX_DARKYELLOW;
  const uint16_t colorGreen = activ ? ST77XX_GREEN : ST77XX_DARKGREEN;
  const uint16_t colorBlue = activ ? ST77XX_BLUE : ST77XX_DARKBLUE;

  uint8_t TEXT_SIZE = 4;
  int8_t TEXT_OFFSET_X = -10;
  int8_t TEXT_OFFSET_Y = -14;

  switch (feld) {

    case SENSO_FELD_ROT:
      tft->fillRect(POS_ROT_X - BTN_RADIUS, POS_ROT_Y - BTN_RADIUS, BTN_SIZE, BTN_SIZE, colorRed);
      tft->setCursor(POS_ROT_X + TEXT_OFFSET_X, POS_ROT_Y + TEXT_OFFSET_Y);
      tft->setTextSize(TEXT_SIZE);
      tft->setTextColor(ST77XX_WHITE);
      tft->print(3);
      break;

    case SENSO_FELD_GELB:
      tft->fillRect(POS_GELB_X - BTN_RADIUS, POS_GELB_Y - BTN_RADIUS, BTN_SIZE, BTN_SIZE, colorYellow);
      tft->setCursor(POS_GELB_X + TEXT_OFFSET_X, POS_GELB_Y + TEXT_OFFSET_Y);
      tft->setTextSize(TEXT_SIZE);
      tft->setTextColor(ST77XX_WHITE);
      tft->print(7);
      break;

    case SENSO_FELD_GRUEN:
      tft->fillRect(POS_GRUEN_X - BTN_RADIUS, POS_GRUEN_Y - BTN_RADIUS, BTN_SIZE, BTN_SIZE, colorGreen);
      tft->setCursor(POS_GRUEN_X + TEXT_OFFSET_X, POS_GRUEN_Y + TEXT_OFFSET_Y);
      tft->setTextSize(TEXT_SIZE);
      tft->setTextColor(ST77XX_WHITE);
      tft->print(1);
      break;

    case SENSO_FELD_BLAU:
      tft->fillRect(POS_BLAU_X - BTN_RADIUS, POS_BLAU_Y - BTN_RADIUS, BTN_SIZE, BTN_SIZE, colorBlue);
      tft->setCursor(POS_BLAU_X + TEXT_OFFSET_X, POS_BLAU_Y + TEXT_OFFSET_Y);
      tft->setTextSize(TEXT_SIZE);
      tft->setTextColor(ST77XX_WHITE);
      tft->print(9);
      break;
  }
}

void sensoInitGame() {
  sensoScore = 0;
  sensoSequenceIndex = 0;
  lastPlayerAction = millis();
  memset(sensoSequence, SENSO_NO_PIN, sizeof(sensoSequence));
  isWaitingForPlayer = false;
  isColorPlayed = true;
  pressedButton = SENSO_NO_PIN;
  gameDelay = DELAY_INIT;
  sensoGameStatus = GAME_STATUS_RUNNING;

  randomSeed(micros() + analogRead(A0) + millis() + ESP.getCycleCount());
  sensoShowScore();

  myDFPlayer.reset();
  delay(800);
  myDFPlayer.play(21);
  delay(2500);
  myDFPlayer.volume(loadVolumeFromLittleFS() + 5);
  delay(200);
  myDFPlayer.stop();
  delay(200);
}

void playColor(byte feld, int delayTime) {
  myDFPlayer.play(sensoTon[feld]);
  delay(400);  //delay(490);
  zeichneFeld(feld, true);
  delay(delayTime);  //delay(480);
  myDFPlayer.stop();
  zeichneFeld(feld, false);
  //myDFPlayer.stop();
  delay(70);
}

void setGameOver() {
  sensoGameStatus = GAME_STATUS_END;
  tft->setCursor(smallDisplay ? 49 : 98,
                 smallDisplay ? 58 : 116);

  tft->setTextSize(smallDisplay ? 1 : 2);
  tft->setTextColor(ST77XX_WHITE, bgColor);
  tft->print(F(" Game Over "));

  tft->setCursor(0, smallDisplay ? 120 : 240);

  tft->print(F("Score: "));
  tft->print(sensoScore);
  Serial.println(F("Game Over!"));
  Serial.print(F("Score: "));
  Serial.println(sensoScore);
  playColor(sensoSequence[sensoSequenceIndex], 500);
  playColor(sensoSequence[sensoSequenceIndex], 500);
  playColor(sensoSequence[sensoSequenceIndex], 500);
  delay(300);
  myDFPlayer.stop();
  delay(200);
  myDFPlayer.volume(loadVolumeFromLittleFS());
  delay(200);
  myDFPlayer.play(14);
  delay(5000);
}

void playSuccess() {
  myDFPlayer.play(19);
  delay(2000);
}

void sensoShowScore() {
  tft->setCursor(0, smallDisplay ? 120 : 220);
  tft->setTextSize(smallDisplay ? 1 : 2);
  tft->setTextColor(ST77XX_WHITE, bgColor);

  tft->print(F("Score: "));
  tft->print(sensoScore);

  tft->setCursor(smallDisplay ? 80 : 160,
                 smallDisplay ? 120 : 116);

  displayButtonAndText(
    smallDisplay ? 94 : 185,
    smallDisplay ? 119 : 220,
    "Abbruch", 'A');
}

void playSequence() {
  bool doBreak = false;
  for (byte i = 0; i < sizeof(sensoSequence); i++) {
    if (sensoSequence[i] == SENSO_NO_PIN) {
      sensoSequence[i] = random(4);
      doBreak = true;
    }
    playColor(sensoSequence[i], gameDelay);
    if (doBreak == true) {
      break;
    }
  }
  sensoSequenceIndex = 0;
  isWaitingForPlayer = true;
  lastPlayerAction = millis();
}

void isPlayerAction() {
  // player did the whole sensoSequence
  if (sensoSequence[sensoSequenceIndex] == SENSO_NO_PIN) {
    isWaitingForPlayer = false;
    sensoScore++;
    sensoShowScore();
    if (gameDelay > DELAY_MIN && gameDelay > DELAY_STEP) {
      gameDelay -= DELAY_STEP;
    }
    pressedButton = SENSO_NO_PIN;
    playSuccess();
    return;
  }

  // no button pressed wait until timeout
  if (pressedButton == SENSO_NO_PIN) {
    if ((millis() - lastPlayerAction) > BUTTON_MAX_INTERVAL) {
      setGameOver();
    }
    return;
  }

  // repeat color the player pressed
  if (!isColorPlayed) {
    //playColor(pressedButton, gameDelay);
    playColor(pressedButton, 600);
    isColorPlayed = true;
  }

  // check current color
  if (sensoSequence[sensoSequenceIndex] == pressedButton) {
    sensoSequenceIndex++;
  } else {
    setGameOver();
  }
  pressedButton = SENSO_NO_PIN;
}

void checkButtonPressed() {
  char eingabe = scanKeypad();
  if (eingabe >= '0' && eingabe <= '9') {
    if (eingabe == '1') {
      pressedButton = SENSO_FELD_GRUEN;
      isColorPlayed = false;
      lastPlayerAction = millis();
      lastButtonPressed = millis();
      return;
    }
    if (eingabe == '3') {
      pressedButton = SENSO_FELD_ROT;
      isColorPlayed = false;
      lastPlayerAction = millis();
      lastButtonPressed = millis();
      return;
    }
    if (eingabe == '7') {
      pressedButton = SENSO_FELD_GELB;
      isColorPlayed = false;
      lastPlayerAction = millis();
      lastButtonPressed = millis();
      return;
    }
    if (eingabe == '9') {
      pressedButton = SENSO_FELD_BLAU;
      isColorPlayed = false;
      lastPlayerAction = millis();
      lastButtonPressed = millis();
      return;
    }
    //delay(300);

  } else if (eingabe == 'A') {
    pressedButton = SENSO_NO_PIN;
    sensoGameStatus = GAME_STATUS_CANCELED;
    Serial.println(F("Abgebrochen"));
    return;
  }
}

void sensoHiscorePruefung() {
  Serial.print(F("hiscore überprüfung und speicherung"));
  hiScore("Senso", sensoScore, 0, "");
}

////////////////////////////////////////
// Funktionen und Hauptroutine für Snake
////////////////////////////////////////

void snake(){
  randomSeed(micros() + analogRead(A0) + millis() + ESP.getCycleCount());

  bgColor = ST77XX_BLACK;
  iconColor = ST77XX_WHITE;
  textColor = ST77XX_WHITE;

  tft->fillScreen(bgColor);
  tft->setTextColor(textColor);

  gameControllInfo("Snake", "", "", false);

  delay(5000);

  tft->fillScreen(bgColor);

  drawSnakeField();
  snakeLoop();
  // Spiel fertig. Zurück ins Menu
}

void drawSnakeField() {
  tft->setTextColor(textColor);
  tft->setTextSize(smallDisplay ? 1 : 2);
  tft->setCursor(1, smallDisplay ? 119 : 223);
  tft->print(F("Score: 0"));
  byte f = smallDisplay ? 5 : 10;
  uint16_t x = gridx * f + 2 + f;
  uint16_t y = gridy * f + 2 + f;
  tft->drawRect(0,0,x,y,textColor);
}

void snakeGameOver(int score){
  myDFPlayer.play(15);
  for(int y=0;y<gridy;y++){
    for(int x=0;x<gridx;x++){
      snakeGrid[y][x]=0;
    }
  }
  tft->setTextColor(textColor, bgColor);
  tft->setTextSize(smallDisplay ? 1 : 2);
  tft->setCursor(10, smallDisplay ? 56 : 104);
  tft->print(F("Game Over"));
  printSnakeScore(score);
  delay(5000);
  hiScore("Snake", score, 0, "");
}

bool snakeLoop() {
  memset(snakeBuffer, 0, sizeof(snakeBuffer));
  memset(snakeGrid, 0, sizeof(snakeGrid));
  long addpos = 1;
  long delpos = 0;
  byte arrow = 2;
  byte lastarrow = 1;
  int score = 0;
  int16_t newPos[2];
  int16_t oldPos[2];
  bool gendot = true;
  bool incaxys = false;
  int8_t increment = 1;

  newPos[0] = gridx / 2;
  newPos[1] = gridy / 2;
  oldPos[0] = newPos[0];
  oldPos[1] = newPos[1];

  char key;
  char lastKey = '\0';
  bool keyReleased = true;
  bool snakeRunning = true;

  unsigned long lastMove = millis();

  while (snakeRunning) {

    // --- NON-BLOCKING MOVEMENT ---
    unsigned long now = millis();
    if (now - lastMove >= velocity) {
      lastMove = now;

      if((arrow!=0)&&(((lastarrow+2!=arrow)&&(lastarrow-2!=arrow))||(score==0))){
        incaxys = (arrow % 2) != 0;
        increment = ((arrow > 1) && (arrow < 4)) ? 1 : -1;
        lastarrow = arrow;
      }
      arrow = 0;

      newPos[incaxys] += increment;

      if ((newPos[0] < 0) || (newPos[0] > gridx) || (newPos[1] < 0) || (newPos[1] > gridy)) {
        snakeGameOver(score);
        snakeRunning = false;
      } else {
        snakeBuffer[addpos][0] = newPos[0];
        snakeBuffer[addpos][1] = newPos[1];
        addpos++;
        if (addpos > (snakesize - 1)) addpos = 0;

        snakeGrid[newPos[1]][newPos[0]]++;
        snakeGrid[oldPos[1]][oldPos[0]] = 0;

        switch (snakeGrid[newPos[1]][newPos[0]]) {
          case 2:
            snakeGameOver(score);
            snakeRunning = false;
            break;

          case 4:
            score += 10;
            myDFPlayer.play(6);
            printSnakeScore(score);
            gendot = true;
            if ((score / 10) < snakesize) delpos--;
            // fallthrough

          default:
            updateSnake(newPos, oldPos);
            delpos++;
            if (delpos > (snakesize - 1)) delpos = 0;
            oldPos[0] = snakeBuffer[delpos][0];
            oldPos[1] = snakeBuffer[delpos][1];
            break;
        }
      }

      // DOT GENERATION
      while (gendot) {
        byte posy = random(gridy - 2) + 1;
        byte posx = random(gridx - 2) + 1;
        if (snakeGrid[posy][posx] == 0) {
          byte f = smallDisplay ? 5 : 10;
          tft->fillRect(posx * f + 1, posy * f + 1, f, f, ST77XX_RED);
          snakeGrid[posy][posx] = 3;
          gendot = false;
        }
      }
    }

    autoPowerOff();
    key = scanKeypad();

    if (key != '\0' && key != lastKey && keyReleased) {
      if (key == 'A') return false;

      if (key == '4') arrow = 4;
      else if (key == '6') arrow = 2;
      else if (key == '2') arrow = 1;
      else if (key == '8') arrow = 3;

      lastKey = key;
      keyReleased = false;
    }

    if (key == '\0' || key == ' ') {
      keyReleased = true;
      lastKey = '\0';
    }
  }
  return false;
}

void printSnakeScore(int score){
  tft->setTextSize(smallDisplay ? 1 : 2);
  tft->setTextColor(ST77XX_WHITE, bgColor);
  tft->setCursor(smallDisplay ? 41 : 81, smallDisplay ? 119 : 223);
  tft->print(score);
}

void updateSnake(int16_t newPos[], int16_t oldPos[]){
  byte f = smallDisplay ? 5 : 10;
  tft->fillRect(oldPos[0]*f+1,oldPos[1]*f+1,f,f,bgColor);
  tft->fillRect(newPos[0]*f+1,newPos[1]*f+1,f,f,textColor);
}
