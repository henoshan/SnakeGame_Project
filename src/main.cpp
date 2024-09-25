#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <EEPROM.h>

// display -----------------------------------------------------------------------------------------
#define TFT_CS    10  // Chip select pin for the ILI9341
#define TFT_RST   9   // Reset pin
#define TFT_DC    8   // Data/Command pin
//joystick -----------------------------------------------------------------------------------------
#define JOYSTICK_X A0 // Joystick X-axis
#define JOYSTICK_Y A1 // Joystick Y-axis
#define JOYSTICK_BUTTON 3 // Joystick button
//Buzzer -----------------------------------------------------------------------------------------
#define Buzzer 6  // buzzer pin
// EEPROM address -----------------------------------------------------------------------------------------
#define HS_Add 0 // high score address

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Global variables
int selectedOption = 0;  // New Game-0,High Score-1
                  //Game variables//
//snake -----------------------------------------------------------------------------------------
int snakeX[200], snakeY[200]; // for storing Snake body positions
int snakeLength = 2; // initial snake length = 2
int snakeDir = 1;  // intial snake direction (right)
// joystick -----------------------------------------------------------------------------------------
int joyX, joyY;  // read joystick x,y values
//Food -----------------------------------------------------------------------------------------
int foodX, foodY; // storing food position
int redFood=0; // To set the redfood appearing per level
int red; // to control the number of red foods for the level
int goodFoodEaten = 0; // Tracks good food eaten
bool foodPlaced = false;      //tracking food placement
bool badFoodFlag = false; //Tracks generation of bad food
//Game progress -----------------------------------------------------------------------------------------
int score = 0; // count game score
bool gameEnded = false; //flag for game over
int Level = 1;         // Initial game level
int speed=100; // Initial snake speed
// Food countdown -----------------------------------------------------------------------------------------
unsigned long foodSpawnTime ;  // Track food spawn time
const int maxCountTime = 5;  // maximum Countdown time  5 seconds
const unsigned long foodTimeout = 5000; // Food disappears after 5 seconds
//Screen and scoreboard -----------------------------------------------------------------------------------------
const int screenWidth = 320; //in pixels
const int screenHeight = 240; // in pixels
const int ScoreBoundaryY = 20;

                    //Function Prototypes//
// for menu -----------------------------------------------------------------------------------------
void handleJoystickInput(void);
void drawMenu(void);
void showHighscore(void);
              // for game//
void newGame(void); // game flow
void readJoystick(void); 
//Food -----------------------------------------------------------------------------------------
void spawnFood(void);
void drawFood(void);
void clearFood(void);
//snake -----------------------------------------------------------------------------------------
void drawSnake(void);
void moveSnake(void);
void shiftSnake(void);
//game display -----------------------------------------------------------------------------------------
void displayScore(void);
void gameOver(void);
void displayCountdown(int timeLeft);
void drawBarrier(void);
void displayLevel(void);
// game progress -----------------------------------------------------------------------------------------
void checkCollision(void);
void levelCheck(void);
void checkFoodTimeout(void);
void checkCountdown(void);
// reset for newgame -----------------------------------------------------------------------------------------
void resetGameVariables(void);
// Game sound -----------------------------------------------------------------------------------------
void playGoodFoodSound(void);
void playBadFoodSound(void);
void playGameOverSound(void);
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);  // Start serial communication for testing
  tft.begin();
  tft.setRotation(1);  // Set rotation to 1 (landscape)(0-3)
  tft.fillScreen(ILI9341_BLACK);

  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);
  pinMode(Buzzer, OUTPUT);

  // Seed the random number generator with a value from an unconnected analog pin
  randomSeed(analogRead(A3));
  EEPROM.put(HS_Add,0);  // To reset the high score value 
  // Draw the initial menu screen
  drawMenu();
}

