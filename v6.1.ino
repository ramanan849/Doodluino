// v6 Menu + v5 Game Logic
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <EEPROM.h>

// Pin Definitions (Consistent)
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define JOY_X A5
#define JOY_Y A3
#define JOY_SW 6
#define BUZZER_PIN 2 // Define your buzzer pin if used

// Level Definitions (Consistent)
#define LEVEL_EASY 0
#define LEVEL_MEDIUM 1
#define LEVEL_HARD 2

// Game States (Menu States) - Use integers
#define MAIN_MENU 0
#define LEVEL_MENU 1
#define HIGHSCORES_MENU 2
#define CREDITS_MENU 3
#define GAME_PLAYING 4
#define GAME_OVER_STATE 5
// #define SPLASH_SCREEN 6 // Optional: Add a splash screen state

// Game Constants (Taken from v5)
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define DOODLER_WIDTH 15
#define DOODLER_HEIGHT 28
#define PLATFORM_WIDTH 40
#define PLATFORM_HEIGHT 8
#define NUM_PLATFORMS 7
#define MAX_SCROLL 5    // Smoother scrolling (v5)
#define JUMP_FORCE -12  // v5
#define GRAVITY 0.4     // *** Fixed Gravity from v5 ***
#define TOP_OFFSET 30  // Height of score panel
#define PLAY_AREA_HEIGHT (SCREEN_HEIGHT - TOP_OFFSET)
// #define VISIBLE_PLATFORMS 7 // Not explicitly used in v5 logic this way
// #define BASE_SCROLL 5       // Not explicitly used in v5 logic this way
// #define SCROLL_INCREASE 0.1 // Not explicitly used in v5 logic this way
// #define BASE_GRAVITY 0.4    // Use fixed GRAVITY from v5
// #define GRAVITY_INCREASE 0.01 // Not used in v5

// Color definitions (Consistent)
#define BLACK   0x0000
#define WHITE   0xFFFF
#define GREEN   0x07E0
#define BLUE    0x001F
#define RED     0xF800
#define MAGENTA 0xF81F
#define CYAN    0x07FF
#define YELLOW  0xFFE0

// Note definitions (Can keep from previous)
#define NOTE_B4  494
#define NOTE_B5  988
#define NOTE_FS5 740
#define NOTE_DS5 622
#define NOTE_C5  523
#define NOTE_C6  1047
#define NOTE_G6  1568
#define NOTE_E6  1319
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_C4  262
#define REST     0

// EEPROM Addresses (Taken from v5)
const int easyAddress = 0;
const int medAddress = 2; // Assuming int is 2 bytes on target platform
const int highAddress = 4;
const int INIT_MARKER_ADDR = 6; // Marker address from v5

// Global Variables
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// GameState Struct (Ensure it includes members needed by v5 game logic AND menu logic)
struct GameState {
  int doodlerX, doodlerY;
  float doodlerVelocityY;
  int currentMenu; // Current state (menu, game, etc.) - type is int
  int mainMenuSelection = 0; // For main menu
  int platformX[NUM_PLATFORMS];
  int platformY[NUM_PLATFORMS];
  // int platformDirection[NUM_PLATFORMS]; // Not used in v5 game logic
  int score = 0;
  bool gameOver = true;
  bool platformUsed[NUM_PLATFORMS]; // v5 uses this
  int prevDoodlerX, prevDoodlerY; // v5 uses these
  int prevPlatformX[NUM_PLATFORMS]; // v5 uses these
  int prevPlatformY[NUM_PLATFORMS]; // v5 uses these
  int gameLevel = LEVEL_EASY; // Default level, consistent
  int selectedOption = 0; // Used for level menu selection
  // int visiblePlatforms = VISIBLE_PLATFORMS; // Not used in v5 game logic
  bool levelSelected = false; // v5 uses this, keep it if needed elsewhere, maybe redundant now
};

