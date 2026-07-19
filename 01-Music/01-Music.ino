#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
#define SPEAKER_PIN 4 // PC speaker connected to D4
#define LED_PIN 13 // Built-in LED

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Note Frequencies (in Hz) ---
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define REST     0

// --- Melody 1: Korobeiniki (Tetris Theme) ---
const int korobeiniki[] PROGMEM = {
  NOTE_E5, 4,  NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4,  NOTE_C5, 8,  NOTE_B4, 8,
  NOTE_A4, 4,  NOTE_A4, 8,  NOTE_C5, 8,  NOTE_E5, 4,  NOTE_D5, 8,  NOTE_C5, 8,
  NOTE_B4, -4, NOTE_C5, 8,  NOTE_D5, 4,  NOTE_E5, 4,
  NOTE_C5, 4,  NOTE_A4, 4,  NOTE_A4, 4,  REST, 4
};

// --- Melody 2: Super Mario Bros (Intro) ---
const int mario[] PROGMEM = {
  NOTE_E5, 8, NOTE_E5, 8, REST, 8, NOTE_E5, 8,
  REST, 8, NOTE_C5, 8, NOTE_E5, 8,
  NOTE_G5, 4, REST, 4, NOTE_G4, 4, REST, 4
};

// Calculate lengths of the arrays
const int korobeiniki_notes = sizeof(korobeiniki) / sizeof(korobeiniki[0]) / 2;
const int mario_notes = sizeof(mario) / sizeof(mario[0]) / 2;

const int imperial[] PROGMEM = {
  NOTE_A4, 4, NOTE_A4, 4, NOTE_A4, 4, NOTE_F4, -8, NOTE_C5, 16,
  NOTE_A4, 4, NOTE_F4, -8, NOTE_C5, 16, NOTE_A4, 2,

  NOTE_E5, 4, NOTE_E5, 4, NOTE_E5, 4, NOTE_F5, -8, NOTE_C5, 16,
  NOTE_GS4, 4, NOTE_F4, -8, NOTE_C5, 16, NOTE_A4, 2
};
const int imperial_notes = sizeof(imperial) / sizeof(imperial[0]) / 2;

const int ussr[] PROGMEM = {
  NOTE_G4, 4, NOTE_C5, 2, NOTE_G4, 4, NOTE_A4, 4, NOTE_B4, 2, NOTE_E4, 4, NOTE_E4, 4,
  NOTE_A4, 2, NOTE_G4, 4, NOTE_F4, 4, NOTE_G4, 2, NOTE_C4, 4, NOTE_C4, 4,
  NOTE_D4, 2, NOTE_D4, 4, NOTE_E4, 4, NOTE_F4, 2, NOTE_F4, 4, NOTE_G4, 4,
  NOTE_A4, 2, NOTE_B4, 4, NOTE_C5, 4, NOTE_D5, 2
};
const int ussr_notes = sizeof(ussr) / sizeof(ussr[0]) / 2;

int currentSong = 0; // State variable to toggle songs

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed - check wiring!"));
    for(;;);
  }
}

void loop() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Hello Nano V3.0!"));
  display.setTextSize(2);
  display.setCursor(0, 20);

  if (currentSong == 0) {
    display.println(F("TETRIS"));
    display.display(); 
    // Play Tetris at 144 BPM
    playMelody(korobeiniki, korobeiniki_notes, 144);
  } else if (currentSong == 1) {
    display.println(F("MARIO"));
    display.display(); 
    // Play Mario at 200 BPM
    playMelody(mario, mario_notes, 200);
  } else if (currentSong == 2) {
    display.println(F("STARWARS"));
    display.display(); 
    // Play Star Wars at 108 BPM
    playMelody(imperial, imperial_notes, 108);
  } else {
    display.println(F("USSR"));
    display.display(); 
    // Play USSR at 114 BPM
    playMelody(ussr, ussr_notes, 114);
  }

  // Toggle the state variable for the next loop iteration
  currentSong = (currentSong + 1) % 4; 
  
  delay(1500); // 1.5-second pause between songs
}

// Updated function accepts a pointer to a constant array, the number of notes, and the tempo
void playMelody(const int *melody, int numNotes, int tempo) {
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;

  for (int thisNote = 0; thisNote < numNotes * 2; thisNote = thisNote + 2) {
    int noteFreq = pgm_read_word_near(melody + thisNote);
    divider = (int)pgm_read_word_near(melody + thisNote + 1);
    
    if (divider > 0) {
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; 
    }

    if (noteFreq != REST) {
      digitalWrite(LED_PIN, HIGH);
    }
    tone(SPEAKER_PIN, noteFreq, noteDuration * 0.9);
    delay(noteDuration);
    digitalWrite(LED_PIN, LOW);
    noTone(SPEAKER_PIN);
  }
}