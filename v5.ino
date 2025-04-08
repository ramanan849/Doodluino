#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <EEPROM.h>

#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define JOY_X A5
#define JOY_Y A3
#define JOY_SW 6
#define LEVEL_EASY 0
#define LEVEL_MEDIUM 1
#define LEVEL_HARD 2

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define DOODLER_WIDTH 15
#define DOODLER_HEIGHT 28
#define PLATFORM_WIDTH 40
#define PLATFORM_HEIGHT 8
#define NUM_PLATFORMS 7
#define MAX_SCROLL 5    // Smoother scrolling
#define JUMP_FORCE -12
#define GRAVITY 0.4
#define TOP_OFFSET 30  // Height of score panel - the separate one
#define PLAY_AREA_HEIGHT (SCREEN_HEIGHT - TOP_OFFSET)
#define VISIBLE_PLATFORMS 7 // Start with 7 platforms
#define BASE_SCROLL 5       // Base scroll speed
#define SCROLL_INCREASE 0.1 // Scroll speed increase per score
#define BASE_GRAVITY 0.4    // Base gravity
#define GRAVITY_INCREASE 0.01 // Gravity increase per score

#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define BLUE 0x001F
#define RED 0xF800
#define MAGENTA 0xF81F
#define CYAN 0x07FF
#define YELLOW 0xFFE0


#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST 0

const int easyAddress = 0; 
const int medAddress = 2; 
const int highAddress = 4; 
const int INIT_MARKER_ADDR = 6;

bool beat_easy = 0;
bool beat_med = 0;
bool beat_hard = 0;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// Game state with position tracking
struct GameState {
  int doodlerX, doodlerY;
  float doodlerVelocityY;
  int platformX[NUM_PLATFORMS];
  int platformY[NUM_PLATFORMS];
  int platformDirection[NUM_PLATFORMS]; // 1 for moving and 0 for stationary
  int score = 0;
  bool gameOver = true;
  bool platformUsed[NUM_PLATFORMS];
  int prevDoodlerX, prevDoodlerY;
  int prevPlatformX[NUM_PLATFORMS];
  int prevPlatformY[NUM_PLATFORMS];
  int gameLevel = 0;
  int selectedOption = 0;
  bool levelSelected = false;
  int platformX_start;
  int platformY_start;
  bool plat_start_used;
  bool gameStartedByUser = false;
  int visiblePlatforms; // New variable to control visible platforms
};

struct Player {
  int easy_score;
  int med_score;
  int hard_score;
  char name[24];
};

Player highScore = {
  0,0,0,"The One Above All"
};

Player currentScore = {
  0,0,0,"Average Joe (YOU)"
};

GameState state;

// New parameters for moving platforms
const int PLATFORM_MOVE_SPEED = 1;
const int PLATFORM_MOVE_RANGE = 50; // movement range

