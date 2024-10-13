#include <ESP32Servo.h>
#include <WiFi.h>
#include "ESPNowW.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int connectionstate = 1;

struct controlpacket
{
  uint8_t ruder;
  uint8_t direction; // 0 stop, 1 forward, 2 reverse
  uint8_t speed;
};

controlpacket rcdata;

uint8_t receiver_mac[] = {0xb0, 0xb2, 0x1c, 0x0a, 0x04, 0x4c};

// Callback when data is sent
void onDataSent(const uint8_t *macAddress, esp_now_send_status_t sendStatus)
{
  if (sendStatus == ESP_NOW_SEND_SUCCESS)
  {
    connectionstate = 0;
  }
  else
  {
    connectionstate = 1;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  ESPNow.init();
  ESPNow.add_peer(receiver_mac);
  esp_now_register_send_cb(onDataSent);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
}

void loop()
{
  uint8_t vals[3];

  rcdata.ruder = map(analogRead(34),4095,0,0,255);

  // Geschwindigkeit und Richtung berechnen
  int w = 4096 - analogRead(35) - 2048;
  if (w > 2)
  {
    rcdata.direction = 1;
  }
  else if (w < -2)
  {
    rcdata.direction = 2;
  }
  else
  {
    rcdata.direction = 0;
  }

  if (w < 0)
  {
    w = w * -1;
  }
  rcdata.speed = map(w, 0, 2047, 0, 60);
  //rcdata.speed = (uint8_t)tanh( constrain(w,0,2047) / 1024 - 1 ) * 255;
  //Serial.println(rcdata.speed);
  //rcdata.speed = w;

  //ESPDaten füllen
  vals[0] = rcdata.ruder;
  vals[1] = rcdata.direction;
  vals[2] = rcdata.speed;

  ESPNow.send_message(receiver_mac, vals, sizeof(vals));

  // Werte auf Display schreiben
  display.clearDisplay();

  display.setTextSize(1);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);             // Start at top-left corner
  display.cp437(true);                 // Use full 256 char 'Code Page 437' font
  // Ruder
  display.write("Ruder: ");
  display.setCursor(50, 0);
  char cstr[16];
  itoa(rcdata.ruder, cstr, 10);
  display.write(cstr);

  // Motor
  display.setCursor(0, 10);
  display.write("Motor:");
  display.setCursor(50, 10);
  itoa(rcdata.speed, cstr, 10);
  display.write(cstr);

  // Richtung
  display.setCursor(0, 20);
  display.write("Dir:");
  display.setCursor(50, 20);
  itoa(rcdata.direction, cstr, 10);
  display.write(cstr);

  // Verbindungsstatus
  display.setCursor(0, 50);
  if (connectionstate == 0)
  {
    display.write("Verbunden");
  }
  else
  {
    display.write("Getrennt");
  }

  display.display();
  delay(100);
}
