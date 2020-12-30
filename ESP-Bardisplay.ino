//
// ESP-Bardisplay
//
// TFT-Display fuer die Bar, zeigt verschiedene Infos und die Barrechnung des Gastes an.
//
// ESP8266 D1 Mini mit ILI9488 3.5' 480x320 TFT oder ILI9341 2.4' 240x320 TFT.
// Webserver, der Endpunkte für die Kasse/Bar-Anwendung bereitstellt, um Texte und Bilder
//  darstellen zu können.
//
// (C) Alexander Schmitt, www.net-things.de
//
//
////////////////////////////////////////////////////////////////////////////////////////

//#define DEBUG 1
String VERSION = "1.0";

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "Web_Fetch.h"

// hier SSID und WPA2-Kennwort des WiFi eintragen:
#define STASSID "your-ssid"
#define STAPSK  "your-password"

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer webserver(80);
TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();

  // initialisiere TFT-Display
  tft.begin();
  // Rotiere die Anzeige im TFT-Display um 90 Grad -> Querformat
  tft.setRotation(1);
  delay(10);
  tft.fillScreen(TFT_RED);
  delay(1000);
  tft.fillScreen(TFT_GREEN);
  delay(1000);
  tft.fillScreen(TFT_BLUE);
  delay(1000);
  tft.fillScreen(TFT_BLACK);
  // Standardwerte setzen (Beispiel)
  // Textfarbe weiss auf schwarzem Hintergrund
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Textausgabe mit Drawstring zentrieren
  tft.setTextDatum(TC_DATUM);
  // Textgröße doppelt
  tft.setTextSize(2);
  
  tft.drawString("Bardisplay", 160, 40);
  tft.drawString("Willkommen", 1, 80);
  delay(100);

  // initialisiere Jpeg-Bildanzeige-Bibliothek
  // setze Skalierung auf 1 (erlaubte Werte sind 1, 2, 4, or 8)
  TJpgDec.setJpgScale(1);
  // für TFT_eSPI-Bibliothek muss die Bytereihenfolge unmgedreht werden
  TJpgDec.setSwapBytes(true);
  // rendering-Funktion für die Bilduasgabe angeben
  TJpgDec.setCallback(tft_output);

  // WiFi verbinden
  initWiFi();
  tft.drawString("WiFi verbunden!", 1, 120);
  IPAddress ip = WiFi.localIP();
  char buf[20];
  sprintf(buf, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  tft.drawString(buf, 1, 160);

  // Endpunkte des Webservers definieren
  webserver.on("/", handleRoot);
  webserver.on("/version", handleVersion);
  webserver.on("/showText", handleShowText);
  webserver.on("/showImage", handleShowImage);
  webserver.on("/clearScreen", handleClearScreen);
  webserver.on("/off", handleOff);
  webserver.on("/on", handleOn);
  webserver.on("/reboot", handleReboot);
  webserver.onNotFound(handleNotFound);
  webserver.begin();
  Serial.println("HTTP-Server gestartet");

  tft.drawString("Setup fertig!", 1, 0);
  Serial.println("Setup complete!");
  delay(100);
}

void loop() {
  webserver.handleClient();
}

