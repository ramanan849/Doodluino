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
#define NUM_PLATFORMS 6
#define MAX_SCROLL 5    // Smoother scrolling
#define JUMP_FORCE -10
#define GRAVITY 0.4

#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define RED 0xF800
#define MAGENTA 0xF81F

/*
The strctures defined by me:
1. GameState
  - struct object = state

The functions defined: 
1. showStartScreen()
2. initGame()
3. updateGame()
4. checkCollisions()
5. checkGameOver()
6. drawGame()
7. checkGameOver()
8. handleGameOver()

*/
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// Game state with position tracking
struct GameState {
  int doodlerX, doodlerY; // the doodler positions
  float doodlerVelocityY;
  int platformX[NUM_PLATFORMS];
  int platformY[NUM_PLATFORMS];
  int score = 0;
  bool gameOver = true;
  bool platformUsed[NUM_PLATFORMS]; // NEW addition
  // Previous positions
  int prevDoodlerX, prevDoodlerY;
  int prevPlatformX[NUM_PLATFORMS];
  int prevPlatformY[NUM_PLATFORMS];
};

GameState state; // an object with name "state"

void setup() {
  Serial.begin(115200);
  
  // Hardware reset
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(10);
  digitalWrite(TFT_RST, HIGH);
  delay(10);
  
  tft.begin();
  tft.setRotation(0); // portrait mode
  SPI.setClockDivider(SPI_CLOCK_DIV2); // 8MHz SPI // the fastest
  
  showStartScreen();
  initGame();
}
// fn 1
void showStartScreen() { // a vooid function that gets called in the loop

  // Not in the top priority now to change 
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(40, 120);
  tft.print("DOODLUINO");
  tft.setCursor(30, 150);
  tft.print("Press joystick");
  while(analogRead(JOY_X) > 400 && analogRead(JOY_X) < 600) delay(50);
  tft.fillScreen(BLACK);
}
// fn 2
void initGame() { // again a void function
  // seems OK
  state.doodlerX = SCREEN_WIDTH/2 - DOODLER_WIDTH/2;
  state.doodlerY = SCREEN_HEIGHT/2;
  state.doodlerVelocityY = 0;
  state.score = 0;
  state.gameOver = false;

  // Initialize platforms with better distribution 
  for(int i=0; i<NUM_PLATFORMS; i++) {
    state.platformUsed[i] = false; // NEW ADDITION - 19/03
    state.platformX[i] = random(SCREEN_WIDTH - PLATFORM_WIDTH);
    state.platformY[i] = SCREEN_HEIGHT - (i * (SCREEN_HEIGHT/NUM_PLATFORMS));
    state.prevPlatformX[i] = state.platformX[i];
    state.prevPlatformY[i] = state.platformY[i];
  }
}