void loop() {
  // Handle joystick movement for menu navigation
  handleJoystickInput();
  // Handle joystick button press for selection
  if (digitalRead(JOYSTICK_BUTTON) == LOW) {
    while (digitalRead(JOYSTICK_BUTTON) == LOW);  // Wait for button release
    if (selectedOption == 0) {
      gameEnded = false;
      newGame();
      delay(3000);  // hold the game over for 3 seconds
      tft.fillScreen(ILI9341_BLACK); // clean the display
      drawMenu(); // draw menu when exiting the game
    } else if (selectedOption == 1) {
      showHighscore();
    }
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
                        // Functions //
// functions for handling Menu -----------------------------------------------------------------------------------------
void handleJoystickInput(void) {
  int yValue = analogRead(JOYSTICK_Y);

  // Detect movement up or down
  if ((yValue < 400 ) & (selectedOption != 1)) { // Move to "High Score" only if not already selected
      selectedOption = 1;
      drawMenu();
      // delay(200); // Debounce
  } else if ((yValue > 600) &(selectedOption != 0)) { // Move to "New Game" only if not already selected
      selectedOption = 0;
      drawMenu();
      // delay(200); // Debounce
  }
}
void drawMenu(void) {

  // Draw a red border around the screen
  for(int i=0;i<5;i++){
    tft.drawRect(i, i, tft.width()-(20+2*i), tft.height()-2*i, ILI9341_RED);
    tft.drawLine(0, 118+i, 295, 118+i, ILI9341_RED);
  } 
  // Draw "New Game" option 
  if (selectedOption == 0) {
    tft.setTextColor(ILI9341_YELLOW);
  } else {
    tft.setTextColor(ILI9341_WHITE);
  }
  tft.setTextSize(4);
  tft.setCursor(55, 45);  // Position for "New Game" 
  tft.println("New Game");

  // Draw "High Score" option 
  if (selectedOption == 1) {
    tft.setTextColor(ILI9341_YELLOW);
  } else {
    tft.setTextColor(ILI9341_WHITE);
  }
  tft.setTextSize(4);
  tft.setCursor(35, 165);  // Position for "High Score" 
  tft.println("High Score");
}
void showHighscore(void){
  int highScore;
  int cursor; 
  EEPROM.get(HS_Add,highScore);
  tft.fillScreen(ILI9341_BLACK);
  // Draw a red border around the screen 
  for(int i=0;i<5;i++){
    tft.drawRect(i, i, tft.width()-(20+2*i), tft.height()-2*i, ILI9341_RED);
  }
    // Set the text properties
  tft.setTextColor(ILI9341_WHITE);  // text colour
  tft.setTextSize(4);               // text size
  tft.setCursor(35, 80);            // text position 
  tft.println("High Score");
  // centering score
  if(highScore>99){
    cursor = 115;
  }else if((highScore <= 99) && (highScore>9)){
    cursor = 130;
  }else cursor = 145;
  // Set the text properties for score
  tft.setTextColor(ILI9341_RED);    // Red color
  tft.setTextSize(4);               // text size
  tft.setCursor(cursor, 140);           // Set position
  tft.println(highScore);      
  delay(4000); // wait for 4s and return to main menu
  tft.fillScreen(ILI9341_BLACK); // clean the display
  drawMenu(); // draw menu 

}
// Game Play // -----------------------------------------------------------------------------------------
void readJoystick(void) {
  joyX = analogRead(JOYSTICK_X);
  joyY = analogRead(JOYSTICK_Y);

  if (joyX < 400 && snakeDir != 3) {
    snakeDir = 1; // right
  } else if (joyX > 600 && snakeDir != 1) {
    snakeDir = 3; // left
  } else if (joyY < 400 && snakeDir != 0) {
    snakeDir = 2; // down
  } else if (joyY > 600 && snakeDir != 2) {
    snakeDir = 0; // up
  }
}
//Food handling -----------------------------------------------------------------------------------------
void spawnFood(void) {  
  while (!foodPlaced) {
    // Randomly generate food coordinates
    foodX = random(0, screenWidth / 10) * 10;
    foodY = random(0, screenHeight/ 10) * 10;
    
    // Check if the food is on the snake's head
    bool isOnHead = (snakeX[0] == foodX && snakeY[0] == foodY);

    // check if food is placed on or near the barrier 
    bool onBarrier = ((foodX >= 90) && (foodX <= 200) && (foodY >= 50) && (foodY <= 190));

    // Check if the food is in the score display area (top-right corner)
    bool isInScoreArea = (foodY >= 0 && foodY <= ScoreBoundaryY);
    
    // Place food, if it's not on the snake's head, not in the score area, and not on the barrier
    if ((!isOnHead) && (!isInScoreArea) && (Level==1)) {
      //foodPlaced 
      foodPlaced = true;
      drawFood();
    }else if ((!isOnHead) && (!isInScoreArea) && (!onBarrier) && (Level>1)){
      //foodPlaced 
      drawFood();
      foodSpawnTime = millis(); // condition is checked only igf Level>=3
      foodPlaced = true;
    }
  }
}
void drawFood(void){
  // until level 4 good food
  if(Level<4){
    tft.fillRect(foodX, foodY, 10, 10, ILI9341_GREEN); //goodfood
  }else{ // randomly generate red,green food
    int r = random(1,3); // randomly generates 1 or 2
    if((r==1) && (red>0)){
      tft.fillRect(foodX, foodY, 10, 10, ILI9341_RED); //badfood
      badFoodFlag = true;
      red--;
    }else{
      tft.fillRect(foodX, foodY, 10, 10, ILI9341_GREEN); //goodfood
    }
  }
}
void clearFood(void) {
  tft.fillRect(foodX, foodY, 10, 10, ILI9341_BLACK);
  foodPlaced = false; // no food on display
}
// Snake handling -----------------------------------------------------------------------------------------
void drawSnake(void) {
  // Draw the head
  tft.fillRoundRect(snakeX[0], snakeY[0], 10, 10, 3, ILI9341_WHITE);
  // Draw the body
  for (int i = 1; i < snakeLength; i++) {
    if(snakeX[i] == foodX && snakeY[i] == foodY){
      if(badFoodFlag){
        tft.fillRect(snakeX[i], snakeY[i], 10, 10, ILI9341_RED); // handling if bad food spawn on snake body
      }else{
        tft.fillRect(snakeX[i], snakeY[i], 10, 10, ILI9341_GREEN); // handling if good food spawn on snake body
      }
    }else{
      tft.fillRoundRect(snakeX[i], snakeY[i], 10, 10,1, ILI9341_BLUE);
    }
  }
}
void moveSnake(void) {
  // Erase the tail  from the display
  if(snakeX[snakeLength - 1] == foodX && snakeY[snakeLength - 1] == foodY){
    if(badFoodFlag){
      tft.fillRect(snakeX[snakeLength - 1], snakeY[snakeLength - 1], 10, 10, ILI9341_RED); // handling if food spawn on snake body
    }else{
      tft.fillRect(snakeX[snakeLength - 1], snakeY[snakeLength - 1], 10, 10, ILI9341_GREEN); // handling if food spawn on snake body
    }
  }else{
    tft.fillRect(snakeX[snakeLength - 1], snakeY[snakeLength - 1], 10, 10, ILI9341_BLACK);
  }
  // Move the body
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  // Move the head
  switch (snakeDir) {
    case 0: snakeY[0] -= 10; break; // Up
    case 1: snakeX[0] += 10; break; // Right
    case 2: snakeY[0] += 10; break; // Down
    case 3: snakeX[0] -= 10; break; // Left
  }
  // Wrap the snake around the screen edges and score board

  if (snakeX[0] >= screenWidth) snakeX[0] = 0;
  if (snakeX[0] < 0) snakeX[0] = screenWidth - 10;
  if (snakeY[0] >= screenHeight) snakeY[0] = ScoreBoundaryY;  // 0-20  scorecard
  if (snakeY[0] < ScoreBoundaryY) snakeY[0] = screenHeight - 10;

  drawSnake();
}
void shiftSnake(void){
  if ((snakeX[0] >= 100) && (snakeX[0] <= 190) && (snakeY[0] >= 60) && (snakeY[0] <= 180)) {
    // If snake is inside the '0' shape, move it outside
    for (int i = 0; i < snakeLength; i++) {  // Clear the old snake
      tft.fillRect(snakeX[i], snakeY[i], 10, 10, ILI9341_BLACK);  
    }
    snakeX[0] = 40;  // Move snake to a safe position
    snakeY[0] = 220;
  // Re-initialize the body to follow the head in a straight line
    for (int i = 1; i < snakeLength; i++) {
      snakeX[i] = snakeX[i - 1] - 10;
      snakeY[i] = snakeY[0];  // Keep the Y coordinate same for horizontal placement

      // Handle screen wrapping for the body (horizontally only)
      if (snakeX[i] < 0) {
        snakeX[i] = screenWidth - 10;
      }
    }
    drawSnake();
  }
} 
// Display handling -----------------------------------------------------------------------------------------
void displayScore(void) {
  tft.setCursor(80, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.fillRect(80, 0, 120, 20, ILI9341_BLACK);
  tft.print("Score ");
  tft.print(score);
}
void displayLevel(void) {
  tft.setCursor(200, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.fillRect(200, 0, 120, 20, ILI9341_BLACK);
  tft.print("Level ");
  tft.print(Level);
}
void displayCountdown(int timeLeft) {
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);  // yellow text, black background
  tft.setTextSize(2);
  tft.setCursor(0, 0);  // Position the countdown in the top center
  tft.print(timeLeft);
}
void drawBarrier(void) {
    // Loop to draw multiple rectangles inside each other
  for (int i = 0; i < 10; i++) {
    tft.drawRoundRect(110 + i, 70 + i, 70 -  2* i, 100 - 2 * i, 20 - 2*i, ILI9341_RED);
    tft.drawLine(117+i/2, 155+i, 167+i/2, 75+i, ILI9341_RED);  // From bottom-left to top-right
  }
}
void gameOver(void) {
  playGameOverSound();
  int highScore;
  if(score > EEPROM.get(HS_Add,highScore)){ // update highscore to EEPROM
    EEPROM.put(HS_Add,score);
  }
  resetGameVariables();  // reset all game variables
  // Show Game over
  tft.fillScreen(ILI9341_BLACK);
  // Draw a red border around the screen 
  for(int i=0;i<5;i++){
    tft.drawRect(i, i, tft.width()-(20+2*i), tft.height()-2*i, ILI9341_RED);
  }
  // print Game over
  tft.setCursor(90, 70);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(5);
  tft.println("GAME");
  tft.setCursor(90, 120);
  tft.println("OVER");
}
// game progress -----------------------------------------------------------------------------------------
void newGame(void){
  tft.fillScreen(ILI9341_BLACK);
  snakeX[0] = random(0, screenWidth / 10) * 10;
  snakeY[0] = random(ScoreBoundaryY/10, screenHeight/ 10) * 10;
  for (int i = 1; i < snakeLength; i++) { // initialize snake
    snakeX[i] = snakeX[0] - i * 10;
    snakeY[i] = snakeY[0];
  }
  spawnFood();
  displayScore();
  displayLevel();
  while(!gameEnded && (snakeLength>0) && (score>=0)){
    readJoystick();
    moveSnake();
    checkCollision();
    checkCountdown();
    delay(speed); // Controls speed
  }
  gameOver();
}
void checkCollision(void) {
  // Check if the snake eats good food
  if (snakeX[0] == foodX && snakeY[0] == foodY) {
    if(badFoodFlag==true){
      playBadFoodSound();
    // reduce snake length
      tft.fillRect(snakeX[snakeLength - 1], snakeY[snakeLength - 1], 10, 10, ILI9341_BLACK);
      snakeLength--;
      score--; // reduce score
      foodSpawnTime=0;
      badFoodFlag=false;
    }else{
      playGoodFoodSound();
      snakeLength++;// Increase snake length
      score++; // Increase score
      goodFoodEaten++; // increase good food eaten flag
    }
    displayScore();
    clearFood();  // Remove the current food
    levelCheck();
    spawnFood(); // Spawn new food
  }
  // Check if the snake eats itself
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      gameEnded=true;
    }
  }
  // check if snake head hits the barrier
  if(Level>=2){
    if ((snakeX[0] >= 110) && (snakeX[0] <= 170) && (snakeY[0] >= 70) && (snakeY[0] <= 160)) {
      gameEnded=true;
    }
  }
}
void levelCheck(void){
  if(goodFoodEaten==2){
    Level++;
    displayLevel();
    goodFoodEaten=0;
    if(Level==2){
      shiftSnake();// shift if snake is in the center
      drawBarrier();
    }
    if(Level>=4){
      redFood++;
      red= redFood; // number of red foods for the level
    }
    if(Level>4 && speed>=10){
      // change snake speed increase by 20%
      speed*=0.8;

    }
  }
}
void checkCountdown(void){
  // Check if 5 seconds have passed since the food was spawned (red and green)
  if ((millis() - foodSpawnTime >= foodTimeout) && (Level>=3) && (foodPlaced)) {   //  && (foodPlaced)
    clearFood();  // Remove the current food
    badFoodFlag=false;  // clear flag when bad food dissappears
    spawnFood();  // Spawn new food
  }
  int time = (maxCountTime-(millis()-foodSpawnTime)/1000);
  if((Level>=3) && (foodPlaced)){  // display the countown for 5s from level3
    displayCountdown(time);
  }
}
// reset for new game -----------------------------------------------------------------------------------------
void resetGameVariables(void) {
  // reset game variables for new game
  score = 0;
  Level = 1;
  foodPlaced = false;
  badFoodFlag = false;
  speed = 100;
  redFood = 0;
  snakeLength = 2;
  snakeDir = 3;
  goodFoodEaten = 0;
}
// Game sound -----------------------------------------------------------------------------------------
void playGoodFoodSound(void) {
  tone(Buzzer, 2058, 100);  // Play a 1000 Hz tone for 100ms
}
void playBadFoodSound(void) {
  tone(Buzzer, 456, 100);   // Play a 500 Hz tone for 100ms
}
void playGameOverSound(void) {
  tone(Buzzer, 258, 1000);  // Play a 250 Hz tone for 1 second
}