void initializeEEPROM() {
  Serial.println("Initializing EEPROM check...");
  if (EEPROM.read(INIT_MARKER_ADDR) != 0x33) { // Check new marker address
    Serial.println("Initializing EEPROM with default high score values");
    EEPROM.put(easyAddress, highScore.easy_score);
    EEPROM.put(medAddress, highScore.med_score);
    EEPROM.put(highAddress, highScore.hard_score);
    EEPROM.write(INIT_MARKER_ADDR, 0x33); // Write marker to new address
  } else {
    Serial.println("EEPROM already initialized.");
    // Read existing high scores
    EEPROM.get(easyAddress, highScore.easy_score);
    Serial.print("Easy Level High Score = ");
    Serial.println(highScore.easy_score);
    EEPROM.get(medAddress, highScore.med_score);
    Serial.print("Med Level High Score = ");
    Serial.println(highScore.med_score);
    EEPROM.get(highAddress, highScore.hard_score);
    Serial.print("Hard Level High Score = ");
    Serial.println(highScore.hard_score);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("avana aa aa");
  pinMode(TFT_RST, OUTPUT);
  pinMode(JOY_SW, INPUT_PULLUP);
  digitalWrite(TFT_RST, LOW);
  delay(10);
  digitalWrite(TFT_RST, HIGH);
  delay(10);

  tft.begin();
  tft.setRotation(0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  showStartScreen();
  tft.setTextSize(3);
  tft.fillScreen(RED);
  tft.setTextColor(WHITE);
  tft.setCursor(40, 60);
  tft.print("DOODLUINO");
  EEPROM.get(easyAddress, highScore.easy_score);
  EEPROM.get(medAddress, highScore.med_score);
  EEPROM.get(highAddress, highScore.hard_score);
  initializeEEPROM();
  delay(2);
  tft.fillScreen(YELLOW);
  
  tft.setTextColor(BLUE);
  tft.setCursor(40, 60);
  delay(2);
  tft.print("DOODLUINO");
  delay(10);
  tft.fillScreen(GREEN);
  delay(10);
  tft.setTextColor(BLACK);
  tft.setCursor(40, 60);
  tft.print("DOODLUINO");
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  initGame();
  
} // look at this

void RestartGame(){
  showStartScreen();
  tft.fillScreen(RED);
  delay(2);
  tft.fillScreen(YELLOW);
  delay(2);
  tft.fillScreen(GREEN);
  delay(2);
  tft.fillScreen(BLACK);
  initGame();
} // look at this

void showStartScreen() {
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(40, 30);
  tft.print("DOODLUINO");

  tft.setTextSize(2);
  tft.setCursor(70, 80);
  tft.print("Select Level");

  state.levelSelected = false; // Reset selection state
  state.selectedOption = 0;
  // Start with EASY selected
  


  while (!state.levelSelected) {
    drawLevelMenu();
    readMenuInput();
  }
} // skip

void drawLevelMenu() {
  static int lastSelection = -1;

  if (state.selectedOption != lastSelection) {
    tft.setTextSize(2);
    // **Clear the entire menu area**
    tft.fillRect(40, 115, 160, 130, BLACK);
    // Adjusted to fully cover the menu

    for (int i = 0; i < 3; i++) {
      int yPos = 120 + (i * 40);
      if (i == state.selectedOption) {
        if (i == 0) {
          tft.fillRoundRect(40, yPos - 5, 160, 30, 5, GREEN);
        } else if (i == 1) {
          tft.fillRoundRect(40, yPos - 5, 160, 30, 5, YELLOW);
        } else if (i == 2) {
          tft.fillRoundRect(40, yPos - 5, 160, 30, 5, RED);
        }
      }

      tft.setCursor(80, yPos);
      switch (i) {
      case LEVEL_EASY:
        tft.setTextColor(WHITE);
        tft.print("EASY");
        break;
      case LEVEL_MEDIUM:
        tft.setTextColor(BLUE);
        tft.print("MEDIUM");
        break;
      case LEVEL_HARD:
        tft.setTextColor(WHITE);
        tft.print("HARD");
        break;
      }
    }
    lastSelection = state.selectedOption;
  }
} // skip

void playMusic() {
  int tempo = 120;
  int buzzer = 2;
  int gameIntro[] = {
      NOTE_B4, 16, NOTE_B5, 16, NOTE_FS5, 16, NOTE_DS5, 16, //1
      NOTE_B5, 32, NOTE_FS5, -16, NOTE_DS5, 8, NOTE_C5, 16,
      NOTE_C6, 16, NOTE_G6, 16, NOTE_E6, 16, NOTE_C6, 32, NOTE_G6, -16, NOTE_E6, 8,

      NOTE_B4, 16,  NOTE_B5, 16,  NOTE_FS5, 16,   NOTE_DS5, 16,  NOTE_B5, 32,  //2
      NOTE_FS5, -16, NOTE_DS5, 8,  NOTE_DS5, 32, NOTE_E5, 32,  NOTE_F5, 32,
      NOTE_F5, 32,  NOTE_FS5, 32,  NOTE_G5, 32, NOTE_GS5, 32,  NOTE_A5, 16, NOTE_B5, 8};
  int notes = sizeof(gameIntro) / sizeof(gameIntro[0]) / 2;
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = gameIntro[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzer, gameIntro[thisNote], noteDuration * 0.9);
    // Wait for the specief duration before playing the next note.
    delay(noteDuration);
    // stop the waveform generation before the next note.
    noTone(buzzer);
  }
} // skip for now

void readMenuInput() {
  static uint32_t lastInputTime = 0;
  if (millis() - lastInputTime < 200)
    return;

  int yValue = analogRead(JOY_Y);
  if (yValue < 400) {
    state.selectedOption = min(state.selectedOption + 1, 2);
    lastInputTime = millis();
  } else if (yValue > 600) {
    state.selectedOption = max(state.selectedOption - 1, 0);
    lastInputTime = millis();
  }

  if (digitalRead(JOY_SW) == LOW) {
    state.gameLevel = state.selectedOption;
    state.levelSelected = true;
    delay(200);
  }
} // skip

void initGame() {
  state.doodlerX = SCREEN_WIDTH / 2 - DOODLER_WIDTH / 2;
  state.doodlerY = PLAY_AREA_HEIGHT / 2;
  state.doodlerVelocityY = 0;
  state.score = 0;
  state.gameOver = false;
  state.plat_start_used = false;
  state.platformX_start = state.doodlerX;
  state.platformY_start = state.doodlerY - PLAY_AREA_HEIGHT / 2 + 5;
  state.visiblePlatforms = VISIBLE_PLATFORMS; // Initialize visiblePlatforms

  // Initialize platforms based on level
  for (int i = 0; i < NUM_PLATFORMS; i++) {
    state.plat_start_used = true;
    state.platformUsed[i] = false;
    state.platformX[i] = random(SCREEN_WIDTH - PLATFORM_WIDTH);
    state.platformY[i] = PLAY_AREA_HEIGHT - (i * (PLAY_AREA_HEIGHT / NUM_PLATFORMS));
    // Set platform movement based on level
    if (state.gameLevel == LEVEL_EASY) {
      state.platformDirection[i] = 0;
      // Stationary

    } else {
      state.platformDirection[i] = (random(2) == 0) ? 1 : -1; // 1 = moving right & -1 = moving left
    }

    state.prevPlatformX[i] = state.platformX[i];
    state.prevPlatformY[i] = state.platformY[i];
  }
} // check this

void loop() {
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate < 33)
    return;
  lastUpdate = millis();

  if (!state.gameOver) {
    updateGame();
    drawGame();
  } else {
    handleGameOver();
  }
} // check this

