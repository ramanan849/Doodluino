// v6.8 - Doodle Jump with Red Ball Obstacles (Fully Working)
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <EEPROM.h>

// --- Pin Definitions ---
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define JOY_X A5
#define JOY_Y A3
#define JOY_SW 6
#define BUZZER_PIN 2

// --- Game Constants ---
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define DOODLER_WIDTH 15
#define DOODLER_HEIGHT 28
#define PLATFORM_WIDTH 40
#define PLATFORM_HEIGHT 8
#define NUM_PLATFORMS 7
#define MAX_SCROLL 5    // Smoother scrolling
#define JUMP_FORCE -12
#define GRAVITY 0.4 // Note: Your v5 code uses dynamic gravity in updateGame, GRAVITY constant seems unused there.
#define TOP_OFFSET 30   // Height of score panel - the separate one
# define BOTTOM_OFFSET 40
#define PLAY_AREA_HEIGHT (SCREEN_HEIGHT - TOP_OFFSET)
#define VISIBLE_PLATFORMS 7 // Start with 7 platforms
#define BASE_SCROLL 5       // Base scroll speed
#define SCROLL_INCREASE 0.1 // Scroll speed increase per score
#define BASE_GRAVITY 0.4    // Base gravity
#define GRAVITY_INCREASE 0.01 // Gravity increase per score
#define OBSTACLE_SPEED 2


// <<< NEW: Obstacle Constants >>>
#define MAX_OBSTACLES 2
#define OBSTACLE_RADIUS 8
#define OBSTACLE_COLOR RED
#define OBSTACLE_START_SCORE 5
#define OBSTACLE_SPAWN_CHANCE 3 // 3% chance per frame
#define OBSTACLE_SPEED 2
#define MIN_OBSTACLE_DISTANCE 40

// --- Game States ---
#define MAIN_MENU 0
#define LEVEL_MENU 1
#define HIGHSCORES_MENU 2
#define CREDITS_MENU 3
#define GAME_PLAYING 4
#define GAME_OVER_STATE 5

// --- Color Definitions ---
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define BLUE 0x001F
#define RED 0xF800
#define YELLOW 0xFFE0
#define MAGENTA 0xF81F
#define CYAN 0x07FF


#define LEVEL_EASY 0
#define LEVEL_MEDIUM 1
#define LEVEL_HARD 2

struct GameState {
  int doodlerX, doodlerY;
  float doodlerVelocityY;
  int platformX[NUM_PLATFORMS];
  int platformY[NUM_PLATFORMS];
  int platformDirection[NUM_PLATFORMS]; // Your struct includes this
  int score = 0;
  bool gameOver = true;
  bool platformUsed[NUM_PLATFORMS];
  int prevDoodlerX, prevDoodlerY;
  int prevPlatformX[NUM_PLATFORMS];
  int prevPlatformY[NUM_PLATFORMS];
  int gameLevel = 0;
  int selectedOption = 0; // Used by original readMenuInput, now new menu too
  bool levelSelected = false;
  int platformX_start;    // Your struct includes this
  int platformY_start;    // Your struct includes this
  bool plat_start_used;   // Your struct includes this
  bool gameStartedByUser; // Your struct includes this
  int visiblePlatforms;   // Your struct includes this
  float displayGravity; 
  // <<< NEW: Menu state variables >>>
  int currentMenu; // Current state (menu, game, etc.)
  int mainMenuSelection = 0; // For main menu navigation

  bool gameIsActive = false; // v6.3

  // <<< NEW: Obstacle variables >>>
  int obstacleX[MAX_OBSTACLES];
  int obstacleY[MAX_OBSTACLES];
  int obstacleDir[MAX_OBSTACLES]; // 1 = right, -1 = left
  bool obstacleActive[MAX_OBSTACLES];
  int prevObstacleX[MAX_OBSTACLES];
  int prevObstacleY[MAX_OBSTACLES];
};

const int easyAddress = 0;
const int medAddress = 2; // Assuming sizeof(int) == 2 on target
const int highAddress = 4;
const int INIT_MARKER_ADDR = 6;
const float HOMING_FACTOR = 0.8;

// --- Global Variables (from your code) ---
bool beat_easy = 0; // Used in original handleGameOver
bool beat_med = 0;  // Used in original handleGameOver
bool beat_hard = 0; // Used in original handleGameOver

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);