// Player Struct for High Scores (Consistent)
struct Player {
  int easy_score;
  int med_score;
  int hard_score;
};

Player highScore = {0, 0, 0}; // Default high scores
GameState state;

// --- Function Declarations ---
// Core & Menu
void setup();
void loop();
void drawMainMenu();
void drawLevelMenu();
void drawHighScores();
void drawCredits();
void readMenuInput();
void handleMenuSelection();
void readMenuInput_GameOver();
void resetDrawFlags();

// EEPROM (v5 Versions)
void initializeEEPROM();
void updateHighScore();

// Game Logic (v5 Versions)
void initGame();
void updateGame();
void drawGame();
void checkCollisions();
void handleScrolling();
void updatePlatforms();
void checkGameOver();

// Other
void handleGameOver(); // Displays game over screen
void playMusic();      // Optional

// --- EEPROM Functions (v5 VERSION) ---
void initializeEEPROM() {
    Serial.println("Initializing EEPROM check...");
    if (EEPROM.read(INIT_MARKER_ADDR) != 0x33) {
        Serial.println("Initializing EEPROM with default high scores.");
        highScore = {0, 0, 0}; // Ensure defaults are 0
        EEPROM.put(easyAddress, highScore.easy_score);
        EEPROM.put(medAddress, highScore.med_score);
        EEPROM.put(highAddress, highScore.hard_score);
        EEPROM.write(INIT_MARKER_ADDR, 0x33);
    } else {
        Serial.println("EEPROM already initialized. Reading scores...");
        EEPROM.get(easyAddress, highScore.easy_score);
        EEPROM.get(medAddress, highScore.med_score);
        EEPROM.get(highAddress, highScore.hard_score);
    }
    Serial.print("Easy High: "); Serial.println(highScore.easy_score);
    Serial.print("Med High: "); Serial.println(highScore.med_score);
    Serial.print("Hard High: "); Serial.println(highScore.hard_score);
}

// *** v5 updateHighScore logic ***
void updateHighScore() {
    bool highScoreUpdated = false;
    // Temporary flags (consider removing if not used elsewhere)
    int beat_easy = 0, beat_med = 0, beat_hard = 0;

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
        Serial.print("New high score for level "); Serial.print(state.gameLevel); Serial.print(": "); Serial.println(state.score);
        // Use EEPROM.put for simplicity and potential wear leveling benefits over update
        if (state.gameLevel == LEVEL_EASY) {
            EEPROM.put(easyAddress, highScore.easy_score);
        } else if (state.gameLevel == LEVEL_MEDIUM) {
            EEPROM.put(medAddress, highScore.med_score);
        } else if (state.gameLevel == LEVEL_HARD) {
            EEPROM.put(highAddress, highScore.hard_score);
        }
        // Optional: Verification (can be kept)
        int readBackValue;
        int addressToCheck = (state.gameLevel == LEVEL_EASY) ? easyAddress : (state.gameLevel == LEVEL_MEDIUM) ? medAddress : highAddress;
        EEPROM.get(addressToCheck, readBackValue);
        if (readBackValue != state.score) {
           Serial.println("EEPROM Write/Verify Error!");
        } else {
           Serial.println("EEPROM Save Verified.");
        }
    }
}


// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  Serial.println("Doodluino Starting (v6 Menu + v5 Game)...");

  pinMode(TFT_RST, OUTPUT);
  pinMode(JOY_SW, INPUT_PULLUP);
  digitalWrite(TFT_RST, LOW); delay(10); digitalWrite(TFT_RST, HIGH); delay(10);

  tft.begin();
  tft.setRotation(0);

  initializeEEPROM(); // Load scores using v5 logic

  // Initial state setup
  state.currentMenu = MAIN_MENU; // Start at the Main Menu
  state.mainMenuSelection = 0;
  state.gameOver = true; // Start in a non-playing state

  randomSeed(analogRead(A0)); // Seed random generator

}