void updateGame() {
  // Input handling
  int joy = analogRead(JOY_X);
  state.prevDoodlerX = state.doodlerX;
  state.doodlerX += (joy < 400) ? -5 : (joy > 600) ? +5 : 0;
  if (state.doodlerX < 0)
    state.doodlerX = SCREEN_WIDTH - DOODLER_WIDTH;
  if (state.doodlerX > SCREEN_WIDTH - DOODLER_WIDTH)
    state.doodlerX = 0;
  // Dynamic gravity
  float dynamicGravity = BASE_GRAVITY + (state.score * GRAVITY_INCREASE);

  // Physics
  state.prevDoodlerY = state.doodlerY;
  state.doodlerVelocityY += dynamicGravity;
  state.doodlerY += state.doodlerVelocityY;

  checkCollisions();
  handleScrolling();
  updatePlatforms(); // Move the platforms
  checkGameOver();
} // ok

void updatePlatforms() {
  int moveSpeed = PLATFORM_MOVE_SPEED;

  // Adjust speed based on level and score
  if (state.gameLevel == LEVEL_HARD) {
    moveSpeed += state.score * 0.4;
  }

  for (int i = 0; i < NUM_PLATFORMS; i++) {
    state.prevPlatformX[i] = state.platformX[i];
    state.prevPlatformY[i] = state.platformY[i];

    if (state.gameLevel != LEVEL_EASY) {
      state.platformX[i] += moveSpeed * state.platformDirection[i];
      // Boundary check
      if (state.platformX[i] <= 0) {
        state.platformDirection[i] = 1;
        state.platformX[i] = 0;
      } else if (state.platformX[i] >= SCREEN_WIDTH - PLATFORM_WIDTH) {
        state.platformDirection[i] = -1;
        state.platformX[i] = SCREEN_WIDTH - PLATFORM_WIDTH;
      }
    }
  }
} // ok, but check again