struct Player {
  int easy_score;
  int med_score;
  int hard_score;
  char name[24]; // Your struct includes this
};

// --- Global Struct Instances (from your code) ---
Player highScore = { 0,0,0,"The One Above All" };
Player currentScore = { 0,0,0,"Average Joe (YOU)" }; // Your code includes this
GameState state;

// --- Moving Platform Constants (from your code) ---
const int PLATFORM_MOVE_SPEED = 1;
const int PLATFORM_MOVE_RANGE = 50; // movement range

// <<< NEW: Flags for menu drawing state >>>
static bool mainMenuFirstDraw = true;
static bool levelMenuFirstDraw = true;
static bool highScoresDrawn = false;
static bool creditsDrawn = false;
static bool gameOverMsgDrawn = false;


// === Function Declarations ===
// Core & Menu (Some new, some modified)


// === EEPROM Function (Original from your code) ===
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
    EEPROM.get(easyAddress, highScore.easy_score);
    Serial.print("Easy Level High Score = "); Serial.println(highScore.easy_score);
    EEPROM.get(medAddress, highScore.med_score);
    Serial.print("Med Level High Score = "); Serial.println(highScore.med_score);
    EEPROM.get(highAddress, highScore.hard_score);
    Serial.print("Hard Level High Score = "); Serial.println(highScore.hard_score);
  }
}

// === Setup Function (MODIFIED for State Machine) ===
void setup() {
  Serial.begin(115200);
  Serial.println("v5 Base Code + New Menu Setup..."); // Identify version

  // Hardware Init (from your code)
  pinMode(TFT_RST, OUTPUT);
  pinMode(JOY_SW, INPUT_PULLUP);
  digitalWrite(TFT_RST, LOW); delay(10); digitalWrite(TFT_RST, HIGH); delay(10);

  tft.begin();
  tft.setRotation(0);
  SPI.setClockDivider(SPI_CLOCK_DIV2); // Keep as per your code

  // EEPROM Init (from your code)
  initializeEEPROM();
  // EEPROM gets from your code (redundant if initializeEEPROM does it, but keep if intended)
  EEPROM.get(easyAddress, highScore.easy_score);
  EEPROM.get(medAddress, highScore.med_score);
  EEPROM.get(highAddress, highScore.hard_score);


  // REMOVED from your setup: showStartScreen();
  // REMOVED from your setup: initGame();
  // REMOVED from your setup: Splash screen sequence (the fillScreen delays)

  // <<< NEW: Initialize state machine >>>
  state.currentMenu = MAIN_MENU; // Start at main menu
  state.mainMenuSelection = 0;
  state.gameOver = true; // Ensure game isn't running initially
  state.levelSelected = false; // Reset flag

  randomSeed(analogRead(A0)); // Seed random generator (use a floating pin like A0)
}

// === Main Loop (REPLACED with State Machine) ===
void loop() {
  switch (state.currentMenu) {
    case MAIN_MENU:
      drawMainMenu();
      readMenuInput(); // New non-blocking input handler
      break;
    case LEVEL_MENU:
      drawLevelMenu(); // New non-blocking draw handler
      readMenuInput(); // New non-blocking input handler
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
      // Use original game loop structure
      if (!state.gameOver) {
          // Original throttle logic
          static uint32_t lastUpdate = 0;
          if (millis() - lastUpdate < 33) return; // Original throttle
          lastUpdate = millis();

          updateGame(); // Calls ORIGINAL updateGame
          drawGame();   // Calls ORIGINAL drawGame
      } else {
          // If gameOver becomes true, transition state
          // updateHighScore logic moved into handleGameOver
          state.currentMenu = GAME_OVER_STATE;
          resetDrawFlags();
      }
      break;
    case GAME_OVER_STATE:
      handleGameOver();         // Display score, update EEPROM (Adapted from original)
      readMenuInput_GameOver(); // Wait for input to return to menu
      break;
  }
}


// === NEW Menu Drawing Functions ===
void resetDrawFlags() {
    mainMenuFirstDraw = true; levelMenuFirstDraw = true;
    highScoresDrawn = false; creditsDrawn = false; gameOverMsgDrawn = false;
}

