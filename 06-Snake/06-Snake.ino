#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <IRremote.hpp>

#define TFT_CS    10
#define TFT_RST   8
#define TFT_DC    9
#define IR_PIN    2

Adafruit_ST7735 tft(TFT_CS, TFT_DC, 11, 13, TFT_RST);


#define CELL 5

#define W (128 / CELL)
#define H (160 / CELL)

#define MAX_SNAKE 50


uint8_t snakeX[MAX_SNAKE];
uint8_t snakeY[MAX_SNAKE];

uint8_t length = 5;

int8_t dx = 1;
int8_t dy = 0;

uint8_t foodX;
uint8_t foodY;

uint8_t score = 0;

bool gameOver = false;

unsigned long lastMove = 0;



void drawCell(uint8_t x, uint8_t y, uint16_t color)
{
  tft.fillRect(
    x * CELL,
    y * CELL,
    CELL,
    CELL,
    color
  );
}



void spawnFood()
{
  foodX = random(W);
  foodY = random(H);
}



void drawSnake()
{
  for (uint8_t i = 0; i < length; i++)
  {
    drawCell(
      snakeX[i],
      snakeY[i],
      ST77XX_GREEN
    );
  }

  drawCell(
    foodX,
    foodY,
    ST77XX_RED
  );
}



bool checkCollision()
{
  // wall collision

  if (snakeX[0] >= W ||
      snakeY[0] >= H)
  {
    return true;
  }


  // self collision

  for (uint8_t i = 1; i < length; i++)
  {
    if (snakeX[0] == snakeX[i] &&
        snakeY[0] == snakeY[i])
    {
      return true;
    }
  }

  return false;
}



void showGameOver()
{
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);

  tft.setCursor(10, 40);
  tft.print("GAME OVER");


  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(15, 80);
  tft.print("SCORE: ");

  tft.print(score);
}



void restartGame()
{
  length = 5;
  score = 0;

  dx = 1;
  dy = 0;

  gameOver = false;


  tft.fillScreen(ST77XX_BLACK);


  for (uint8_t i = 0; i < length; i++)
  {
    snakeX[i] = 10 - i;
    snakeY[i] = 10;
  }


  spawnFood();

  drawSnake();
}



void moveSnake()
{
  if (gameOver)
    return;


  // erase tail

  drawCell(
    snakeX[length - 1],
    snakeY[length - 1],
    ST77XX_BLACK
  );


  // move body

  for (uint8_t i = length - 1; i > 0; i--)
  {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }


  // move head

  snakeX[0] += dx;
  snakeY[0] += dy;



  if (checkCollision())
  {
    gameOver = true;
    showGameOver();
    return;
  }



  // draw head

  drawCell(
    snakeX[0],
    snakeY[0],
    ST77XX_GREEN
  );



  // eat food

  if (snakeX[0] == foodX &&
      snakeY[0] == foodY)
  {

    if (length < MAX_SNAKE)
    {
      length++;
      score++;
    }


    spawnFood();

    drawCell(
      foodX,
      foodY,
      ST77XX_RED
    );
  }
}



void handleIR()
{
  if (!IrReceiver.decode())
    return;


  if (IrReceiver.decodedIRData.protocol == NEC)
  {

    // Any button restarts after game over

    if (gameOver)
    {
      restartGame();

      IrReceiver.resume();
      return;
    }


    if (!(IrReceiver.decodedIRData.flags &
          IRDATA_FLAGS_IS_REPEAT))
    {

      switch(IrReceiver.decodedIRData.command)
      {

        // RIGHT

        case 0x5A:

          if (dx != -1)
          {
            dx = 1;
            dy = 0;
          }

          break;


        // LEFT

        case 0x08:

          if (dx != 1)
          {
            dx = -1;
            dy = 0;
          }

          break;


        // UP

        case 0x18:

          if (dy != 1)
          {
            dx = 0;
            dy = -1;
          }

          break;


        // DOWN

        case 0x52:

          if (dy != -1)
          {
            dx = 0;
            dy = 1;
          }

          break;

      }
    }
  }


  IrReceiver.resume();
}



void setup()
{
  tft.initR(INITR_BLACKTAB);

  tft.setRotation(1);

  tft.fillScreen(ST77XX_BLACK);


  randomSeed(
    analogRead(A0)
  );


  for (uint8_t i = 0; i < length; i++)
  {
    snakeX[i] = 10 - i;
    snakeY[i] = 10;
  }


  spawnFood();

  drawSnake();


  IrReceiver.begin(
    IR_PIN,
    DISABLE_LED_FEEDBACK
  );
}



void loop()
{
  handleIR();


  if (millis() - lastMove > 150)
  {
    lastMove = millis();

    moveSnake();
  }
}