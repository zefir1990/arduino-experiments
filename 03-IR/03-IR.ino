#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <IRremote.hpp>

#define TFT_CS    10
#define TFT_RST    8
#define TFT_DC     9
#define TFT_SCLK  13
#define TFT_MOSI  11

#define IR_PIN     2

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(5, 5);
  tft.println("IR Receiver");

  Serial.begin(115200);

  // Start IR receiver
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  tft.setTextSize(1);
  tft.setCursor(5, 30);
  tft.println("Waiting...");
}

void loop() {

  if (IrReceiver.decode()) {

    // Clear area for new message
    tft.fillRect(0, 30, 160, 98, ST77XX_BLACK);

    tft.setCursor(5, 30);
    tft.setTextColor(ST77XX_GREEN);
    tft.print("Protocol: ");
    tft.println(getProtocolString(IrReceiver.decodedIRData.protocol));

    tft.setCursor(5, 45);
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("Address: 0x");
    tft.println(IrReceiver.decodedIRData.address, HEX);

    tft.setCursor(5, 60);
    tft.setTextColor(ST77XX_CYAN);
    tft.print("Command: 0x");
    tft.println(IrReceiver.decodedIRData.command, HEX);

    tft.setCursor(5, 75);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Raw: 0x");
    tft.println(IrReceiver.decodedIRData.decodedRawData, HEX);

    Serial.print("Protocol: ");
    Serial.println(getProtocolString(IrReceiver.decodedIRData.protocol));

    Serial.print("Address: 0x");
    Serial.println(IrReceiver.decodedIRData.address, HEX);

    Serial.print("Command: 0x");
    Serial.println(IrReceiver.decodedIRData.command, HEX);

    Serial.print("Raw: 0x");
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);

    // Ready for next code
    IrReceiver.resume();
  }
}