void drawMainMenu() {
  static int lastSelection = -1;
  if (state.mainMenuSelection != lastSelection || mainMenuFirstDraw) {
    if (mainMenuFirstDraw) {
      tft.fillScreen(BLACK); tft.setTextColor(WHITE); tft.setTextSize(4);
      tft.setCursor(10, 30); tft.print("DOODLUINO"); tft.setTextSize(2);
      mainMenuFirstDraw = false;
    } else { tft.fillRect(35, 110, 170, 130, BLACK); }

    const char* menuItems[] = {"LEVEL SELECT", "HIGH SCORES", "CREDITS"};
    for (int i = 0; i < 3; i++) {
      int yPos = 120 + (i * 40);
      uint16_t fgColor = WHITE, bgColor = GREEN;
      if (i == state.mainMenuSelection) { tft.fillRoundRect(40, yPos - 5, 160, 30, 5, bgColor); fgColor = BLACK; }
      tft.setTextColor(fgColor);
      int textWidth = strlen(menuItems[i]) * 6 * 2;
      tft.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, yPos); tft.print(menuItems[i]);
    }
    lastSelection = state.mainMenuSelection;
  }
  tft.setCursor(180,300);
  tft.print("V6.8");
}

// REPLACED drawLevelMenu (New non-blocking implementation)
void drawLevelMenu() {
  static int lastSelection = -1;
  const int numOptions = 4; // Easy, Med, Hard, Back
  if (state.selectedOption != lastSelection || levelMenuFirstDraw) {
    if (levelMenuFirstDraw) {
       tft.fillScreen(BLACK); tft.setTextColor(WHITE); tft.setTextSize(3);
       tft.setCursor(30, 30); tft.print("SELECT LEVEL"); tft.setTextSize(2);
       levelMenuFirstDraw = false;
    } else { tft.fillRect(35, 110, 170, 170, BLACK); } // Clear menu area + back

    for (int i = 0; i < numOptions; i++) { // Loop includes Back option
      int yPos = 120 + (i * 40);
      bool isSelected = (i == state.selectedOption);
      uint16_t color = BLUE; // Default highlight for Back
      uint16_t textColor = WHITE;
      const char* optionText = "";

      if (isSelected) {
         if (i == LEVEL_EASY) color = GREEN; else if (i == LEVEL_MEDIUM) color = YELLOW; else if (i == LEVEL_HARD) color = RED;
         tft.fillRoundRect(40, yPos - 5, 160, 30, 5, color);
      }

      switch (i) {
        case LEVEL_EASY: optionText = "EASY"; textColor = (isSelected ? BLACK : WHITE); break;
        case LEVEL_MEDIUM: optionText = "MEDIUM"; textColor = (isSelected ? BLACK : BLUE); break;
        case LEVEL_HARD: optionText = "HARD"; textColor = (isSelected ? BLACK : WHITE); break;
        case 3: optionText = "BACK"; textColor = (isSelected ? WHITE : RED); break;
      }
      tft.setTextColor(textColor);
      int textWidth = strlen(optionText) * 6 * 2;
      tft.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, yPos); tft.print(optionText);
    }
    lastSelection = state.selectedOption;
  }
}

void drawHighScores() {
  if (!highScoresDrawn) {
      tft.fillScreen(BLACK); tft.setTextColor(WHITE); tft.setTextSize(2);
      tft.setCursor(20, 30); tft.print("HIGH SCORES");
      tft.setCursor(60, 80); tft.print("EASY: "); tft.print(highScore.easy_score);
      tft.setCursor(60, 120); tft.print("MEDIUM: "); tft.print(highScore.med_score);
      tft.setCursor(60, 160); tft.print("HARD: "); tft.print(highScore.hard_score);
      tft.setTextColor(RED); tft.setCursor(20, 220); tft.print("Press Select to BACK");
      highScoresDrawn = true;
  }
}

void drawCredits() {
  if (!creditsDrawn) {
      tft.fillScreen(BLACK); tft.setTextColor(WHITE); tft.setTextSize(3);
      tft.setCursor(60, 50); tft.print("CREDITS"); tft.setTextSize(2);
      tft.setCursor(30, 120); tft.print("Game By: LUKOG"); // Your name
      
      tft.setTextColor(RED); tft.setCursor(20, 220); tft.print("Press Select to BACK");
      creditsDrawn = true;
   }
}

