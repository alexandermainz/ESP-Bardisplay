# ESP-Bardisplay

Arduino/PlatformIO sketch for ESP8266 web service based TFT display.

This sketch implements a web service on an ESP8266 based microcontroller for controlling a connected TFT display.
The web service exposes methods for displaying arbitrary text and JPEG images.
It is a part extracted from a bigger project, where this setup is being used as a customer information screen at the bar of an association's premises.

Detailed information in german language can be found at https://net-things.de/index.php/blog/text-und-bild-display-mit-esp8266-webserver.

#### Hardware:

* Wemos D1 Mini or similar with ESP8266
* 3.5'' TFT-Display with SPI interface and ILI9488-Chipsatz or compatible

#### Software:

* Arduino or PlatformIO IDE with installed ESP8266 support
* TFT_eSPI library from Bodmer (https://github.com/Bodmer/TFT_eSPI)
* TJpeg_Decoder library from Bodmer (https://github.com/Bodmer/TJpg_Decoder)
* this sketch

### Pin assignment

| TFT Pin |	ESP8266 Pin |
| ------- | ----------- |
| VCC |	5V |
| GND |	GND |
| CS |	D2 |
| RESET |	RST |
| DC/RS |	D4 |
| MOSI | D7 |
| SCK |	D5 |
| LED |	D1 |

### Software configuration

* set your WiFi access point SSID and password in `ESP-Bardisplay.ino` lines 25 & 26
* setup the type of your display and your pin assigment in `User_Setup.h` of the TFT-eSPI library according to the lib's documentation

### Web service endpoints

**`/showText`:**
Show a text string on the display.   
Parameters:
* `text`: the string to display, must be URL encoded, multiline possible using "\r\n",
* `size`: text size, numeric value (1 = original size of the used font, 2 = double size, ...), optional
* `x`: horizontal pixel position for the text to start, source is upper left, optional,
* `y`: vertical pixel position for the text to start, optional,
* `color`: text color number, optional, must be a decimal integer value according to the TFT color table given in `TFT_eSPI.h`
* `centered`: assign `true`, if the text should be rendered centered, optional,
* `clear`: assign `true` if the screen should be cleared (painted black) before printing the text, optional.

When centered output is selected, the x position will mark the desired mid of the text string, otherwise x indicates the left margin of the string.

Example use:   

    http://192.168.1.2/showText?text=Ein%20Teststring&size=2&x=120&y=10&color=15&centered=true&clear=true
  
**`/showImage`:**
Show a JPEG image on the display.   
Parameters:
* `url`: location of the image file (local or remote), http only (no https support),
* `x`: horizontal pixel position for the upper left corner of the image, screen source is upper left, optional,
* `y`: vertical pixel position for the upper left corner of the image, optional.

The image size must not exceed the pixel size of your display. Max image file size is 35kB, since the ESP8266 does not have more RAM space. Since flash memory has a limited number of write cycles, using the flash memory for storing image data is not encouraged. Try to reduce file size by increasing the JPEG compression, if necessary.

Example use:

    http://192.168.1.2/showImage?url=http://demo.net-things.de/ttig.jpg
    
**`/version`:**
Show the VERSION of the sketch.

**`/clearScreen`:**
"Clear" the display by setting all pixels to black.

**`/off`:**
Switch display off. LED backlight is switched off, if display supports this via a LED pin which must be connected to D1 at the microcontroller. For IlI9488 based displays, a "soft off" command is also sent to the display, putting it into a sleep state with all pixels off and a reduced power consumption.

**`/on`:**
Switch display on. LED backlight is switched on, if display supports this via a LED pin which must be connected to D1 at the microcontroller. For IlI9488 based displays, a "wake up" command is also sent to the display, which will end a previously enabled "sleep mode".

**`/reboot:`**
Reboots the microcontroller.
