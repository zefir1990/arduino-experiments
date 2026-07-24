#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#include <avr/pgmspace.h>

#define TFT_CS    10
#define TFT_RST   8
#define TFT_DC    9
#define KEY_PIN   A0

Adafruit_ST7735 tft(TFT_CS, TFT_DC, 11, 13, TFT_RST);

#define CELL 7
#define GRID_WIDTH 10
#define GRID_HEIGHT 20
#define GRID_X 14
#define GRID_PIXEL_WIDTH (GRID_WIDTH * CELL)
#define GRID_PIXEL_HEIGHT (GRID_HEIGHT * CELL)
#define PIECE_COUNT 7

#define KEY_NONE 0
#define KEY_RIGHT 1
#define KEY_UP 2
#define KEY_DOWN 3
#define KEY_LEFT 4
#define KEY_SELECT 5

#define KEY_MIN_RIGHT 510
#define KEY_MAX_RIGHT 520
#define KEY_MIN_UP 152
#define KEY_MAX_UP 166
#define KEY_MIN_DOWN 335
#define KEY_MAX_DOWN 346
#define KEY_MIN_LEFT 5
#define KEY_MAX_LEFT 40
#define KEY_MIN_SELECT 744
#define KEY_MAX_SELECT 748

#define KEY_DEBOUNCE_SAMPLES 3

uint8_t gridOffsetY;
uint8_t previousKey;
uint8_t debouncedKey;
uint8_t candidateKey;
uint8_t candidateCount;

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

const uint16_t PIECE_COLORS[PIECE_COUNT] PROGMEM = {
  0x07FF, 0xFFE0, 0x780F, 0x07E0, 0xF800, 0x001F, 0xFD20
};

uint8_t grid[GRID_HEIGHT][GRID_WIDTH];
uint8_t currentPiece;
uint8_t currentRotation;
uint8_t nextPiece;
int8_t pieceX;
int8_t pieceY;
uint16_t clearedLines;
uint8_t level;
bool gameOver;
bool titleScreen;
unsigned long gameOverTime;
unsigned long lastFall;
uint16_t fallInterval;



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
    gridOffsetY + gridY * CELL,
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
          drawBlock(drawX, drawY, pgm_read_word(&PIECE_COLORS[piece]));
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
    gridOffsetY - 1,
    GRID_PIXEL_WIDTH + 2,
    GRID_PIXEL_HEIGHT + 2,
    ST77XX_WHITE
  );
}



void redrawFullField()
{
  tft.fillRect(
    GRID_X,
    gridOffsetY,
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
        drawBlock(col, row, pgm_read_word(&PIECE_COLORS[grid[row][col] - 1]));
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
  uint8_t previewY = gridOffsetY + 5;

  for (uint8_t row = 0; row < 4; row++)
  {
    for (uint8_t col = 0; col < 4; col++)
    {
      uint8_t blockX = previewX + col * CELL;
      uint8_t blockY = previewY + row * CELL;

      if (hasBlock(nextPiece, 0, row, col))
      {
        tft.fillRect(blockX, blockY, CELL, CELL, pgm_read_word(&PIECE_COLORS[nextPiece]));
      }
      else
      {
        tft.fillRect(blockX, blockY, CELL, CELL, ST77XX_BLACK);
      }
    }
  }

  uint8_t linesY = previewY + 4 * CELL + 10;

  tft.fillRect(previewX, linesY, CELL * 5, 16, ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  tft.setCursor(previewX, linesY);
  tft.print(F("LINES"));

  tft.setCursor(previewX, linesY + 8);
  tft.print(clearedLines);
}



void drawTitleScreen()
{
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);

  tft.setCursor(6, 50);
  tft.print(F("BIG TETRIS"));

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  tft.setCursor(25, 90);
  tft.print(F("PRESS ANY KEY"));

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  tft.setCursor(10, 115);
  tft.print(F("DEMENSDEUM.COM 2026"));
}



void showGameOver()
{
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);

  tft.setCursor(10, 40);
  tft.print(F("GAME OVER"));

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  tft.setCursor(15, 80);
  tft.print(F("LEVEL: "));
  tft.print(level);

  tft.setCursor(15, 90);
  tft.print(F("LINES: "));
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
    gameOverTime = millis();
    showGameOver();
    return;
  }

  drawPieceCells(currentPiece, currentRotation, pieceX, pieceY);
}