// === NEW Input Handling Functions ===
void readMenuInput() {
  static uint32_t lastInputTime = 0; uint32_t now = millis(); if (now - lastInputTime < 180) return;
  int yValue = analogRead(JOY_Y); int swValue = digitalRead(JOY_SW); bool inputProcessed = false; int currentSelection = 0; int maxOption = 0;

  switch(state.currentMenu) {
      case MAIN_MENU: currentSelection = state.mainMenuSelection; maxOption = 2; break;
      case LEVEL_MENU: currentSelection = state.selectedOption; maxOption = 3; break;
      case HIGHSCORES_MENU: case CREDITS_MENU: break; default: return;
  }
  if (state.currentMenu == MAIN_MENU || state.currentMenu == LEVEL_MENU) {
      // Use thresholds consistent with original readMenuInput if different
      if (yValue < 400) { currentSelection++; if (currentSelection > maxOption) currentSelection = 0; inputProcessed = true; }
      else if (yValue > 600) { currentSelection--; if (currentSelection < 0) currentSelection = maxOption; inputProcessed = true; }
      if (inputProcessed) { if(state.currentMenu == MAIN_MENU) state.mainMenuSelection = currentSelection; else if (state.currentMenu == LEVEL_MENU) state.selectedOption = currentSelection; lastInputTime = now; }
  }
  if (swValue == LOW) { handleMenuSelection(); lastInputTime = now + 250; } // Debounce after select
}

void handleMenuSelection() {
  int previousMenu = state.currentMenu;
  switch(state.currentMenu) {
      case MAIN_MENU:
          switch(state.mainMenuSelection) {
              case 0: state.currentMenu = LEVEL_MENU; state.selectedOption = 0; break;
              case 1: state.currentMenu = HIGHSCORES_MENU; break;
              case 2: state.currentMenu = CREDITS_MENU; break;
          } break;
      case LEVEL_MENU:
          if (state.selectedOption == 3) { state.currentMenu = MAIN_MENU; state.mainMenuSelection = 0; }
          else { state.gameLevel = state.selectedOption; state.levelSelected = true; state.currentMenu = GAME_PLAYING; tft.fillScreen(RED);
                  delay(2);
                  tft.fillScreen(YELLOW);
                  delay(2);
                  tft.fillScreen(GREEN);
                  delay(2);
                  tft.fillScreen(BLACK);initGame(); } // Calls ORIGINAL initGame
          break;
      case HIGHSCORES_MENU: case CREDITS_MENU: state.currentMenu = MAIN_MENU; state.mainMenuSelection = (previousMenu == HIGHSCORES_MENU) ? 1 : 2; break;
  }
  if (state.currentMenu != previousMenu) { resetDrawFlags(); }
}

void readMenuInput_GameOver() {
    static uint32_t lastInputTime_GO = 0; uint32_t now = millis(); if (now - lastInputTime_GO < 400) return;
    if (digitalRead(JOY_SW) == LOW) { state.currentMenu = MAIN_MENU; state.mainMenuSelection = 0; state.gameOver = true; lastInputTime_GO = now; resetDrawFlags(); }
}


// === ORIGINAL Game Logic Functions (Unchanged from your code) ===

// void playMusic() { // Original from your code
//   int tempo = 120;
//   int buzzer = BUZZER_PIN; // Use BUZZER_PIN
//   int gameIntro[] = { /* ... notes ... */
//       NOTE_B4, 16, NOTE_B5, 16, NOTE_FS5, 16, NOTE_DS5, 16, //1
//       NOTE_B5, 32, NOTE_FS5, -16, NOTE_DS5, 8, NOTE_C5, 16,
//       NOTE_C6, 16, NOTE_G6, 16, NOTE_E6, 16, NOTE_C6, 32, NOTE_G6, -16, NOTE_E6, 8,
//       NOTE_B4, 16,  NOTE_B5, 16,  NOTE_FS5, 16,   NOTE_DS5, 16,  NOTE_B5, 32,  //2
//       NOTE_FS5, -16, NOTE_DS5, 8,  NOTE_DS5, 32, NOTE_E5, 32,  NOTE_F5, 32,
//       NOTE_F5, 32,  NOTE_FS5, 32,  NOTE_G5, 32, NOTE_GS5, 32,  NOTE_A5, 16, NOTE_B5, 8};
//   int notes = sizeof(gameIntro) / sizeof(gameIntro[0]) / 2;
//   int wholenote = (60000 * 4) / tempo;
//   int divider = 0, noteDuration = 0;
//   for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
//     divider = gameIntro[thisNote + 1];
//     if (divider > 0) { noteDuration = (wholenote) / divider; }
//     else if (divider < 0) { noteDuration = (wholenote) / abs(divider); noteDuration *= 1.5; }
//     tone(buzzer, gameIntro[thisNote], noteDuration * 0.9);
//     delay(noteDuration); // Note: delay() will block other actions
//     noTone(buzzer);
//   }
// }