void loop() {
  static uint32_t lastUpdate = 0; // constant number, zero
  if(millis() - lastUpdate < 33) return; // ~30 FPS - why?? // millis gives the no. of milli seconds elapsed since program start 
  // i can just change it to millis() < 33, huh? since lastUpdate is always 0? 
  lastUpdate = millis(); 

  if(!state.gameOver) { // if gameOver  = false, then call updateGame and drawGame to play the game
    updateGame();
    drawGame();
  } else {
    handleGameOver(); // else, if gameOver = true, go to the handlegameover fn to handle the death situation
  }
}
// fn 3
void updateGame() {
  // Input handling
  int joy = analogRead(JOY_X);
  state.prevDoodlerX = state.doodlerX;
  state.doodlerX += (joy < 400) ? -5 : (joy > 600) ? 5 : 0; // this is power; try making it a user changeable parameter
  // -5 for L and +5 for R; reduce or increase uniformly on both L and R 


  // Screen wrapping 
  if(state.doodlerX < 0) state.doodlerX = SCREEN_WIDTH - DOODLER_WIDTH; // if you're on the left edge, transport the doodler to the right most edge & keep the same sign for velocity (-ve on the left, -ve on the right)
  if(state.doodlerX > SCREEN_WIDTH - DOODLER_WIDTH) state.doodlerX = 0; // same as above, but for right edge, transport to the left edge and keep same sign of velocity (+ve on the right, still +ve on the left)

  // Physics bod!!!
  state.prevDoodlerY = state.doodlerY;
  state.doodlerVelocityY += GRAVITY; // change 
  state.doodlerY += state.doodlerVelocityY; 

  /*
  the above three lines: 
  prev_pos_y = pos_y, then
  y_vel = y_vel + gravity
  pos_y = pos_y + y_vel  
  */
  
  checkCollisions();
  handleScrolling();
  checkGameOver();
}
// fn 4
void checkCollisions() {
  // need to change the score updation logic; right now, it is like, score+=1 after every successful detected jump, so it repeats; i  have to change it such that each platform is unique, add something like boolean visited or not
  // for(int i=0; i<NUM_PLATFORMS; i++) {
  //   if(state.doodlerVelocityY > 0 && 
  //      state.doodlerX + DOODLER_WIDTH > state.platformX[i] &&
  //      state.doodlerX < state.platformX[i] + PLATFORM_WIDTH &&
  //      state.prevDoodlerY + DOODLER_HEIGHT <= state.platformY[i] && 
  //      state.doodlerY + DOODLER_HEIGHT >= state.platformY[i]) {
  //     state.doodlerVelocityY = JUMP_FORCE;
  //     state.score++;
  //   }
  // } 

  // NEW for loop to change score update logic - 19/03

  for(int i=0; i<NUM_PLATFORMS; i++) {
    if(state.doodlerVelocityY > 0 && 
       state.doodlerX + DOODLER_WIDTH > state.platformX[i] &&
       state.doodlerX < state.platformX[i] + PLATFORM_WIDTH &&
       state.prevDoodlerY + DOODLER_HEIGHT <= state.platformY[i] && 
       state.doodlerY + DOODLER_HEIGHT >= state.platformY[i]) 
    {
      state.doodlerVelocityY = JUMP_FORCE;
      
      // Only score if platform hasn't been used yet
      if(!state.platformUsed[i]) {
        state.score++;
        state.platformUsed[i] = true;
      }
    }
  }
}
// fn 5
void handleScrolling() {
  if(state.doodlerY < SCREEN_HEIGHT/3) {
    int scroll = min(SCREEN_HEIGHT/3 - state.doodlerY, MAX_SCROLL);
    state.doodlerY += scroll;
    
    for(int i=0; i<NUM_PLATFORMS; i++) {
      state.platformY[i] += scroll;
      if(state.platformY[i] > SCREEN_HEIGHT) {
        state.platformX[i] = random(SCREEN_WIDTH - PLATFORM_WIDTH);
        state.platformY[i] = random(-PLATFORM_HEIGHT, 0);
        state.platformUsed[i] = false;
      }
    }
  }
}
// fn 6
void checkGameOver() {
  if(state.doodlerY > SCREEN_HEIGHT) state.gameOver = true;
}
// fn 7
void drawGame() {
  // Clear previous positions
  tft.fillRect(state.prevDoodlerX, state.prevDoodlerY, DOODLER_WIDTH, DOODLER_HEIGHT, BLACK);
  
  // Draw platforms (only changed ones)
  for(int i=0; i<NUM_PLATFORMS; i++) {
    if(state.platformX[i] != state.prevPlatformX[i] || state.platformY[i] != state.prevPlatformY[i]) {
      tft.fillRect(state.prevPlatformX[i], state.prevPlatformY[i], PLATFORM_WIDTH, PLATFORM_HEIGHT, BLACK);
      state.prevPlatformX[i] = state.platformX[i];
      state.prevPlatformY[i] = state.platformY[i];
    }
    tft.fillRect(state.platformX[i], state.platformY[i], PLATFORM_WIDTH, PLATFORM_HEIGHT, GREEN);
  }

  // Draw doodler
  tft.fillRect(state.doodlerX, state.doodlerY, DOODLER_WIDTH, DOODLER_HEIGHT, WHITE);

  // Update score
  static int lastScore = -1;
  if(state.score != lastScore) {
    tft.fillRect(5, 5, 100, 20, BLACK);
    tft.setCursor(5, 5);
    tft.print("Score: ");
    tft.print(state.score);
    lastScore = state.score;
  }
}
// fn 8
void handleGameOver() {
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  tft.setCursor(70, 100);
  tft.print("GAME OVER");
  tft.setCursor(70, 130);
  tft.print("Score: ");
  tft.print(state.score);
  
  delay(1000);
  while(analogRead(JOY_X) > 400 && analogRead(JOY_X) < 600) delay(50);
  initGame();
}