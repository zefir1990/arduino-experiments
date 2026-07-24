#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <IRremote.hpp>

#define TFT_CS    10
#define TFT_RST    8
#define TFT_DC     9
#define IR_PIN     2

Adafruit_ST7735 tft(TFT_CS, TFT_DC, 11, 13, TFT_RST);

#define STEP 5

uint8_t x = 0;
uint8_t y = 0;
uint8_t lastCommand = 0;

void drawBall() {
  tft.fillCircle(x, y, STEP, ST77XX_RED);
}

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  drawBall();

  IrReceiver.begin(IR_PIN, DISABLE_LED_FEEDBACK);
}

void loop() {

  if (!IrReceiver.decode()) {
    return;
  }

  if (IrReceiver.decodedIRData.protocol != NEC) {
    IrReceiver.resume();
    return;
  }

  if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
    lastCommand = IrReceiver.decodedIRData.command;
  }

  switch (lastCommand) {
    case 0x5A:
      x += STEP;
      break;

    case 0x08:
      x -= STEP;
      break;

    case 0x18:
      y -= STEP;
      break;

    case 0x52:
      y += STEP;
      break;
  }

  drawBall();

  IrReceiver.resume();
}