void updateGame() { // Modify existing updateGame

  // Store previous positions first
  state.prevDoodlerX = state.doodlerX;
  state.prevDoodlerY = state.doodlerY;

  // <<< NEW: Check if the game is active >>>
  if (!state.gameIsActive) {
    // --- PRE-GAME IDLE STATE ---

    // 1. Check for Start Trigger (Joystick Button Press)
    if (digitalRead(JOY_SW) == LOW) {
        state.gameIsActive = true; // Start the game!
        state.doodlerVelocityY = JUMP_FORCE; // Apply initial full jump force
        Serial.println("Game Started!"); // Debug message
        // Optional: Play a start sound
        return; // Skip rest of update for this frame
    }

    // 2. Idle Bounce Physics (simple bounce on platform 0)
    const float IDLE_GRAVITY = 0.1f; // Very weak gravity for idle
    const float IDLE_JUMP = -1.5f;   // Small upward bounce velocity

    state.doodlerVelocityY += IDLE_GRAVITY; // Apply weak gravity
    state.doodlerY += state.doodlerVelocityY; // Update position

    // Collision check ONLY with platform 0 (the starting one)
    int i = 0; // Index of the starting platform
    bool xOverlap = (state.doodlerX + DOODLER_WIDTH > state.platformX[i]) &&
                    (state.doodlerX < state.platformX[i] + PLATFORM_WIDTH);
    // Simplified Y check for idle bounce (are feet at or below platform top?)
    bool yLanded = (state.doodlerY + DOODLER_HEIGHT >= state.platformY[i]);

    // Apply bounce only if falling and landed
    if (xOverlap && yLanded && state.doodlerVelocityY >= 0) {
        state.doodlerVelocityY = IDLE_JUMP; // Apply small bounce
        state.doodlerY = state.platformY[i] - DOODLER_HEIGHT; // Snap to top
    }

    // Make sure regular platforms update their prev positions but don't move
    // Call original updatePlatforms but it should ideally do nothing if level==easy
    // or if platformDirection is 0 for the relevant platforms
    updatePlatforms(); // Keep original call - needed to update prevX/Y for drawing

  } else {
    // --- ACTIVE GAME STATE (Your Original v5 Logic) ---

    // Input handling (Horizontal)
    int joy = analogRead(JOY_X);
    // Use the horizontal control logic from your v5 code:
    state.doodlerX += (joy < 400) ? -5 : (joy > 600) ? +5 : 0; // Assuming this is your v5 logic
    // Screen wrap
    if (state.doodlerX < 0) state.doodlerX = SCREEN_WIDTH - DOODLER_WIDTH;
    if (state.doodlerX > SCREEN_WIDTH - DOODLER_WIDTH) state.doodlerX = 0;

    // Physics (Normal Gravity - using your original dynamic gravity)
    float dynamicGravity = BASE_GRAVITY + (state.score * GRAVITY_INCREASE);
    state.displayGravity = dynamicGravity;
    state.doodlerVelocityY += dynamicGravity;
    // Apply terminal velocity (optional)
    // state.doodlerVelocityY = min(state.doodlerVelocityY, 15.0f);
    state.doodlerY += state.doodlerVelocityY;

    // Standard Game Logic Calls (Your Original Functions)
    updateObstacles();
    checkCollisions();
    handleScrolling();
    updatePlatforms(); // Platforms will move now based on level/direction
    checkGameOver();
  }
}

