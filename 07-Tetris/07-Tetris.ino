#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <IRremote.hpp>
#include <avr/pgmspace.h>

#define TFT_CS    10
#define TFT_RST   8
#define TFT_DC    9
#define IR_PIN    2

Adafruit_ST7735 tft(TFT_CS, TFT_DC, 11, 13, TFT_RST);

#define CELL 5
#define GRID_WIDTH 10
#define GRID_HEIGHT 20
#define GRID_X ((128 - GRID_WIDTH * CELL) / 2)
#define GRID_Y ((160 - GRID_HEIGHT * CELL) / 2)
#define GRID_PIXEL_WIDTH (GRID_WIDTH * CELL)
#define GRID_PIXEL_HEIGHT (GRID_HEIGHT * CELL)
#define PIECE_COUNT 7

const uint8_t PIECE_SHAPES[PIECE_COUNT][4][4] PROGMEM = {
  {{0b0000, 0b1111, 0b0000, 0b0000},
   {0b0010, 0b0010, 0b0010, 0b0010},
   {0b0000, 0b0000, 0b1111, 0b0000},
   {0b0100, 0b0100, 0b0100, 0b0100}},
  {{0b0110, 0b0110, 0b0000, 0b0000},
   {0b0110, 0b0110, 0b0000, 0b0000},
   {0b0110, 0b0110, 0b0000, 0b0000},
   {0b0110, 0b0110, 0b0000, 0b0000}},
  {{0b0100, 0b1110, 0b0000, 0b0000},
   {0b0100, 0b0110, 0b0100, 0b0000},
   {0b0000, 0b1110, 0b0100, 0b0000},
   {0b0100, 0b1100, 0b0100, 0b0000}},
  {{0b0110, 0b1100, 0b0000, 0b0000},
   {0b0100, 0b0110, 0b0010, 0b0000},
   {0b0000, 0b0110, 0b1100, 0b0000},
   {0b1000, 0b1100, 0b0100, 0b0000}},
  {{0b1100, 0b0110, 0b0000, 0b0000},
   {0b0010, 0b0110, 0b0100, 0b0000},
   {0b0000, 0b1100, 0b0110, 0b0000},
   {0b0100, 0b1100, 0b1000, 0b0000}},
  {{0b1000, 0b1110, 0b0000, 0b0000},
   {0b0110, 0b0100, 0b0100, 0b0000},
   {0b0000, 0b1110, 0b0010, 0b0000},
   {0b0100, 0b0100, 0b1100, 0b0000}},
  {{0b0010, 0b1110, 0b0000, 0b0000},
   {0b0100, 0b0100, 0b0110, 0b0000},
   {0b0000, 0b1110, 0b1000, 0b0000},
   {0b1100, 0b0100, 0b0100, 0b0000}},
};

const uint16_t PIECE_COLORS[PIECE_COUNT] = {
  0x07FF, 0xFFE0, 0x780F, 0x07E0, 0xF800, 0x001F, 0xFD20
};

uint8_t grid[GRID_HEIGHT][GRID_WIDTH];
uint8_t currentPiece;
uint8_t currentRotation;
uint8_t nextPiece;
int8_t pieceX;
int8_t pieceY;
uint16_t score;
uint16_t clearedLines;
uint8_t level;
bool gameOver;
unsigned long lastFall;
unsigned long fallInterval;



bool hasBlock(uint8_t piece, uint8_t rotation, uint8_t row, uint8_t col)
{
  return pgm_read_byte(&PIECE_SHAPES[piece][rotation][row]) & (1 << (3 - col));
}



bool checkCollision(uint8_t piece, uint8_t rotation, int8_t x, int8_t y)
{
  for (uint8_t row = 0; row < 4; row++)
  {
    for (uint8_t col = 0; col < 4; col++)
    {
      if (hasBlock(piece, rotation, row, col))
      {
        int8_t gridX = x + col;
        int8_t gridY = y + row;

        if (gridX < 0 || gridX >= GRID_WIDTH || gridY >= GRID_HEIGHT)
        {
          return true;
        }

        if (gridY >= 0 && grid[gridY][gridX] != 0)
        {
          return true;
        }
      }
    }
  }

  return false;
}



void drawBlock(uint8_t gridX, uint8_t gridY, uint16_t color)
{
  tft.fillRect(
    GRID_X + gridX * CELL,
    GRID_Y + gridY * CELL,
    CELL,
    CELL,
    color
  );
}



