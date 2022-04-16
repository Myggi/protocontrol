#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <stdint.h>

//#define FASTLED_ESP32_I2S

#include <FastLED.h>
// #include "AsyncJson.h"
// #include "ArduinoJson.h"
// #include "wifi_credentials.h"
#include "facestorage.h"
// #include "webserver.h"
#include "matrix.h"
#include "matrixmanager.h"
// #include "max7219painter.h"
#include "displays/ws2812/ws2812display.h"
#include "displays/ws2812/ws2812matrix.h"
#include "displays/ws2812/ws2812string.h"

#define DELTA_E 0.5
#define FACE_COLS 16 + 16 + 8 + 8 + 32 + 32
#define FACE_ROWS 8
#define FBSIZE FACE_COLS
#define N_LAYERS 4

// Define the layout of our physical display
// The Strings need to be pointers to prevent slicing,
// since we treat them as their base class internally.
WS2812Display display {
    new WS2812StringPin<16> {
            WS2812Matrix(16, 8, 0),
            WS2812Matrix(32, 8, 0),
            WS2812Matrix(8, 8, 0),
    },
    new WS2812StringPin<17> {
            WS2812Matrix(16, 8, 2),
            WS2812Matrix(32, 8, 2),
            WS2812Matrix(8, 8, 2),
    }
};

// // Internal, hardware agnostic representation of the matrices
// // Basically just specialized bitmaps with some convenience functions
// // and animation stuff built in
MatrixManager matrixmanager;
Matrix eye_r("eye_r", 16, 8, 0, 0, N_LAYERS);
Matrix eye_l("eye_l", 16, 8, 56, 0, N_LAYERS);
Matrix nose_r("nose_r", 8, 8, 48, 0, N_LAYERS);
Matrix nose_l("nose_l", 8, 8, 104, 0, N_LAYERS);
Matrix mouth_r("mouth_r", 32, 8, 16, 0, N_LAYERS);
Matrix mouth_l("mouth_l", 32, 8, 72, 0, N_LAYERS);

// // The face that is actually drawn
// // TODO: refactor stuff that accesses this pointer
uint8_t *framebuffer;// = matrixmanager.data; (can't set here yet since it's still uninitialized)

// // Loaded from wifi_credentials.h
// extern const char *ssid;
// extern const char *password;

// // Manages access to SPI flash for loading and storing faces
FaceStorage facestorage(SPIFFS, FBSIZE);

// // WebServer wraps the async server for convenience and cleaner setup
// AsyncWebServer asyncserver(4000);
// WebServer server(asyncserver, facestorage, matrixmanager, FBSIZE, framebuffer);

// void setupWiFI()
// {
//   WiFi.begin(ssid, password);

//   if (WiFi.waitForConnectResult() != WL_CONNECTED)
//   {
//     Serial.println("Connection failed");
//     delay(1000);
//     return;
//   }

//   Serial.print("Connected: ");
//   Serial.println(WiFi.localIP());

//   delay(1000);
// }

void setup()
{
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  Serial.begin(115200);

  // Init Storage
  if (!SPIFFS.begin())
  {
    Serial.setTimeout(100000);
    Serial.println("Storage Mount Failed. Format? [y/n]");
    while(true) {
      if (Serial.available()) {
        char response = Serial.read();
        if (response == 'y') {
          SPIFFS.format();
          Serial.println("Storage Formatted");
          ESP.restart();
        } else {
          Serial.println("Rebooting...");
          ESP.restart();
        }
      }
    }
  }

  // Register & configure matrices
  matrixmanager.add(&eye_r);
  matrixmanager.add(&eye_l);
  matrixmanager.add(&nose_r);
  matrixmanager.add(&nose_l);
  matrixmanager.add(&mouth_r);
  matrixmanager.add(&mouth_l);
  matrixmanager.setMatrixBlink("eye_r", true);
  matrixmanager.setMatrixBlink("eye_l", true);
  matrixmanager.init(); // DO NOT FORGET omg
  framebuffer = matrixmanager.data;

  // Set the drawing positions of the matrices on the screen
  // painter.setNameMapping(matrixPositions);

  // setupWiFI();

  // Serial.println("Setting up server");
  // server.init(framebuffer);
  // Serial.println("Server setup complete");

  // Load default face
  Serial.println("Loading default face");
  int result = facestorage.loadface("boi", framebuffer);
  if (result != 0)
  {
    Serial.println("Failed loading default face");
  } else {
    Serial.println("Setting default face");
  }

  matrixmanager.setFrame(framebuffer);

  display.setBrightness(16);

  Serial.println("Setup complete");
}

uint32_t last = 0;
uint32_t last_blink = 0;
uint32_t blink_interval = 10000;
int blink_randomness;
bool blinking = false;
uint32_t last_debug_print = 0;
int last_dtime = 0;

uint cur_x = 0;
uint cur_y = 0;

void loop()
{
  // delay(16);

  // display.clear();
  // // display.drawPixel(cur_x, cur_y, 1);
  // display.drawFastVLine(cur_x, 0, 8, 1);
  // display.show();

  // cur_x += 1;
  // if (cur_x > 14*8) {
  //   cur_x = 0;
  //   cur_y += 1;
  // }

  // if (cur_y > 8) {
  //   cur_x = 0;
  //   cur_y = 0;
  // }

  // return;

  uint32_t now = millis();

  // // Handle overflow, update next loop
  if (now < last)
  {
    last = 0;
    return;
  }

  // Time delta in milliseconds
  uint32_t deltatime = now - last;
  last = now;

  // Handle blinking
  last_blink += deltatime;
  if (last_blink > blink_interval)
  {
    if (!blinking)
    {
      blinking = true;
      matrixmanager.blink();
    }
    if (blinking)
    {
      if (last_blink > blink_interval + 100)
      {
        last_blink = 0;
        blink_interval = random(7000, 13000);
        blinking = false;
        matrixmanager.clearBlink();
      }
    }
  }

  // Animate face
  matrixmanager.update(deltatime);

  display.clear();
  // display.drawPixel(cur_x, 0, 1);
  matrixmanager.draw(display);
  display.show();

  // Draw face to screen/matrices/whatever
  // matrixmanager.paint(painter);

  // // Debug prints
  // if (deltatime > last_dtime + 1 || deltatime < last_dtime - 1)
  // {
  //   Serial.print("dtime: ");
  //   Serial.println(deltatime);

  //   last_dtime = deltatime;
  // }
}