void updatePlatforms() {
  int moveSpeed = PLATFORM_MOVE_SPEED;

  // Adjust speed based on level and score
  if (state.gameLevel == LEVEL_HARD) {
    moveSpeed += state.score * 0.25;
  }

  for (int i = 0; i < NUM_PLATFORMS; i++) {
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
}



void updateObstacles() {
  // Calculate current max obstacles based on score (1 + 1 per 10 points)
  int currentMaxObstacles = 1 + (state.score / 10);
  currentMaxObstacles = min(currentMaxObstacles, MAX_OBSTACLES);

  // Spawn new obstacles
  if (state.score >= OBSTACLE_START_SCORE && random(100) < OBSTACLE_SPAWN_CHANCE) {
    for (int i = 0; i < currentMaxObstacles; i++) { // Only check allowed slots
      if (!state.obstacleActive[i]) {
        state.obstacleActive[i] = true;
        state.obstacleX[i] = random(OBSTACLE_RADIUS, SCREEN_WIDTH - OBSTACLE_RADIUS);
        
        // Homing effect only in Hard mode
        if (state.gameLevel == LEVEL_HARD) {
          float horizontalDifference = state.doodlerX - state.obstacleX[i];
          if (horizontalDifference > 5) {
            state.obstacleX[i] += HOMING_FACTOR;
          } 
          else if (horizontalDifference < -5) {
            state.obstacleX[i] -= HOMING_FACTOR;
          }
          // Keep within bounds after adjustment
          state.obstacleX[i] = constrain(state.obstacleX[i], 
            OBSTACLE_RADIUS, 
            SCREEN_WIDTH - OBSTACLE_RADIUS);
        }
        
        state.obstacleY[i] = random(-PLATFORM_HEIGHT+20, 0);
        state.obstacleDir[i] = (random(2) ? 1 : -1);
        state.prevObstacleX[i] = state.obstacleX[i];
        state.prevObstacleY[i] = state.obstacleY[i];
        break;
      }
    }
  }

  // Move active obstacles - only in Hard mode
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (state.obstacleActive[i]) {
      state.prevObstacleX[i] = state.obstacleX[i];
      state.prevObstacleY[i] = state.obstacleY[i];
      
      // Only move obstacles in Hard difficulty
      if (state.gameLevel == LEVEL_HARD) {
        state.obstacleX[i] += OBSTACLE_SPEED * state.obstacleDir[i];
        
        // Bounce logic
        if (state.obstacleX[i] <= OBSTACLE_RADIUS || 
            state.obstacleX[i] >= SCREEN_WIDTH - OBSTACLE_RADIUS) {
          state.obstacleDir[i] *= -1;
          // Prevent sticking at edges
          state.obstacleX[i] = constrain(state.obstacleX[i],
            OBSTACLE_RADIUS,
            SCREEN_WIDTH - OBSTACLE_RADIUS);
        }
      }
    }
  }
}
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
    tft.setTextColor(WHITE);
    tft.print("Score: ");
    
    tft.print(state.score);
    lastScore = state.score;
    // tft.setf
    tft.setCursor(150,5);
    tft.print("g: ");
    tft.print(state.displayGravity);

  }

  // Clear previous doodler position
  tft.fillRect(state.prevDoodlerX, state.prevDoodlerY + TOP_OFFSET,
               DOODLER_WIDTH, DOODLER_HEIGHT, BLACK);
  // Draw new doodler position
  tft.fillRect(state.doodlerX, state.doodlerY + TOP_OFFSET,
               DOODLER_WIDTH, DOODLER_HEIGHT, WHITE);
  
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
  if (!state.gameIsActive) {
        tft.setTextColor(YELLOW);
        tft.setTextSize(2);
        const char* startText = "Press Start!";
        int16_t x1, y1;
        uint16_t w, h;
        tft.getTextBounds(startText, 0, 0, &x1, &y1, &w, &h); // Get text bounds
        // Position near bottom center
        tft.setCursor(SCREEN_WIDTH / 2 - w / 2, SCREEN_HEIGHT - h - 10);
        tft.print(startText);
    }

  // <<< NEW: Draw obstacles >>>
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (state.obstacleActive[i]) {
      // Clear old position
      tft.fillCircle(state.prevObstacleX[i], 
                    state.prevObstacleY[i] + TOP_OFFSET,
                    OBSTACLE_RADIUS, BLACK);
      
      // Draw new position
      tft.fillCircle(state.obstacleX[i],
                    state.obstacleY[i] + TOP_OFFSET,
                    OBSTACLE_RADIUS, OBSTACLE_COLOR);
    }
  }
}