void drawPieceCells(uint8_t piece, uint8_t rotation, int8_t x, int8_t y)
{
  for (uint8_t row = 0; row < 4; row++)
  {
    for (uint8_t col = 0; col < 4; col++)
    {
      if (hasBlock(piece, rotation, row, col))
      {
        int8_t drawX = x + col;
        int8_t drawY = y + row;

        if (drawX >= 0 && drawX < GRID_WIDTH && drawY >= 0 && drawY < GRID_HEIGHT)
        {
          drawBlock(drawX, drawY, PIECE_COLORS[piece]);
        }
      }
    }
  }
}



void clearPieceCells(uint8_t piece, uint8_t rotation, int8_t x, int8_t y)
{
  for (uint8_t row = 0; row < 4; row++)
  {
    for (uint8_t col = 0; col < 4; col++)
    {
      if (hasBlock(piece, rotation, row, col))
      {
        int8_t drawX = x + col;
        int8_t drawY = y + row;

        if (drawX >= 0 && drawX < GRID_WIDTH && drawY >= 0 && drawY < GRID_HEIGHT)
        {
          drawBlock(drawX, drawY, ST77XX_BLACK);
        }
      }
    }
  }
}



void drawBorder()
{
  tft.drawRect(
    GRID_X - 1,
    GRID_Y - 1,
    GRID_PIXEL_WIDTH + 2,
    GRID_PIXEL_HEIGHT + 2,
    ST77XX_WHITE
  );
}



void redrawFullField()
{
  tft.fillRect(
    GRID_X,
    GRID_Y,
    GRID_PIXEL_WIDTH,
    GRID_PIXEL_HEIGHT,
    ST77XX_BLACK
  );

  for (uint8_t row = 0; row < GRID_HEIGHT; row++)
  {
    for (uint8_t col = 0; col < GRID_WIDTH; col++)
    {
      if (grid[row][col] != 0)
      {
        drawBlock(col, row, PIECE_COLORS[grid[row][col] - 1]);
      }
    }
  }

  if (!gameOver)
  {
    drawPieceCells(currentPiece, currentRotation, pieceX, pieceY);
  }
}



void drawHUD()
{
  uint8_t previewX = GRID_X + GRID_PIXEL_WIDTH + 5;
  uint8_t previewY = GRID_Y + 5;

  for (uint8_t row = 0; row < 4; row++)
  {
    for (uint8_t col = 0; col < 4; col++)
    {
      uint8_t blockX = previewX + col * CELL;
      uint8_t blockY = previewY + row * CELL;

      if (hasBlock(nextPiece, 0, row, col))
      {
        tft.fillRect(blockX, blockY, CELL, CELL, PIECE_COLORS[nextPiece]);
      }
      else
      {
        tft.fillRect(blockX, blockY, CELL, CELL, ST77XX_BLACK);
      }
    }
  }
}



void showGameOver()
{
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);

  tft.setCursor(10, 40);
  tft.print("GAME OVER");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  tft.setCursor(15, 80);
  tft.print("SCORE: ");
  tft.print(score);

  tft.setCursor(15, 90);
  tft.print("LEVEL: ");
  tft.print(level);

  tft.setCursor(15, 100);
  tft.print("LINES: ");
  tft.print(clearedLines);
}



void lockPiece()
{
  for (uint8_t row = 0; row < 4; row++)
  {
    for (uint8_t col = 0; col < 4; col++)
    {
      if (hasBlock(currentPiece, currentRotation, row, col))
      {
        int8_t gridX = pieceX + col;
        int8_t gridY = pieceY + row;

        grid[gridY][gridX] = currentPiece + 1;
      }
    }
  }
}



uint8_t clearFullLines()
{
  uint8_t cleared = 0;

  for (int8_t row = GRID_HEIGHT - 1; row >= 0; row--)
  {
    bool full = true;

    for (uint8_t col = 0; col < GRID_WIDTH; col++)
    {
      if (grid[row][col] == 0)
      {
        full = false;
        break;
      }
    }

    if (full)
    {
      cleared++;

      for (int8_t r = row; r > 0; r--)
      {
        for (uint8_t c = 0; c < GRID_WIDTH; c++)
        {
          grid[r][c] = grid[r - 1][c];
        }
      }

      for (uint8_t c = 0; c < GRID_WIDTH; c++)
      {
        grid[0][c] = 0;
      }

      row++;
    }
  }

  return cleared;
}



