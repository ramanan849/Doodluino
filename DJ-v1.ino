// Version 1 - skeletal non-optimized game 

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// pins for the TFT display
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8

// pin for the joystick
#define JOY_X A5 // left - right control only

// Screen dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// Colors
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define RED 0xF800
#define MAGENTA 0xF81F

// Game properties
#define DOODLER_WIDTH 15
#define DOODLER_HEIGHT 15
#define PLATFORM_WIDTH 40
#define PLATFORM_HEIGHT 8
#define NUM_PLATFORMS 16

// Initialize display with hardware SPI
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Game variables
int doodlerX, doodlerY;
float doodlerVelocityY;
int screenOffset;
int platformX[NUM_PLATFORMS];
int platformY[NUM_PLATFORMS];
int score = 0;
bool gameOver = false;

// Previous positions for partial updates
int prevDoodlerX, prevDoodlerY;
int prevPlatformX[NUM_PLATFORMS];
int prevPlatformY[NUM_PLATFORMS];

void setup() {
  Serial.begin(115200); // why 115200 ? 
  
  // Hardware reset
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(10);
  digitalWrite(TFT_RST, HIGH);
  delay(10);
  
  // Initialize display
  tft.begin();
  tft.setRotation(0); // Portrait mode
  
  // Increase SPI speed for faster drawing
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  
  // Initial screen
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(40, 120);
  tft.println("DOODLUINO");
  tft.println("Doodle Jump on Arduino");
  tft.setCursor(30, 150);
  tft.println("Press joystick");
  tft.setCursor(50, 170);
  tft.println("to start");
  
  // Wait for joystick input
  while (analogRead(JOY_X) > 400 && analogRead(JOY_X) < 600) {
    delay(50);
  }
  
  // Initialize game
  initGame();
}

void initGame() {
  // Clearing screen
  tft.fillScreen(BLACK);
  
  // Initialize doodler
  doodlerX = SCREEN_WIDTH / 2 - DOODLER_WIDTH / 2;
  doodlerY = SCREEN_HEIGHT / 2;
  doodlerVelocityY = 0;
  prevDoodlerX = doodlerX;
  prevDoodlerY = doodlerY;
  
  // Initialize platforms
  screenOffset = 0;
  for (int i = 0; i < NUM_PLATFORMS; i++) {
    platformX[i] = random(SCREEN_WIDTH - PLATFORM_WIDTH);
    platformY[i] = SCREEN_HEIGHT - (SCREEN_HEIGHT / NUM_PLATFORMS) * i - 30;
    prevPlatformX[i] = platformX[i];
    prevPlatformY[i] = platformY[i];
  }
  
  // Reset score
  score = 0;
  gameOver = false;
}

void loop() {
  if (!gameOver) {
    // Read joystick
    int joystickValue = analogRead(JOY_X);
    
    // Clear previous doodler position
    tft.fillRect(prevDoodlerX, prevDoodlerY, DOODLER_WIDTH, DOODLER_HEIGHT, BLACK);
    
    // Move doodler horizontally
    prevDoodlerX = doodlerX;
    if (joystickValue < 400) { // Move left
      doodlerX -= 5;
    } else if (joystickValue > 600) { // Move right
      doodlerX += 5;
    }
    
    // Screen wrap
    if (doodlerX < 0) {
      doodlerX = SCREEN_WIDTH - DOODLER_WIDTH;
    } else if (doodlerX > SCREEN_WIDTH - DOODLER_WIDTH) {
      doodlerX = 0;
    }
    
    // Apply gravity
    doodlerVelocityY += 0.4;
    prevDoodlerY = doodlerY;
    doodlerY += doodlerVelocityY;
    
    // Check platform collisions
    for (int i = 0; i < NUM_PLATFORMS; i++) {
      if (doodlerVelocityY > 0 && 
          doodlerX + DOODLER_WIDTH > platformX[i] && 
          doodlerX < platformX[i] + PLATFORM_WIDTH &&
          prevDoodlerY + DOODLER_HEIGHT <= platformY[i] && 
          doodlerY + DOODLER_HEIGHT >= platformY[i]) {
        doodlerVelocityY = -10; // Jump
        score++;
      }
    }
    
    // Scroll screen when doodler is high enough
    if (doodlerY < SCREEN_HEIGHT / 3) {
      int scrollAmount = SCREEN_HEIGHT / 3 - doodlerY;
      doodlerY = SCREEN_HEIGHT / 3;
      
      for (int i = 0; i < NUM_PLATFORMS; i++) {
        // Clear previous platform
        tft.fillRect(prevPlatformX[i], prevPlatformY[i], PLATFORM_WIDTH, PLATFORM_HEIGHT, BLACK);
        
        prevPlatformY[i] = platformY[i];
        platformY[i] += scrollAmount;
        
        // If platform is off screen, create a new one at the top
        if (platformY[i] > SCREEN_HEIGHT) {
          prevPlatformX[i] = platformX[i];
          platformX[i] = random(SCREEN_WIDTH - PLATFORM_WIDTH);
          platformY[i] = 0;
        }
      }
    }
    
    // Game over if doodler falls off screen
    if (doodlerY > SCREEN_HEIGHT) {
      gameOver = true;
    }
    
    // Draw platforms (only if they've moved)
    for (int i = 0; i < NUM_PLATFORMS; i++) {
      if (prevPlatformX[i] != platformX[i] || prevPlatformY[i] != platformY[i]) {
        tft.fillRect(prevPlatformX[i], prevPlatformY[i], PLATFORM_WIDTH, PLATFORM_HEIGHT, BLACK);
        prevPlatformX[i] = platformX[i];
        prevPlatformY[i] = platformY[i];
      }
      tft.fillRect(platformX[i], platformY[i], PLATFORM_WIDTH, PLATFORM_HEIGHT, GREEN);
    }
    
    // Draw doodler
    tft.fillRect(doodlerX, doodlerY, DOODLER_WIDTH, DOODLER_HEIGHT, WHITE);
    
    // Display score
    tft.fillRect(5, 5, 100, 20, BLACK);
    tft.setCursor(5, 5);
    tft.print("Score: ");
    tft.print(score);
    
  } else {
    // Game over screen
    tft.fillScreen(BLACK);
    tft.setTextSize(2);
    tft.setCursor(70, 100);
    tft.println("GAME OVER");
    tft.setCursor(70, 130);
    tft.print("Score: ");
    tft.println(score);
    tft.setCursor(30, 170);
    tft.println("Press to restart");
    
    delay(1000); // Debounce
    
    // Wait for joystick input to restart
    while (analogRead(JOY_X) > 400 && analogRead(JOY_X) < 600) {
      delay(50);
    }
    
    initGame();
  }
  
  // Control frame rate
  delay(20); // observed that decreasing it doesn't make much difference, like from 20 ms to 5 ms is hardly noticeably

}