// --- Main Loop (State Machine - FROM PREVIOUSLY CORRECTED VERSION) ---
void loop() {
  // Main game loop - handles different states
  switch (state.currentMenu) {
    case MAIN_MENU:
      drawMainMenu();
      readMenuInput();
      break;

    case LEVEL_MENU:
      drawLevelMenu();
      readMenuInput();
      break;

    case HIGHSCORES_MENU:
      drawHighScores();
      readMenuInput();
      break;

    case CREDITS_MENU:
      drawCredits();
      readMenuInput();
      break;

    case GAME_PLAYING:
      if (!state.gameOver) {
        static uint32_t lastGameUpdate = 0;
        uint32_t now = millis();
        // v5 Game logic doesn't necessarily need throttling, but can keep if desired
        if (now - lastGameUpdate > 30) { // ~33 FPS limit
          updateGame(); // *** Calls v5 updateGame ***
          drawGame();   // *** Calls v5 drawGame ***
          lastGameUpdate = now;
        }
      } else {
        // If gameOver becomes true while in GAME_PLAYING state
        updateHighScore(); // *** Calls v5 updateHighScore ***
        state.currentMenu = GAME_OVER_STATE;
        resetDrawFlags(); // Reset draw flags for game over screen
      }
      break;

    case GAME_OVER_STATE:
      handleGameOver(); // Display game over message
      readMenuInput_GameOver(); // Listens for input to return to menu
      break;

  } // End switch(state.currentMenu)

}

// --- Menu Drawing Functions (FROM PREVIOUSLY CORRECTED VERSION) ---

// Flags to control drawing (reset when entering the state)
static bool mainMenuFirstDraw = true;
static bool levelMenuFirstDraw = true;
static bool highScoresDrawn = false;
static bool creditsDrawn = false;
static bool gameOverMsgDrawn = false;

// Helper function to reset flags when state changes
void resetDrawFlags() {
    mainMenuFirstDraw = true;
    levelMenuFirstDraw = true;
    highScoresDrawn = false;
    creditsDrawn = false;
    gameOverMsgDrawn = false;
}


void drawMainMenu() {
  static int lastSelection = -1;

  if (state.mainMenuSelection != lastSelection || mainMenuFirstDraw) {
    if (mainMenuFirstDraw) {
      tft.fillScreen(BLACK);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.setCursor(40, 30);
      tft.print("DOODLUINO");
      tft.setTextSize(2);
      mainMenuFirstDraw = false;
    } else {
      tft.fillRect(35, 110, 170, 130, BLACK);
    }

    const char* menuItems[] = {"LEVEL SELECT", "HIGH SCORES", "CREDITS"};
    for (int i = 0; i < 3; i++) {
      int yPos = 120 + (i * 40);
      if (i == state.mainMenuSelection) {
        tft.fillRoundRect(40, yPos - 5, 160, 30, 5, GREEN);
        tft.setTextColor(BLACK);
      } else {
        tft.setTextColor(WHITE);
      }
      int textWidth = strlen(menuItems[i]) * 6 * 2;
      tft.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, yPos);
      tft.print(menuItems[i]);
    }
    lastSelection = state.mainMenuSelection;
  }
}

