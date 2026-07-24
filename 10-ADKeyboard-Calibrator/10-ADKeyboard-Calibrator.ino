#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_CS    10
#define TFT_RST   8
#define TFT_DC    9
#define KEY_PIN   A0

Adafruit_ST7735 tft(TFT_CS, TFT_DC, 11, 13, TFT_RST);

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
#define KEY_MIN_LEFT 12
#define KEY_MAX_LEFT 25
#define KEY_MIN_SELECT 744
#define KEY_MAX_SELECT 748

#define CENTER_X 64
#define CENTER_Y 80

const char* keyLabels[] = {
  "NONE",
  "RIGHT",
  "UP",
  "DOWN",
  "LEFT",
  "SELECT"
};



int readKey(int value)
{
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



uint16_t getKeyColor(int key)
{
  switch (key)
  {
    case KEY_RIGHT:  return ST77XX_GREEN;
    case KEY_UP:     return ST77XX_CYAN;
    case KEY_DOWN:   return ST77XX_YELLOW;
    case KEY_LEFT:   return ST77XX_BLUE;
    case KEY_SELECT: return ST77XX_MAGENTA;
    default:         return ST77XX_WHITE;
  }
}



void drawKeyName(int key)
{
  uint16_t color = getKeyColor(key);
  const char* label = keyLabels[key];
  int labelWidth = strlen(label) * 6;
  int labelX = CENTER_X - labelWidth / 2;

  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.setCursor(labelX, CENTER_Y - 8);
  tft.print(label);
}



void drawRawValue(int analogValue)
{
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%d", analogValue);
  int valueWidth = strlen(buffer) * 6;
  int valueX = CENTER_X - valueWidth / 2;

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(valueX, CENTER_Y + 4);
  tft.print(buffer);
}



int previousAnalogValue;
int previousKey;



void renderScreen(int analogValue, int key)
{
  tft.fillRect(CENTER_X - 40, CENTER_Y - 12, 80, 28, ST77XX_BLACK);

  drawKeyName(key);
  drawRawValue(analogValue);
}



void setup()
{
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);

  previousAnalogValue = analogRead(KEY_PIN);
  previousKey = readKey(previousAnalogValue);

  renderScreen(previousAnalogValue, previousKey);
}



void loop()
{
  int analogValue = analogRead(KEY_PIN);
  int currentKey = readKey(analogValue);

  if (currentKey != previousKey || analogValue != previousAnalogValue)
  {
    renderScreen(analogValue, currentKey);

    previousAnalogValue = analogValue;
    previousKey = currentKey;
  }

  delay(50);
}