void checkCollisions() {
  for (int i = 0; i < NUM_PLATFORMS; i++) {
    if (state.platformY[i] + TOP_OFFSET >= 0 &&
        state.platformY[i] + TOP_OFFSET < SCREEN_HEIGHT) {
      if (state.doodlerVelocityY > 0 &&
          state.doodlerX + DOODLER_WIDTH > state.platformX[i] &&
          state.doodlerX < state.platformX[i] + PLATFORM_WIDTH &&
          state.prevDoodlerY + DOODLER_HEIGHT <= state.platformY[i] &&
          state.doodlerY + DOODLER_HEIGHT >= state.platformY[i]) {

        float dynamicJump = JUMP_FORCE - (state.score * 0.1);
        state.doodlerVelocityY = dynamicJump;

        if (!state.platformUsed[i]) {
          state.score++;
          state.platformUsed[i] = true;
        }
      }
    }
  }
} // check this

void handleScrolling() {
  float dynamicScroll = BASE_SCROLL + (state.score * SCROLL_INCREASE);
  if (state.doodlerY < SCREEN_HEIGHT / 3) {
    int scroll = min(SCREEN_HEIGHT / 3 - state.doodlerY, dynamicScroll);
    state.doodlerY += scroll;

    for (int i = 0; i < NUM_PLATFORMS; i++) {
      state.platformY[i] += scroll;
      if (state.platformY[i] > SCREEN_HEIGHT) {
        state.platformX[i] = random(SCREEN_WIDTH - PLATFORM_WIDTH);
        state.platformY[i] = random(-PLATFORM_HEIGHT, 0);
        state.platformUsed[i] = false;
        state.platformDirection[i] = (random(2) == 0) ? 1 : -1;
        //new direction
      }
    }
  }
} // cehck this

void checkGameOver() {
  if (state.doodlerY > SCREEN_HEIGHT)
    state.gameOver = true;
} // check this