void drawLevelMenu() {
  static int lastSelection = -1;
  const int numOptions = 4; // Easy, Med, Hard, Back

  if (state.selectedOption != lastSelection || levelMenuFirstDraw) {
    if (levelMenuFirstDraw) {
       tft.fillScreen(BLACK);
       tft.setTextColor(WHITE);
       tft.setTextSize(3);
       tft.setCursor(30, 30);
       tft.print("SELECT LEVEL");
       tft.setTextSize(2);
       levelMenuFirstDraw = false;
    } else {
       tft.fillRect(35, 110, 170, 170, BLACK);
    }

    for (int i = 0; i < numOptions; i++) {
      int yPos = 120 + (i * 40);
      bool isSelected = (i == state.selectedOption);
      if (isSelected) {
         uint16_t color = BLUE; // Back default
         if (i == LEVEL_EASY) color = GREEN;
         else if (i == LEVEL_MEDIUM) color = YELLOW;
         else if (i == LEVEL_HARD) color = RED;
         tft.fillRoundRect(40, yPos - 5, 160, 30, 5, color);
      }

      const char* optionText = "";
      uint16_t textColor = WHITE;
      switch (i) {
        case LEVEL_EASY: optionText = "EASY"; textColor = (isSelected ? BLACK : WHITE); break;
        case LEVEL_MEDIUM: optionText = "MEDIUM"; textColor = (isSelected ? BLACK : BLUE); break;
        case LEVEL_HARD: optionText = "HARD"; textColor = (isSelected ? BLACK : WHITE); break;
        case 3: optionText = "BACK"; textColor = (isSelected ? WHITE : RED); break;
      }
      tft.setTextColor(textColor);
      int textWidth = strlen(optionText) * 6 * 2;
      tft.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, yPos);
      tft.print(optionText);
    }
    lastSelection = state.selectedOption;
  }
}

void drawHighScores() {
  if (!highScoresDrawn) {
      tft.fillScreen(BLACK);
      tft.setTextColor(WHITE);
      tft.setTextSize(2);
      tft.setCursor(20, 30);
      tft.print("HIGH SCORES");
      tft.setCursor(60, 80); tft.print("EASY: "); tft.print(highScore.easy_score);
      tft.setCursor(60, 120); tft.print("MEDIUM: "); tft.print(highScore.med_score);
      tft.setCursor(60, 160); tft.print("HARD: "); tft.print(highScore.hard_score);
      tft.setTextColor(RED);
      tft.setCursor(20, 220);
      tft.print("Press Select to BACK");
      highScoresDrawn = true;
  }
}

void drawCredits() {
  if (!creditsDrawn) {
      tft.fillScreen(BLACK);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.setCursor(60, 50);
      tft.print("CREDITS");
      tft.setTextSize(2);
      tft.setCursor(30, 120);
      tft.print("Game By: LUKOG"); // Your name
      tft.setTextColor(RED);
      tft.setCursor(20, 220);
      tft.print("Press Select to BACK");
      creditsDrawn = true;
   }
}


// --- Input Handling Functions (FROM PREVIOUSLY CORRECTED VERSION) ---

void readMenuInput() {
  static uint32_t lastInputTime = 0;
  uint32_t now = millis();
  if (now - lastInputTime < 180) return;

  int yValue = analogRead(JOY_Y);
  int swValue = digitalRead(JOY_SW);
  bool inputProcessed = false;
  int currentSelection = 0;
  int maxOption = 0;

  switch(state.currentMenu) {
      case MAIN_MENU: currentSelection = state.mainMenuSelection; maxOption = 2; break;
      case LEVEL_MENU: currentSelection = state.selectedOption; maxOption = 3; break;
      case HIGHSCORES_MENU: case CREDITS_MENU: break; // Only SW handled below
      default: return;
  }

  if (state.currentMenu == MAIN_MENU || state.currentMenu == LEVEL_MENU) {
      if (yValue < 300) { // Down
          currentSelection++; if (currentSelection > maxOption) currentSelection = 0;
          inputProcessed = true;
      } else if (yValue > 700) { // Up
          currentSelection--; if (currentSelection < 0) currentSelection = maxOption;
          inputProcessed = true;
      }
      if (inputProcessed) {
           if(state.currentMenu == MAIN_MENU) state.mainMenuSelection = currentSelection;
           else if (state.currentMenu == LEVEL_MENU) state.selectedOption = currentSelection;
           lastInputTime = now;
      }
  }

  if (swValue == LOW) { // Select Pressed
    handleMenuSelection();
    lastInputTime = now + 250; // Longer debounce after select
  }
}