void checkCollisions() {
  for (int i = 0; i < NUM_PLATFORMS; i++) {
    if (state.platformY[i] + TOP_OFFSET >= 0 && state.platformY[i] + TOP_OFFSET < SCREEN_HEIGHT) { // Visibility check
      // Collision check logic
      if (state.doodlerVelocityY > 0 &&
          state.doodlerX + DOODLER_WIDTH > state.platformX[i] &&
          state.doodlerX < state.platformX[i] + PLATFORM_WIDTH &&
          state.prevDoodlerY + DOODLER_HEIGHT <= state.platformY[i] &&
          state.doodlerY + DOODLER_HEIGHT >= state.platformY[i]) {

        // Dynamic jump force (as per your original code)
        float dynamicJump = JUMP_FORCE - (state.score * 0.1);
        state.doodlerVelocityY = dynamicJump;
        state.doodlerY = state.platformY[i] - DOODLER_HEIGHT; // Snap to top

        if (!state.platformUsed[i]) { state.score++; state.platformUsed[i] = true; }
        // return; // Original code didn't have return, check implications
      }
    }
  }

  // <<< NEW: Obstacle collisions >>>
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (state.obstacleActive[i]) {
      // Simple Axis-Aligned Bounding Box (AABB) check for Rect-Circle
      // More robust and common than the previous distance check.

      // Find closest point on doodler rectangle to circle center
      float closestX = max((float)state.doodlerX, min((float)state.obstacleX[i], (float)(state.doodlerX + DOODLER_WIDTH)));
      float closestY = max((float)state.doodlerY, min((float)state.obstacleY[i], (float)(state.doodlerY + DOODLER_HEIGHT)));

      // Calculate distance squared between circle center and closest point
      float dx = state.obstacleX[i] - closestX;
      float dy = state.obstacleY[i] - closestY;
      float distanceSquared = (dx * dx) + (dy * dy);

      // If distance is less than radius squared, collision!
      if (distanceSquared < (OBSTACLE_RADIUS * OBSTACLE_RADIUS)) {
        state.gameOver = true;
        // You might want a sound effect here
        // tone(BUZZER_PIN, NOTE_C4, 100); // Example collision sound
        return; // Exit collision check early
      }
    }
  }

}

// ========== INITIALIZATION ========== //
void initGame() {
  state.score = 0;
  state.gameOver = false;
  state.gameIsActive = false; // <<< Game doesn't start immediately
  state.levelSelected = true; // Mark level as selected if needed

  // --- Platform Initialization (Keep your original logic) ---
  state.plat_start_used = false; // From original v5 initGame
  state.platformX_start = state.doodlerX; // From original v5 initGame
  state.platformY_start = state.doodlerY - PLAY_AREA_HEIGHT / 2 + 5; // From original v5 initGame
  state.visiblePlatforms = VISIBLE_PLATFORMS; // From original v5 initGame

  for (int i = 0; i < NUM_PLATFORMS; i++) {
    state.plat_start_used = true; // From original v5 initGame
    state.platformUsed[i] = false;
    state.platformX[i] = random(SCREEN_WIDTH - PLATFORM_WIDTH);
    state.platformY[i] = PLAY_AREA_HEIGHT - (i * (PLAY_AREA_HEIGHT / NUM_PLATFORMS));
    if (state.gameLevel == LEVEL_EASY) { state.platformDirection[i] = 0; }
    else { state.platformDirection[i] = (random(2) == 0) ? 1 : -1; }
    state.prevPlatformX[i] = state.platformX[i];
    state.prevPlatformY[i] = state.platformY[i];
  }
   // --- END Platform Initialization ---


  // <<< NEW: Place Doodler ON the starting platform (platform[0]) >>>
  // Ensure platform 0 is positioned reasonably for start
  state.platformX[0] = SCREEN_WIDTH / 2 - PLATFORM_WIDTH / 2; // Center first platform
  state.platformY[0] = PLAY_AREA_HEIGHT - 60; // Place it relatively low
  state.prevPlatformX[0] = state.platformX[0]; // Update its prev position too
  state.prevPlatformY[0] = state.platformY[0];

  // Set doodler position based on platform 0
  state.doodlerX = state.platformX[0] + (PLATFORM_WIDTH / 2) - (DOODLER_WIDTH / 2);
  state.doodlerY = state.platformY[0] - DOODLER_HEIGHT; // Place doodler feet on platform 0

  state.doodlerVelocityY = -1.5; // <<< Small initial upward velocity for idle bounce

  state.prevDoodlerX = state.doodlerX;
  state.prevDoodlerY = state.doodlerY;

  resetDrawFlags();

  // Initialize obstacles
  

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    state.obstacleActive[i] = false;
    state.obstacleX[i] = -100; // Off-screen
    state.obstacleY[i] = -100;
    state.obstacleDir[i] = (random(2) ? 1 : -1); // Random initial direction
  }
}

