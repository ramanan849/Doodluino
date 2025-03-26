#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define JOY_X A5

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define DOODLER_WIDTH 15
#define DOODLER_HEIGHT 28
#define PLATFORM_WIDTH 40
#define PLATFORM_HEIGHT 8
#define NUM_PLATFORMS 4
#define MAX_SCROLL 5    // Smoother scrolling
#define JUMP_FORCE -12
#define GRAVITY 0.4
#define TOP_OFFSET 30  // Height of score panel - the separate one
#define PLAY_AREA_HEIGHT (SCREEN_HEIGHT - TOP_OFFSET)
#define VISIBLE_PLATFORMS 3
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

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// Game state with position tracking
struct GameState {
  int doodlerX, doodlerY;
  float doodlerVelocityY;
  int platformX[NUM_PLATFORMS];
  int platformY[NUM_PLATFORMS];
  int platformDirection[NUM_PLATFORMS]; // 1 for right, -1 for left
  int score = 0;
  bool gameOver = true;
  bool platformUsed[NUM_PLATFORMS];
  int prevDoodlerX, prevDoodlerY;
  int prevPlatformX[NUM_PLATFORMS];
  int prevPlatformY[NUM_PLATFORMS];
};

GameState state;

// New parameters for moving platforms
const int PLATFORM_MOVE_SPEED = 1;
const int PLATFORM_MOVE_RANGE = 50; // movement range

void setup() {
  Serial.begin(115200);

  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(10);
  digitalWrite(TFT_RST, HIGH);
  delay(10);

  tft.begin();
  tft.setRotation(0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  showStartScreen();
  initGame();
}

void showStartScreen() {
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(40, 120);
  tft.print("DOODLUINO");
  tft.setCursor(30, 150);
  tft.print("Press joystick");
  while (analogRead(JOY_X) > 400 && analogRead(JOY_X) < 600) delay(50);
  tft.fillScreen(BLACK);
}

void initGame() {
  state.doodlerX = SCREEN_WIDTH / 2 - DOODLER_WIDTH / 2;
  state.doodlerY = PLAY_AREA_HEIGHT / 2;
  state.doodlerVelocityY = 0;
  state.score = 0;
  state.gameOver = false;

  // Initialize platforms
  for (int i = 0; i < NUM_PLATFORMS; i++) {
    state.platformUsed[i] = false;
    state.platformX[i] = random(SCREEN_WIDTH - PLATFORM_WIDTH);
    state.platformY[i] = PLAY_AREA_HEIGHT - (i * (PLAY_AREA_HEIGHT / NUM_PLATFORMS));
    state.prevPlatformX[i] = state.platformX[i];
    state.prevPlatformY[i] = state.platformY[i];
    state.platformDirection[i] = (random(2) == 0) ? 1 : -1;  // Random direction
  }
}

void loop() {
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate < 33) return;
  lastUpdate = millis();

  if (!state.gameOver) {
    updateGame();
    drawGame();
  } else {
    handleGameOver();
  }
}

void updateGame() {
  // Input handling
  int joy = analogRead(JOY_X);
  state.prevDoodlerX = state.doodlerX;
  state.doodlerX += (joy < 400) ? +5 : (joy > 600) ? -5 : 0;
  if (state.doodlerX < 0) state.doodlerX = SCREEN_WIDTH - DOODLER_WIDTH;
  if (state.doodlerX > SCREEN_WIDTH - DOODLER_WIDTH) state.doodlerX = 0;

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
}

void updatePlatforms() {
  for (int i = 0; i < NUM_PLATFORMS; i++) {
    state.platformX[i] += PLATFORM_MOVE_SPEED * state.platformDirection[i];

    // Check boundaries and reverse direction if needed
    if (state.platformX[i] <= 0) {
      state.platformDirection[i] = 1; // Move right
    } else if (state.platformX[i] >= SCREEN_WIDTH - PLATFORM_WIDTH) {
      state.platformDirection[i] = -1; // Move left
    }
  }
}

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
}

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
        state.platformDirection[i] = (random(2) == 0) ? 1 : -1; //new direction
      }
    }
  }
}

void checkGameOver() {
  if (state.doodlerY > SCREEN_HEIGHT) state.gameOver = true;
}

void drawGame() {
  static int lastScore = -1;
  if (state.score != lastScore) {
    if (state.score <= 10) {
      tft.fillRect(0, 0, SCREEN_WIDTH, TOP_OFFSET - 10, RED);
    }
    else if (state.score > 10 && state.score <= 30) {
      tft.fillRect(0, 0, SCREEN_WIDTH, TOP_OFFSET - 10, BLUE);
    }
    else if (state.score > 30 && state.score < 50) {
      tft.fillRect(0, 0, SCREEN_WIDTH, TOP_OFFSET - 10, CYAN);
    }
    else if (state.score >= 50 && state.score <= 89) {
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

  // Update platforms
  for (int i = 0; i < NUM_PLATFORMS; i++) {
    if (state.platformY[i] + TOP_OFFSET >= 0 &&
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
}

void handleGameOver() {
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  tft.setCursor(50, 50);
  tft.print("GAME OVER :(");
  tft.setCursor(50, 100);
  tft.print("You scored: ");
  tft.print(state.score);
  tft.setCursor(40, 150);
  tft.print("Press joystick");
  tft.setCursor(60, 180);
  tft.print("to restart");

  delay(1000);
  while (analogRead(JOY_X) > 400 && analogRead(JOY_X) < 600) delay(50);
  setup();
}