void handleMenuSelection() {
  int previousMenu = state.currentMenu;

  switch(state.currentMenu) {
      case MAIN_MENU:
          switch(state.mainMenuSelection) {
              case 0: state.currentMenu = LEVEL_MENU; state.selectedOption = 0; break;
              case 1: state.currentMenu = HIGHSCORES_MENU; break;
              case 2: state.currentMenu = CREDITS_MENU; break;
          }
          break;
      case LEVEL_MENU:
          if (state.selectedOption == 3) { // Back
              state.currentMenu = MAIN_MENU; state.mainMenuSelection = 0;
          } else { // Level selected
              state.gameLevel = state.selectedOption;
              state.levelSelected = true; // Keep if v5 logic implicitly uses it?
              state.currentMenu = GAME_PLAYING;
              initGame(); // *** Calls v5 initGame ***
          }
          break;
      case HIGHSCORES_MENU: case CREDITS_MENU: // Back
          state.currentMenu = MAIN_MENU;
          state.mainMenuSelection = (previousMenu == HIGHSCORES_MENU) ? 1 : 2;
          break;
  }

  if (state.currentMenu != previousMenu) {
      resetDrawFlags(); // Reset drawing flags for the new state
  }
}


void readMenuInput_GameOver() {
    static uint32_t lastInputTime_GO = 0;
    uint32_t now = millis();
     if (now - lastInputTime_GO < 400) return;
    if (digitalRead(JOY_SW) == LOW) {
        state.currentMenu = MAIN_MENU;
        state.mainMenuSelection = 0;
        state.gameOver = true; // Ensure game over is true
        lastInputTime_GO = now;
        resetDrawFlags();
    }
}


// --- Game Logic Functions (*** v5 VERSION ***) ---

void initGame() {
    Serial.print("Initializing Game (v5) - Level: "); Serial.println(state.gameLevel);
    state.doodlerX = SCREEN_WIDTH / 2 - DOODLER_WIDTH / 2;
    state.doodlerY = SCREEN_HEIGHT - DOODLER_HEIGHT - 50; // Start lower
    state.doodlerVelocityY = JUMP_FORCE; // Initial jump force
    state.score = 0;
    state.gameOver = false; // Start the game

    // Initialize platforms
    for (int i = 0; i < NUM_PLATFORMS; i++) {
        state.platformUsed[i] = false;
        // Place the first platform directly below the player
        if (i == 0) {
            state.platformY[i] = state.doodlerY + DOODLER_HEIGHT + 10; // Ensure it's reachable
            state.platformX[i] = state.doodlerX + (DOODLER_WIDTH / 2) - (PLATFORM_WIDTH / 2); // Centered below
        } else {
            // Spread other platforms somewhat evenly above the previous one
            state.platformY[i] = state.platformY[i - 1] - (PLAY_AREA_HEIGHT / NUM_PLATFORMS) - random(-20, 20);
            // Ensure Y is not too high initially
            state.platformY[i] = max(-PLATFORM_HEIGHT, state.platformY[i]);
            state.platformX[i] = random(SCREEN_WIDTH - PLATFORM_WIDTH); // Random horizontal position
        }
        // Store initial positions as previous positions for drawing
        state.prevPlatformX[i] = state.platformX[i];
        state.prevPlatformY[i] = state.platformY[i];
    }
     // Store initial doodler position as previous position
    state.prevDoodlerX = state.doodlerX;
    state.prevDoodlerY = state.doodlerY;

    // Clear screen (handled by state change in loop now)
    // tft.fillScreen(BLACK);
    resetDrawFlags(); // Reset draw flags if game elements use them
}