void spawnPiece()
{
  currentPiece = nextPiece;
  nextPiece = random(PIECE_COUNT);
  currentRotation = 0;
  pieceX = (GRID_WIDTH - 4) / 2;
  pieceY = 0;

  if (checkCollision(currentPiece, currentRotation, pieceX, pieceY))
  {
    gameOver = true;
    showGameOver();
    return;
  }

  drawPieceCells(currentPiece, currentRotation, pieceX, pieceY);
}



void updateScore(uint8_t linesCleared)
{
  if (linesCleared == 0)
    return;

  clearedLines += linesCleared;
  level = clearedLines / 10;

  fallInterval = max((unsigned long)100, (unsigned long)(500 - level * 40));

  const uint16_t lineScores[] = {0, 40, 100, 300, 1200};

  score += lineScores[linesCleared] * (level + 1);
}



void tick()
{
  if (gameOver)
    return;

  if (checkCollision(currentPiece, currentRotation, pieceX, pieceY + 1))
  {
    lockPiece();

    uint8_t cleared = clearFullLines();

    updateScore(cleared);

    if (cleared > 0)
    {
      redrawFullField();
    }

    spawnPiece();
    drawHUD();
  }
  else
  {
    clearPieceCells(currentPiece, currentRotation, pieceX, pieceY);

    pieceY++;

    drawPieceCells(currentPiece, currentRotation, pieceX, pieceY);
  }
}



void hardDrop()
{
  if (gameOver)
    return;

  clearPieceCells(currentPiece, currentRotation, pieceX, pieceY);

  while (!checkCollision(currentPiece, currentRotation, pieceX, pieceY + 1))
  {
    pieceY++;
    score++;
  }

  lockPiece();

  uint8_t cleared = clearFullLines();

  updateScore(cleared);

  if (cleared > 0)
  {
    redrawFullField();
  }
  else
  {
    drawPieceCells(currentPiece, currentRotation, pieceX, pieceY);
  }

  spawnPiece();
  drawHUD();
}



void moveLeft()
{
  if (gameOver)
    return;

  if (!checkCollision(currentPiece, currentRotation, pieceX - 1, pieceY))
  {
    clearPieceCells(currentPiece, currentRotation, pieceX, pieceY);

    pieceX--;

    drawPieceCells(currentPiece, currentRotation, pieceX, pieceY);
  }
}



void moveRight()
{
  if (gameOver)
    return;

  if (!checkCollision(currentPiece, currentRotation, pieceX + 1, pieceY))
  {
    clearPieceCells(currentPiece, currentRotation, pieceX, pieceY);

    pieceX++;

    drawPieceCells(currentPiece, currentRotation, pieceX, pieceY);
  }
}



void rotatePiece()
{
  if (gameOver)
    return;

  uint8_t newRotation = (currentRotation + 1) % 4;

  if (!checkCollision(currentPiece, newRotation, pieceX, pieceY))
  {
    clearPieceCells(currentPiece, currentRotation, pieceX, pieceY);

    currentRotation = newRotation;

    drawPieceCells(currentPiece, currentRotation, pieceX, pieceY);
  }
}



void restartGame()
{
  for (uint8_t row = 0; row < GRID_HEIGHT; row++)
  {
    for (uint8_t col = 0; col < GRID_WIDTH; col++)
    {
      grid[row][col] = 0;
    }
  }

  score = 0;
  clearedLines = 0;
  level = 0;
  fallInterval = 500;
  gameOver = false;

  tft.fillScreen(ST77XX_BLACK);
  drawBorder();

  nextPiece = random(PIECE_COUNT);
  spawnPiece();

  drawHUD();
}



void handleIR()
{
  if (!IrReceiver.decode())
    return;

  if (IrReceiver.decodedIRData.protocol == NEC)
  {
    if (gameOver)
    {
      restartGame();

      IrReceiver.resume();
      return;
    }

    switch (IrReceiver.decodedIRData.command)
    {
      case 0x5A:
        moveRight();
        break;

      case 0x08:
        moveLeft();
        break;

      case 0x18:
        if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT))
        {
          rotatePiece();
        }
        break;

      case 0x52:
        if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT))
        {
          hardDrop();
        }
        break;
    }
  }

  IrReceiver.resume();
}



void setup()
{
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  randomSeed(analogRead(A0));

  IrReceiver.begin(IR_PIN, DISABLE_LED_FEEDBACK);

  restartGame();
}



void loop()
{
  handleIR();

  if (!gameOver && millis() - lastFall > fallInterval)
  {
    lastFall = millis();
    tick();
  }
}