void updateClearedLines(uint8_t linesCleared)
{
  if (linesCleared == 0)
    return;

  clearedLines += linesCleared;
  level = clearedLines / 10;

  fallInterval = max(100, 1000 - level * 80);
}



void tick()
{
  if (gameOver)
    return;

  if (checkCollision(currentPiece, currentRotation, pieceX, pieceY + 1))
  {
    lockPiece();

    uint8_t cleared = clearFullLines();

    updateClearedLines(cleared);

    if (cleared > 0)
    {
      redrawFullField();
    }

    spawnPiece();

    if (!gameOver)
    {
      drawHUD();
    }
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
  }

  lockPiece();

  uint8_t cleared = clearFullLines();

  updateClearedLines(cleared);

  if (cleared > 0)
  {
    redrawFullField();
  }
  else
  {
    drawPieceCells(currentPiece, currentRotation, pieceX, pieceY);
  }

  spawnPiece();

  if (!gameOver)
  {
    drawHUD();
  }
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

  clearedLines = 0;
  level = 0;
  fallInterval = 1000;
  gameOver = false;

  tft.fillScreen(ST77XX_BLACK);
  drawBorder();

  nextPiece = random(PIECE_COUNT);
  spawnPiece();

  drawHUD();
}



int readKey()
{
  int value = analogRead(KEY_PIN);

  if (value >= KEY_MIN_RIGHT && value <= KEY_MAX_RIGHT)
  {
    return KEY_RIGHT;
  }

  if (value >= KEY_MIN_UP && value <= KEY_MAX_UP)
  {
    return KEY_UP;
  }

  if (value >= KEY_MIN_DOWN && value <= KEY_MAX_DOWN)
  {
    return KEY_DOWN;
  }

  if (value >= KEY_MIN_LEFT && value <= KEY_MAX_LEFT)
  {
    return KEY_LEFT;
  }

  if (value >= KEY_MIN_SELECT && value <= KEY_MAX_SELECT)
  {
    return KEY_SELECT;
  }

  return KEY_NONE;
}



int readDebouncedKey()
{
  int currentKey = readKey();

  if (currentKey == candidateKey)
  {
    if (candidateCount < KEY_DEBOUNCE_SAMPLES)
    {
      candidateCount++;
    }

    if (candidateCount >= KEY_DEBOUNCE_SAMPLES)
    {
      debouncedKey = currentKey;
    }
  }
  else
  {
    candidateKey = currentKey;
    candidateCount = 1;
  }

  return debouncedKey;
}



void handleADKeyboard(int currentKey)
{
  switch (currentKey)
  {
    case KEY_RIGHT:
      if (previousKey != KEY_RIGHT)
      {
        moveRight();
      }
      break;

    case KEY_LEFT:
      if (previousKey != KEY_LEFT)
      {
        moveLeft();
      }
      break;

    case KEY_SELECT:
      if (previousKey != KEY_SELECT)
      {
        rotatePiece();
      }
      break;

    case KEY_UP:
      // if (previousKey != KEY_UP)
      // {
      //   rotatePiece();
      // }
      break;

    case KEY_DOWN:
      if (previousKey != KEY_DOWN)
      {
        hardDrop();
      }
      break;
  }
}



void setup()
{
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  gridOffsetY = (tft.height() - GRID_PIXEL_HEIGHT) / 2;

  randomSeed(analogRead(A0));

  debouncedKey = KEY_NONE;
  candidateKey = KEY_NONE;
  candidateCount = 0;

  titleScreen = true;
  drawTitleScreen();
}



void loop()
{
  int currentKey = readDebouncedKey();
  bool keyJustPressed = currentKey != KEY_NONE && previousKey == KEY_NONE;

  if (titleScreen)
  {
    if (keyJustPressed)
    {
      titleScreen = false;
      restartGame();
    }

    previousKey = currentKey;
    return;
  }

  handleADKeyboard(currentKey);

  if (gameOver)
  {
    if (millis() - gameOverTime < 3000)
    {
      previousKey = currentKey;
      return;
    }

    if (keyJustPressed)
    {
      drawTitleScreen();
      titleScreen = true;
    }

    previousKey = currentKey;
    return;
  }

  if (millis() - lastFall > fallInterval)
  {
    lastFall = millis();
    tick();
  }

  previousKey = currentKey;
}