void updateGame() {
    // 1. Handle Input (Horizontal Movement)
    int joyX = analogRead(JOY_X);
    state.prevDoodlerX = state.doodlerX; // Store previous position
    // Adjust speed based on level? (v5 logic might not have level-based speed)
    int horizontalSpeed = 4; // v5 used a fixed speed? Check v5 source if needed.
    if (joyX < 400) { // Left
        state.doodlerX -= horizontalSpeed;
    } else if (joyX > 600) { // Right
        state.doodlerX += horizontalSpeed;
    }

    // Screen wrap-around for doodler
    if (state.doodlerX < -DOODLER_WIDTH) state.doodlerX = SCREEN_WIDTH;
    if (state.doodlerX > SCREEN_WIDTH) state.doodlerX = -DOODLER_WIDTH;

    // 2. Apply Physics (Fixed Gravity from v5)
    state.prevDoodlerY = state.doodlerY; // Store previous position
    state.doodlerVelocityY += GRAVITY;    // *** Use fixed GRAVITY ***
    state.doodlerVelocityY = min(state.doodlerVelocityY, 15.0f); // Apply terminal velocity
    state.doodlerY += state.doodlerVelocityY;

    // 3. Handle Scrolling
    handleScrolling(); // Calls v5 handleScrolling

    // 4. Update Platforms (v5 logic - likely stationary)
    updatePlatforms(); // Calls v5 updatePlatforms

    // 5. Check Collisions
    checkCollisions(); // Calls v5 checkCollisions

    // 6. Check Game Over Condition
    checkGameOver(); // Calls v5 checkGameOver
}

void handleScrolling() {
    int scrollThreshold = PLAY_AREA_HEIGHT / 2;
    if (state.doodlerY < scrollThreshold && state.doodlerVelocityY < 0) {
        int scrollAmount = (int)(-state.doodlerVelocityY);
        scrollAmount = max(1, min(scrollAmount, MAX_SCROLL)); // Use fixed MAX_SCROLL from v5
        state.doodlerY += scrollAmount;
        for (int i = 0; i < NUM_PLATFORMS; i++) {
            state.platformY[i] += scrollAmount;
            if (state.platformY[i] > PLAY_AREA_HEIGHT) {
                state.platformUsed[i] = false;
                int minY = -PLATFORM_HEIGHT - 20;
                int maxY = minY - (PLAY_AREA_HEIGHT / (NUM_PLATFORMS / 2));
                state.platformY[i] = random(maxY, minY);
                state.platformX[i] = random(SCREEN_WIDTH - PLATFORM_WIDTH);
            }
        }
    }
}

void updatePlatforms() {
    // v5 platforms are likely stationary. This function might be empty or just update prev positions.
    for (int i = 0; i < NUM_PLATFORMS; i++) {
        state.prevPlatformX[i] = state.platformX[i];
        state.prevPlatformY[i] = state.platformY[i];
    }
}


void checkCollisions() {
    if (state.doodlerVelocityY < 0) return; // Check only when falling

    for (int i = 0; i < NUM_PLATFORMS; i++) {
        // Bounding Box Collision Check (Simplified from v5 source if necessary)
        bool xOverlap = (state.doodlerX < state.platformX[i] + PLATFORM_WIDTH) &&
                        (state.doodlerX + DOODLER_WIDTH > state.platformX[i]);

        // Vertical check: feet landing on platform
        bool yLanded = (state.doodlerY + DOODLER_HEIGHT >= state.platformY[i]) &&
                       (state.prevDoodlerY + DOODLER_HEIGHT <= state.platformY[i] + 2); // Check against previous position

        if (xOverlap && yLanded) {
             state.doodlerVelocityY = JUMP_FORCE; // Fixed jump force
             state.doodlerY = state.platformY[i] - DOODLER_HEIGHT; // Snap to top

             if (!state.platformUsed[i]) {
                 state.score++;
                 state.platformUsed[i] = true;
             }
             return; // Bounce off one platform only
        }
    }
}

void checkGameOver() {
    if (state.doodlerY > PLAY_AREA_HEIGHT) {
        state.gameOver = true;
    }
}


