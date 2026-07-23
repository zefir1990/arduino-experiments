#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_CS    10
#define TFT_RST    8
#define TFT_DC     9
#define TFT_SCLK  13
#define TFT_MOSI  11

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  tft.fillScreen(ST77XX_BLACK);

  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("ARDUINO");

  tft.drawRect(0, 0, 160, 128, ST77XX_GREEN);
  tft.fillCircle(80, 64, 20, ST77XX_RED);
}

void loop() {
}