void drawGame() {
  static int lastScore = -1;
  if (state.score != lastScore) {
    if (state.score <= 10) {
      tft.fillRect(0, 0, SCREEN_WIDTH, TOP_OFFSET - 10, RED);
    } else if (state.score > 10 && state.score <= 30) {
      tft.fillRect(0, 0, SCREEN_WIDTH, TOP_OFFSET - 10, BLUE);
    } else if (state.score > 30 && state.score < 50) {
      tft.fillRect(0, 0, SCREEN_WIDTH, TOP_OFFSET - 10, CYAN);
    } else if (state.score >= 50 && state.score <= 89) {
      tft.fillRect(0, 0, SCREEN_WIDTH, TOP_OFFSET - 10, GREEN);
    }
    tft.setCursor(5, 5);
    tft.print("Score: ");
    tft.print(state.score);
    lastScore = state.score;
  }

  // Clear previous doodler position
  tft.fillRect(state.prevDoodlerX, state.prevDoodlerY + TOP_OFFSET,
               DOODLER_WIDTH, DOODLER_HEIGHT, BLACK);
  // Draw new doodler position
  tft.fillRect(state.doodlerX, state.doodlerY + TOP_OFFSET,
               DOODLER_WIDTH, DOODLER_HEIGHT, WHITE);
  tft.fillRect(state.platformX_start, state.platformY_start + TOP_OFFSET,
               PLATFORM_WIDTH, PLATFORM_HEIGHT, GREEN);
  // Update platforms
  int visiblePlatforms = NUM_PLATFORMS; // Default to NUM_PLATFORMS

  if (state.gameLevel != LEVEL_EASY) {
    if (state.score > 10 && state.score < 20) {
      // Reduce visible platforms gradually
      visiblePlatforms = NUM_PLATFORMS - (int)((state.score - 10) * 0.3); // Reduce by 3 between 10-20
      visiblePlatforms = max(4, visiblePlatforms); // Ensure at least 4 are visible
    }
  }

  for (int i = 0; i < NUM_PLATFORMS; i++) {
    if (i < visiblePlatforms && // Only draw visible platforms
        state.platformY[i] + TOP_OFFSET >= 0 &&
        state.platformY[i] + TOP_OFFSET < SCREEN_HEIGHT) {
      if (state.platformX[i] != state.prevPlatformX[i] ||
          state.platformY[i] != state.prevPlatformY[i]) {
        tft.fillRect(state.prevPlatformX[i], state.prevPlatformY[i] + TOP_OFFSET,
                     PLATFORM_WIDTH, PLATFORM_HEIGHT, BLACK);
      }

      tft.fillRect(state.platformX[i], state.platformY[i] + TOP_OFFSET,
                   PLATFORM_WIDTH, PLATFORM_HEIGHT, GREEN);
      state.prevPlatformX[i] = state.platformX[i];
      state.prevPlatformY[i] = state.platformY[i];
    }
  }
} // check this

void handleGameOver() {
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  tft.setCursor(50, 50);
  tft.print("GAME OVER :(");
  tft.setCursor(50, 100);
  tft.print("You scored: ");
  tft.print(state.score);
  tft.setCursor(40, 150);
  tft.print("Click joystick ");
  tft.setCursor(60, 180);
  tft.print("to restart");

  bool highScoreUpdated = false;

  if (state.gameLevel == LEVEL_EASY) {
    if (state.score > highScore.easy_score) {
      highScore.easy_score = state.score;
      beat_easy = 1;
      highScoreUpdated = true;
    }
  } else if (state.gameLevel == LEVEL_MEDIUM) {
    if (state.score > highScore.med_score) {
      highScore.med_score = state.score;
      beat_med = 1;
      highScoreUpdated = true;
    }
  } else if (state.gameLevel == LEVEL_HARD) {
    if (state.score > highScore.hard_score) {
      highScore.hard_score = state.score;
      beat_hard = 1;
      highScoreUpdated = true;
    }
  }

  if (highScoreUpdated) {
    if (state.gameLevel == LEVEL_EASY) {
      EEPROM.update(easyAddress, highScore.easy_score);
      // Optional Verification
      int readBackValue;
      EEPROM.get(easyAddress, readBackValue);
      if (readBackValue != highScore.easy_score) {
        Serial.println("EEPROM Write Error for Easy Score!");
      }
    }
    else if (state.gameLevel == LEVEL_MEDIUM) {
      EEPROM.update(medAddress, highScore.med_score);
      int readBackValue;
      EEPROM.get(medAddress, readBackValue);
      if (readBackValue != highScore.med_score) {
        Serial.println("EEPROM Write Error for Med Score!");
      }
    }
    else if (state.gameLevel == LEVEL_HARD) {
      EEPROM.update(highAddress, highScore.hard_score);
      int readBackValue;
      EEPROM.get(highAddress, readBackValue);
      if (readBackValue != highScore.hard_score) {
        Serial.println("EEPROM Write Error for Hard Score!");
      }
    }
  }

  while (digitalRead(JOY_SW) == HIGH) {
    delay(50);
  }

  tft.fillRect(40, 80, 160, 200, BLACK);
  state.levelSelected = false;
  state.selectedOption = 0;
  RestartGame();
}