void drawGame() {
    // Efficient drawing: Clear previous, draw current

    // 1. Clear previous doodler position
    tft.fillRect(state.prevDoodlerX, state.prevDoodlerY + TOP_OFFSET, DOODLER_WIDTH, DOODLER_HEIGHT, BLACK);

    // 2. Clear previous platform positions
    for (int i = 0; i < NUM_PLATFORMS; i++) {
        // Only clear if potentially visible
        bool wasVisible = (state.prevPlatformY[i] + TOP_OFFSET + PLATFORM_HEIGHT > TOP_OFFSET) && (state.prevPlatformY[i] + TOP_OFFSET < SCREEN_HEIGHT);
        if (wasVisible) { // Check only visibility, assume updateGame updated prev pos correctly
             tft.fillRect(state.prevPlatformX[i], state.prevPlatformY[i] + TOP_OFFSET, PLATFORM_WIDTH, PLATFORM_HEIGHT, BLACK);
        }
    }

    // 3. Draw Platforms (current positions)
    for (int i = 0; i < NUM_PLATFORMS; i++) {
        if (state.platformY[i] + TOP_OFFSET + PLATFORM_HEIGHT > TOP_OFFSET && state.platformY[i] + TOP_OFFSET < SCREEN_HEIGHT) {
            tft.fillRect(state.platformX[i], state.platformY[i] + TOP_OFFSET, PLATFORM_WIDTH, PLATFORM_HEIGHT, GREEN); // v5 likely used fixed green
        }
    }

    // 4. Draw Doodler (current position)
    if (state.doodlerY + TOP_OFFSET < SCREEN_HEIGHT && state.doodlerY + TOP_OFFSET + DOODLER_HEIGHT > TOP_OFFSET) {
        tft.fillRect(state.doodlerX, state.doodlerY + TOP_OFFSET, DOODLER_WIDTH, DOODLER_HEIGHT, WHITE);
    }

    // 5. Draw Score / UI on top panel (can keep improved version)
    static int lastScoreDrawn = -1;
    if (state.score != lastScoreDrawn) {
        uint16_t scoreBgColor = RED;
        if (state.score >= 10) scoreBgColor = BLUE;
        if (state.score >= 30) scoreBgColor = MAGENTA;
        if (state.score >= 50) scoreBgColor = GREEN;
        if (state.score >= 100) scoreBgColor = YELLOW;
        tft.fillRect(0, 0, SCREEN_WIDTH, TOP_OFFSET, scoreBgColor);
        tft.setTextColor(WHITE);
        tft.setTextSize(2);
        tft.setCursor(5, 5 + (TOP_OFFSET - 16)/2);
        tft.print("Score: ");
        tft.print(state.score);
        lastScoreDrawn = state.score;
    }
}

// --- Game Over Display ---
void handleGameOver() {
    if (!gameOverMsgDrawn) {
        tft.fillScreen(BLACK);
        tft.setTextSize(3);
        tft.setCursor(40, 100);
        tft.setTextColor(RED);
        tft.print("GAME OVER");
        tft.setTextSize(2);
        tft.setCursor(50, 150);
        tft.setTextColor(WHITE);
        tft.print("Score: ");
        tft.print(state.score);

        // Check if it was a new high score (using v5 logic variables if needed)
        int currentHighScore = 0;
        bool isNewHighScore = false;
        if(state.gameLevel == LEVEL_EASY) currentHighScore = highScore.easy_score;
        else if (state.gameLevel == LEVEL_MEDIUM) currentHighScore = highScore.med_score;
        else currentHighScore = highScore.hard_score;
        if (state.score > 0 && state.score == currentHighScore) isNewHighScore = true; // Check against memory score

        if (isNewHighScore) {
            tft.setTextColor(YELLOW);
            tft.setCursor(30, 180);
            tft.print("New High Score!");
        }

        tft.setTextColor(WHITE);
        tft.setCursor(20, 220);
        tft.print("Press Select -> Menu");
        gameOverMsgDrawn = true;
    }
}

// --- Optional Music Function ---
void playMusic() {
  // Keep or modify the music function as desired
}