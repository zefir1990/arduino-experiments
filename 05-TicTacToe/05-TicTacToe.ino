#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <IRremote.hpp>

#define TFT_CS    10
#define TFT_RST   8
#define TFT_DC    9
#define IR_PIN    2

Adafruit_ST7735 tft(TFT_CS, TFT_DC, 11, 13, TFT_RST);

#define CELL_SIZE 40

uint8_t board[9] = {0}; // 0 empty, 1 X, 2 O
uint8_t player = 1;

uint8_t buttonMap[9] = {
  0x45, // 1
  0x46, // 2
  0x47, // 3
  0x44, // 4
  0x40, // 5
  0x43, // 6
  0x7, // 7
  0x15, // 8
  0x9  // 9
};


void drawBoard() {
  tft.fillScreen(ST77XX_BLACK);

  // grid
  for (int i = 1; i < 3; i++) {
    tft.drawLine(i * CELL_SIZE, 0,
                 i * CELL_SIZE, 120,
                 ST77XX_WHITE);

    tft.drawLine(0, i * CELL_SIZE,
                 120, i * CELL_SIZE,
                 ST77XX_WHITE);
  }

  // symbols
  for (int i = 0; i < 9; i++) {

    int x = (i % 3) * CELL_SIZE + CELL_SIZE / 2;
    int y = (i / 3) * CELL_SIZE + CELL_SIZE / 2;

    if (board[i] == 1) {
      tft.drawLine(x - 12, y - 12,
                   x + 12, y + 12,
                   ST77XX_RED);

      tft.drawLine(x + 12, y - 12,
                   x - 12, y + 12,
                   ST77XX_RED);
    }

    if (board[i] == 2) {
      tft.drawCircle(x, y, 14,
                     ST77XX_BLUE);
    }
  }
}


bool checkWin(uint8_t p) {

  uint8_t wins[8][3] = {
    {0,1,2},
    {3,4,5},
    {6,7,8},

    {0,3,6},
    {1,4,7},
    {2,5,8},

    {0,4,8},
    {2,4,6}
  };


  for (int i = 0; i < 8; i++) {

    if (board[wins[i][0]] == p &&
        board[wins[i][1]] == p &&
        board[wins[i][2]] == p) {

      return true;
    }
  }

  return false;
}


bool isDraw() {

  for (int i = 0; i < 9; i++) {
    if (board[i] == 0)
      return false;
  }

  return true;
}


void resetGame() {

  for (int i = 0; i < 9; i++)
    board[i] = 0;

  player = 1;
  drawBoard();
}


void makeMove(uint8_t cell) {

  if (board[cell] != 0)
    return;


  board[cell] = player;

  drawBoard();


  if (checkWin(player)) {

    delay(2000);
    resetGame();
    return;
  }


  if (isDraw()) {

    delay(2000);
    resetGame();
    return;
  }


  player = (player == 1) ? 2 : 1;
}


void setup() {

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  IrReceiver.begin(IR_PIN, DISABLE_LED_FEEDBACK);

  resetGame();
}


void loop() {

  if (!IrReceiver.decode())
    return;


  if (IrReceiver.decodedIRData.protocol == NEC) {

    if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {

      uint8_t cmd = IrReceiver.decodedIRData.command;

      for (int i = 0; i < 9; i++) {

        if (cmd == buttonMap[i]) {
          makeMove(i);
          break;
        }
      }
    }
  }


  IrReceiver.resume();
}