void handleGameOver() {
    // Update High Score (using exact logic from original handleGameOver)
    bool highScoreUpdated = false;
    // Reset local flags each time entering this state
    beat_easy = 0; beat_med = 0; beat_hard = 0;

    if (state.gameLevel == LEVEL_EASY) {
        if (state.score > highScore.easy_score) { highScore.easy_score = state.score; beat_easy = 1; highScoreUpdated = true; }
    } else if (state.gameLevel == LEVEL_MEDIUM) {
        if (state.score > highScore.med_score) { highScore.med_score = state.score; beat_med = 1; highScoreUpdated = true; }
    } else if (state.gameLevel == LEVEL_HARD) {
        if (state.score > highScore.hard_score) { highScore.hard_score = state.score; beat_hard = 1; highScoreUpdated = true; }
    }

    // EEPROM update (using exact logic from original handleGameOver)
    if (highScoreUpdated) {
        int addressToUpdate = 0; int scoreToSave = 0;
        if (state.gameLevel == LEVEL_EASY) { addressToUpdate = easyAddress; scoreToSave = highScore.easy_score; }
        else if (state.gameLevel == LEVEL_MEDIUM) { addressToUpdate = medAddress; scoreToSave = highScore.med_score; }
        else { addressToUpdate = highAddress; scoreToSave = highScore.hard_score; }
        // Use EEPROM.update as per original code
        // Assuming score fits in 2 bytes for original EEPROM logic
        EEPROM.update(addressToUpdate, lowByte(scoreToSave));
        EEPROM.update(addressToUpdate + 1, highByte(scoreToSave));
        // Verification (as per original code)
        int readBackValue; EEPROM.get(addressToUpdate, readBackValue); // Use get for verification
        if (readBackValue != scoreToSave) { Serial.print("EEPROM Write Error! Addr: "); /* ... */ }
        else { Serial.println("EEPROM Save Verified."); }
    }

    // Draw Game Over Screen (only once per entry using global flag)
    if (!gameOverMsgDrawn) {
        tft.fillScreen(BLACK);
        tft.setTextSize(2);
        tft.setCursor(50, 50); tft.setTextColor(RED); tft.print("GAME OVER :("); // Original text
        tft.setCursor(50, 100); tft.setTextColor(WHITE); tft.print("You scored: "); tft.print(state.score);
        if (highScoreUpdated) { tft.setTextColor(YELLOW); tft.setCursor(40, 130); tft.print("New High Score!"); } // Optional msg
        // Original prompt text
        tft.setTextColor(WHITE); tft.setCursor(40, 150); tft.print("Click joystick "); tft.setCursor(60, 180); tft.print("to restart"); // Original text
        gameOverMsgDrawn = true;
    }
    // --- REMOVED blocking while loop ---
    // --- REMOVED call to RestartGame ---
    // Input checking happens in readMenuInput_GameOver in the main loop
}

void checkGameOver() { // Original from your code
  if (state.doodlerY > SCREEN_HEIGHT)
    state.gameOver = true;
}


void handleScrolling() {
  float dynamicScroll = BASE_SCROLL + (state.score * SCROLL_INCREASE);
  if (state.doodlerY < PLAY_AREA_HEIGHT / 3) {
    int scroll = min(PLAY_AREA_HEIGHT / 3 - state.doodlerY, (int)dynamicScroll);
    state.doodlerY += scroll;

    // Scroll platforms
    for (int i = 0; i < NUM_PLATFORMS; i++) {
      state.platformY[i] += scroll;
      
      // Regenerate platforms that scroll off bottom
      if (state.platformY[i] > PLAY_AREA_HEIGHT) {
        state.platformX[i] = random(SCREEN_WIDTH - PLATFORM_WIDTH);
        state.platformY[i] = random(-PLATFORM_HEIGHT, 0); // Spawn above screen
        state.platformUsed[i] = false;
        if (state.gameLevel != LEVEL_EASY) {
          state.platformDirection[i] = (random(2) ? 1 : -1);
        }
      }
    }

    // Scroll obstacles
    for (int i = 0; i < MAX_OBSTACLES; i++) {
      if (state.obstacleActive[i]) {
        state.obstacleY[i] += scroll;
        
        // Remove obstacles that scroll off bottom
        if (state.obstacleY[i] > PLAY_AREA_HEIGHT) {
          state.obstacleActive[i] = false;
        }
      }
    }
  }
}



