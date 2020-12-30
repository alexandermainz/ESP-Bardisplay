/*
 * web_Fetch.h
 * Funktion, um eine JPEG-Bilddatei von einer http-URL zu laden.
 * Basiert auf https://github.com/Bodmer/TJpg_Decoder/blob/master/examples/Web_Jpg/Web_Fetch.h
 * 
 * Parameter:
 * - url: String mit der Adresse des zu ladenden Bildes (nur http-Protokoll erlaubt)
 * Rückgabe;
 * - true (1) bei Erfolg, sinst false (0)
 * 
 * Die Datei wird in das RAM des Microcontrollers geladen, da der Flash-Speicher
 *  nur eine begrenzte Anzahl von Schreibzyklen durchhält und deshalb für ein ständiges
 *  neu beschreiben wie hier benötigt nicht infrage kommt.
 *  Deshalb ist die maximale Dateigröße durch das Controller-RAM beschränkt, im ESP8266
 *  auf 35kB.
 *  Kommt ein Controller mit zusätzlichem RAM zum Einsatz, z.B. ein ESP32 mit PSRAM, kann
 *  die Bilddatei dort hinein geladen werden und entsprechend größer ausfallen. Limitierender
 *  Faktor ist dann der für das Decodieren des JPEG-Formats benötigte RAM-Speicher.
 */
 
// Alloziere einen Puffer für die Bilddatei
uint8_t pImage[35840] = { 0 }; // max. Dateigröße 35kB !!!
unsigned long imageSize = 0;

// Lese die Datei von der übergebenen URL und speichere sie im RAM (Pointer pImage)
bool getFile(String url) {
#ifdef DEBUG
  Serial.println("Download von " + url);
#endif
  // nur bei gültiger WiFi-Verbindung download starten
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    http.begin(url);
    // Starte den Dateiabruf mittels HTTP GET
    int httpCode = http.GET();
#ifdef DEBUG
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
#endif
    // Wenn Datei gefunden, weitermachen
    if (httpCode == HTTP_CODE_OK) {
      int total = http.getSize();
      int len = total;

      // Lesepuffer erstellen
      uint8_t buff[128] = { 0 };
      imageSize = 0;
      
      // Dateistream abholen
      WiFiClient * stream = http.getStreamPtr();

      // Daten vom Server blockweise lesen
      while (http.connected() && (len > 0 || len == -1)) {
        // Anzahl verfügbarer Bytes im Dateistream holen
        size_t size = stream->available();

        if (size) {
          // max. 128 bytes in den Blockpuffer lesen
          int c;
          c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

          // gelesene Bytes im RAM ablegen
          memcpy((pImage+imageSize), buff, c);
          imageSize += c;
          
          // Restlänge berechnen
          if (len > 0) {
            len -= c;
          }
        }
        yield();
      } // while
    }
    else {
      Serial.printf("[HTTP] GET... fehlgeschlagen, Fehler: %s\n", http.errorToString(httpCode).c_str());
      http.end();
      return 0;
    }
    http.end();
  }
  else {
    Serial.println("Fehler: Keine WiFi-Verbindung verfuegbar");
    return 0;
  }
  return 1;
}