void initWiFi() {
  Serial.print("versuche WiFi zu verbinden, SSID: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  int tryout = 0;
  while (WiFi.status() != WL_CONNECTED && tryout++ < 3) {
    Serial.print(".");
    Serial.println(WiFi.begin(ssid, password));
    int wait = 0;
    while (WiFi.status() != WL_CONNECTED && wait++ < 12) {
      delay(500);
      Serial.print(".");
    }
    
    delay(5000);
  }
  if (WiFi.status() != WL_CONNECTED)  // den Controller neu starten, wenn keine WiFi-Verbindung zustande kam
    ESP.restart();
    
  Serial.print("verbunden mit ");
  Serial.println(ssid);  
  Serial.println(WiFi.localIP());
}

/*
 * Endpunkt "/" - Ausgabe einer simplen Statusmeldung.
 */
void handleRoot() {
  digitalWrite(LED_BUILTIN, 0);
  webserver.send(200, "text/html", "<html><body><h2>Das ESP8266 Bardisplay läuft</h2></body></html>");
  digitalWrite(LED_BUILTIN, 1);
}

/*
 * Endpunkt "/version" - Gibt die Skriptversion, wie oben unter VERSION definiert, zurück.
 */
void handleVersion() {
  digitalWrite(LED_BUILTIN, 0);
  webserver.send(200, "text/html", "<html><body><h2>Bardisplay Skriptversion ist " + VERSION + "</h2></body></html>");
  digitalWrite(LED_BUILTIN, 1);
}

/*
 * Hilfsfunktion zum Anzeigen von (farbigem) Text auf schwarzem Hintergrund
 */
void drawText(String s, uint8_t x, uint8_t y, uint8_t fontsize, uint32_t color, bool centered, bool clear) {
  if (clear)
    tft.fillScreen(TFT_BLACK);
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextSize(fontsize);

  if (centered) {
    tft.drawString(s, x, y);
  }
  else {
    tft.setCursor(x,y);
    tft.println(s);
  }
}

/*
 * Endpunkt /showText
 * Textstring aus dem TFT anzeigen.
 * Parameter:
 * - text: Auszugebender Textstring, URL-encodiert zu übergeben,
 * - size: Textgröße, numerisch (1 = Originalgröße des Fonts, 2 = doppelte Größe usw.), optional,
 * - x: X-Koordinate, ab der der Text ausgegeben werden soll, optional,
 * - y: Y-Koordinate, ab der der Text ausgegeben werden soll, optional,
 * - color: Nummer der Farbe, in welcher der Text ausgegeben werden soll (siehe Section 6 in TFT_eSPI.h), optional,
 * - centered: "true", wenn der Text zentriert ausgegeben werden soll, optional,
 * - clear: "true", wenn das Display vor der Ausgabe gelöscht werden soll, optional.
 * 
 * Bei zentriert auszugebendem Text bezeichnet die X-Koordinate die gewünschte Mitte des Strings,
 *  bei nicht-zentrierter Ausgabe die linke Kante.
 * 
 * Beispielaufruf:
 *  http://mein-esp/showText?text=Ein%20Test&size=2&x=120&y=10&color=15&centered=true&clear=true
 */
void handleShowText() {
  digitalWrite(LED_BUILTIN, 0);
  if (webserver.arg("text") != "") {
    unsigned short fontsize = 1;
    unsigned short startX = 1;
    unsigned short startY = 19;
    uint32_t color = TFT_WHITE;
    
    if (webserver.arg("size") != "") {
      fontsize = webserver.arg("size").toInt();
      startY += 5*fontsize;
    }
    if (webserver.arg("x") != "") {
      startX = webserver.arg("x").toInt();
    }
    if (webserver.arg("y") != "") {
      startY = webserver.arg("y").toInt();
    }
    if (webserver.arg("color") != "") {
      color = webserver.arg("color").toInt();
    }
    String s = webserver.arg("text");
    drawText(s, startX, startY, fontsize, color, (webserver.arg("centered")=="true"), (webserver.arg("clear")=="true"));
    webserver.send(200, "text/html", "OK");
  }
  else
    webserver.send(400);
  digitalWrite(LED_BUILTIN, 1);
}

/*
 * Hilfsfunktion zum Laden und Anzeigen eines JPG-Bildes
 */
void drawJpg(String url, uint8_t x, uint8_t y) {
  bool loaded_ok = getFile(url);
#ifdef DEBUG
  Serial.print("loadedOK: "); Serial.println(loaded_ok);
  Serial.println((int)pImage); Serial.println(imageSize);
#endif
  if (loaded_ok)
    TJpgDec.drawJpg(x, y, pImage, imageSize);
  else {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(x,y);
    tft.println("Fehler beim Laden des Bildes\r\n" + url);
  }
}

/*
 * Endpunkt /showImage
 * 
 * Bildgrafik anzeigen
 * - nur http-Protokoll wird unterstützt
 * - nur Standard-JPG-Dateiformat wird unterstützt
 * 
 * Parameter:
 * - url: Adresse des zu ladenden Bildes (lokal oder Internet),
 * - x: X-Koordinate der linken oberen Bildecke auf dem Display (optional),
 * - y: Y-Koordinate der linken oberen Bildecke auf dem Display (optional).
 * 
 * Beispielaufruf:
 *  http://mein-esp/showImage?url=http://demo.net-things.de/ttig.jpg
 */
void handleShowImage() {
  uint8_t x=0; uint8_t y=0;
  digitalWrite(LED_BUILTIN, 0);
  if (webserver.arg("url") != "") {
    if (webserver.arg("x") != "") x = webserver.arg("x").toInt();;
    if (webserver.arg("y") != "") y = webserver.arg("y").toInt();;
    String s = webserver.arg("url");
    drawJpg(s, x, y);
    webserver.send(200, "text/html", "OK");
  }
  else
    webserver.send(400);
  digitalWrite(LED_BUILTIN, 1);
}

/*
 * Endpunkt /clearScreen - "Löscht" das Display, indem alle Pixel auf schwarz gesetzt werden.
 */
void handleClearScreen() {
  digitalWrite(LED_BUILTIN, 0);
  tft.fillScreen(TFT_BLACK);
  webserver.send(200, "text/html", "OK");
  digitalWrite(LED_BUILTIN, 1);
}

/*
 * Endpunkt /on - Schaltet die LED-Hintergrundbeleuchtung des Displays an
 *  und weckt das Display aus dem Standby (nur für ILI9488-Displays).
 */
void handleOn() {
  digitalWrite(LED_BUILTIN, 0);
  displayOn();
  digitalWrite(TFT_BL, HIGH);
  webserver.send(200, "text/html", "OK");
  digitalWrite(LED_BUILTIN, 1);
}

/*
 * Endpunkt /off - Schaltet die LED-Hintergrundbeleuchtung des Displays aus
 *  und schaltet das Display in den Standby (nur für ILI9488-Displays).
 */
void handleOff() {
  digitalWrite(LED_BUILTIN, 0);
  digitalWrite(TFT_BL, LOW);
  displayOff();
  webserver.send(200, "text/html", "OK");
  digitalWrite(LED_BUILTIN, 1);
}

/*
 * Endpunkt /reboot - Startet den Controller neu.
 */
void handleReboot() {
  webserver.send(200, "text/html", "ESP startet jetzt neu...");
  delay(2000);
  ESP.restart();
}

/*
 * Generischer Handler für nicht-definierte Endpunkte, gibt HTTP-Status 404 zurück.
 */
void handleNotFound() {
  digitalWrite(LED_BUILTIN, 0);
  String message = "Seite nicht gefunden\n\n";
  message += "URI: ";
  message += webserver.uri();
  message += "\nMethode: ";
  message += (webserver.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArgumente: ";
  message += webserver.args();
  message += "\n";

  for (uint8_t i = 0; i < webserver.args(); i++) {
    message += " " + webserver.argName(i) + ": " + webserver.arg(i) + "\n";
  }

  webserver.send(404, "text/plain", message);
  digitalWrite(LED_BUILTIN, 1);
}

// Diese callback-Funktion wird während des Decodierens eines JPG-Bildes
//  durch die TJpg-Bibliothek für jeden gelesenen Block des Bildes aufgerufen
//  und schreibt den Block auf das TFT-Display
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  // Wenn Bild den unteren Rand des Displays erreicht hat, Signal zum stoppen zurückgeben
  if ( y >= tft.height() ) return 0;

  // Bildblock an das TFT übergeben
  tft.pushImage(x, y, w, h, bitmap);

  // Gebe 1 zurück, um TJpg zu signalisieren, mit dem nächsten Block weiterzumachen
  return 1;
}

/*
 * Hilfsfunktion für ILI9488-basierte TFT-Displays, um Display aus dem Standby/Off zu wecken.
 */
void displayOn()
{
#ifdef ILI9488_DRIVER
  tft.writecommand(0x29);
  delay(10);
  tft.writecommand(0x11);
  delay(10);
#endif
}

/*
 * Hilfsfunktion für ILI9488-basierte TFT-Displays, um Display "abzuschalten" (eine Art Standby-Modus).
 */
void displayOff()
{
#ifdef ILI9488_DRIVER
  tft.writecommand(0x28);
  delay(10);
  tft.writecommand(0x10);
  delay(10);
#endif
}
