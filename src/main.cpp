#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <EEPROM.h>

#define TFT_CS    10  // Chip select pin for the ILI9341
#define TFT_RST   9   // Reset pin
#define TFT_DC    8   // Data/Command pin
#define JOYSTICK_X A0 // Joystick X-axis
#define JOYSTICK_Y A1 // Joystick Y-axis
#define JOYSTICK_BUTTON 3 // Joystick button
#define Buzzer 6

#define HS_Add 0 // high score address

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Global variables
int selectedOption = 0;  // New Game-0,High Score-1
//Game variables
int snakeX[100], snakeY[100]; // for storing Snake body positions
int snakeLength = 2; // initial snake length = 2
int snakeDir = 3;  // intial snake direction (left)
int joyX, joyY;  // read joystick x,y values
int foodX, foodY; // storing food position
int Food = 0;  // no of food spawned
int score = 0; // count game score
bool gameEnded = false; //flag for game over
int Level = 1;         // Initial game level
int goodFoodEaten = 0; // Tracks good food eaten
bool foodPlaced = false;      //tracking food placement
bool badFoodFlag = false; //Tracks generation of bad food
int speed=100; // Initial snake speed
int redFood=0; // To set the redfood appearing per level
int red; // to control the number of red foods for the level
unsigned long foodSpawnTime ;  // Track food spawn time
int countdown = 5;                // Countdown starts from 5

const int screenWidth = 320; //in pixels
const int screenHeight = 240; // in pixels
const int ScoreBoundaryX = 210; // scorecard boundart (to stop snake from entering scorecard)
const int ScoreBoundaryY = 20;
const unsigned long foodTimeout = 5000; // Food disappears after 5 seconds

//Function Prototypes
// for menu
void handleJoystickInput();
void drawMenu();
// for game
void newGame();
void readJoystick(); 
void spawnFood();
void drawFood();
void clearFood();
void drawSnake();
void moveSnake();
void checkCollision();
void displayScore();
void gameOver();
void showHighscore();
void levelCheck();
void checkFoodTimeout();
void displayCountdown(int timeLeft);

void setup() {
  Serial.begin(9600);  // Start serial communication
  tft.begin();
  tft.setRotation(1);  // Set rotation to 1 (landscape)(0-3)
  tft.fillScreen(ILI9341_BLACK);

  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);

  // Seed the random number generator with a value from an unconnected analog pin
  randomSeed(analogRead(A3));
  EEPROM.put(HS_Add,0);  // To reset the high score value initially
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
      Serial.println("Game Started"); // Start game function
      gameEnded = false;
      newGame();
      delay(3000);  // hold the game over for 3 seconds
      tft.fillScreen(ILI9341_BLACK); // clean the display
      drawMenu(); // draw menu when exiting the game
    } else if (selectedOption == 1) {
      showHighscore();
    }
  }
  delay(100);
}
// no change needed functions//
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
  // Draw "New Game" option (initial rendering)
  if (selectedOption == 0) {
    tft.setTextColor(ILI9341_YELLOW);
  } else {
    tft.setTextColor(ILI9341_WHITE);
  }
  tft.setTextSize(4);
  tft.setCursor(40, 60);  // Position for "New Game" (top half)
  tft.println("New Game");

  // Draw "High Score" option (initial rendering)
  if (selectedOption == 1) {
    tft.setTextColor(ILI9341_YELLOW);
  } else {
    tft.setTextColor(ILI9341_WHITE);
  }
  tft.setTextSize(4);
  tft.setCursor(40, 160);  // Position for "High Score" (bottom half)
  tft.println("High Score");
}
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
void clearFood() {
  tft.fillRect(foodX, foodY, 10, 10, ILI9341_BLACK);
  foodPlaced = false; // no food on display
}
void drawSnake(void) {
  // Draw the head
  tft.fillRect(snakeX[0], snakeY[0], 10, 10, ILI9341_WHITE);
  // Draw the body
  for (int i = 1; i < snakeLength; i++) {
    tft.fillRect(snakeX[i], snakeY[i], 10, 10, ILI9341_BLUE);
  }
}
void moveSnake(void) {
  // Erase the tail  from the display
  tft.fillRect(snakeX[snakeLength - 1], snakeY[snakeLength - 1], 10, 10, ILI9341_BLACK);

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
  if((snakeY[0] >= ScoreBoundaryY)){
    if (snakeX[0] >= screenWidth) snakeX[0] = 0;
    if (snakeX[0] < 0) snakeX[0] = screenWidth - 10;
  }else if((snakeY[0] < ScoreBoundaryY) && (snakeDir==1 || snakeDir==3)){
    if (snakeX[0] >=ScoreBoundaryX) snakeX[0] = 0;
    if (snakeX[0] < 0) snakeX[0] = ScoreBoundaryX - 10;
  }
  if((snakeX[0] < ScoreBoundaryX)){
    if (snakeY[0] >= screenHeight) snakeY[0] = 0;
    if (snakeY[0] < 0) snakeY[0] = screenHeight - 10;
  }else if((snakeX[0] >= ScoreBoundaryX) && (snakeDir==0 || snakeDir==2)){
    if (snakeY[0] >=screenHeight) snakeY[0] = ScoreBoundaryY;
    if (snakeY[0] < ScoreBoundaryY) snakeY[0] = screenHeight - 10;
  }
  drawSnake();
}
void displayScore(void) {
  tft.setCursor(210, 0);
  tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
  tft.setTextSize(2);
  // tft.fillRect(210, 0, 100, 20, ILI9341_YELLOW);
  tft.print("Score ");
  tft.print(score);
}
void gameOver(void) {
  int highScore;
  // Show Game over
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(90, 70);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(5);
  tft.println("GAME");

  tft.setCursor(90, 120);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(5);
  tft.println("OVER");

  if(score>EEPROM.get(HS_Add,highScore)){
    EEPROM.put(HS_Add,score);
  }
}
void showHighscore(){
  int highScore;
  int cursor; 
  EEPROM.get(HS_Add,highScore);
  tft.fillScreen(ILI9341_BLACK);
    // Set the text properties for the "High Score" label
  tft.setTextColor(ILI9341_WHITE);  // White color for the label
  tft.setTextSize(4);               // Set text size
  tft.setCursor(40, 80);            // Set position for "High Score" label
  tft.println("High Score");
  if(highScore>99){
    cursor = 120;
  }else if((highScore <= 99) && (highScore>9)){
    cursor = 135;
  }else cursor = 150;
  // Set the text properties for the actual score (in red)
  tft.setTextColor(ILI9341_RED);    // Red color for the score
  tft.setTextSize(4);               // Set text size
  tft.setCursor(cursor, 140);           // Set position for the actual score
  tft.println(highScore);           // Display the high score
  
  delay(3000); // wait for 3s and return to main menu
  tft.fillScreen(ILI9341_BLACK); // clean the display
  drawMenu(); // draw menu 

}
void displayCountdown(int timeLeft) {
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);  // yellow text, black background
  tft.setTextSize(2);
  tft.setCursor(0, 0);  // Position the countdown in the corner
  tft.print(timeLeft);
}

// no editing needed for now
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
void levelCheck(void){
  if(goodFoodEaten==2){
    Level++;
    goodFoodEaten=0;
    if(Level>=4){
      redFood++;
      red= redFood; // number of red foods for the level
    }
    // Serial.print(Level);
    // if(Level>4){
    //   // change snake speed increase by 20%
    //   speed-=10;
    // }
  }
}

// need editing
void newGame(){
  tft.fillScreen(ILI9341_BLACK);
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = screenWidth / 2 - i * 10;
    snakeY[i] = screenHeight / 2;
  }
  spawnFood();
  displayScore();

  while(!gameEnded && (snakeLength>0) && (score>=0)){
    readJoystick();
    moveSnake();
    checkCollision();
    levelCheck();
  // Check if 5 seconds have passed since the food was spawned (red and green)
  if ((millis() - foodSpawnTime >= foodTimeout) && (Level>=3) && (foodPlaced)) {   //  && (foodPlaced)
    clearFood();  // Remove the current food
    spawnFood();  // Spawn new food
  }
    // checkFoodTimeout(); // for counter
    delay(speed); // Controls speed
  }
  gameOver();
}
void spawnFood(void) {  
  while (!foodPlaced) {
    // Randomly generate food coordinates
    foodX = random(0, screenWidth / 10) * 10;
    foodY = random(0, screenHeight/ 10) * 10;
    
    // Check if the food is on the snake's head
    bool isOnHead = (snakeX[0] == foodX && snakeY[0] == foodY);
    
    // Check if the food is in the score display area (top-right corner)
    bool isInScoreArea = (foodX >= 200 && foodX <= 300 && foodY >= 0 && foodY <= 20);

    // check if food is placed on or near the barrier
    bool onBarrier = false;
    
    // Place food, if it's not on the snake's head, not in the score area, and not on the barrier
    if ((!isOnHead) && (!isInScoreArea) && (Level==1)) {
      //foodPlaced 
      foodPlaced = true;
      drawFood();
      Food++;   // not needed just for testing
    }else if ((!isOnHead) && (!isInScoreArea) && (!onBarrier) && (Level>1)){
      //foodPlaced 
      drawFood();
      foodSpawnTime = millis(); // condition is checked only igf Level>=3
      foodPlaced = true;
      Food++;
    }
  }
}  // edit after adding the barrier

void checkCollision(void) {
  // Check if the snake eats good food
  if (snakeX[0] == foodX && snakeY[0] == foodY) {
    if(badFoodFlag==true){
    // reduce snake length
      tft.fillRect(snakeX[snakeLength - 1], snakeY[snakeLength - 1], 10, 10, ILI9341_BLACK);
      snakeLength--;
      score--; // reduce score
      displayScore();
      clearFood();  // Remove the current food
      foodSpawnTime=0;
      badFoodFlag=false;
      spawnFood(); // Spawn new food
    }else{
      snakeLength++;// Increase snake length
      score++; // Increase score
      displayScore();
      clearFood();  // Remove the current food
  
      goodFoodEaten++; // increase good food eaten flag
      spawnFood(); // Spawn new food
    }
  }
  // Check if the snake eats itself
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      gameEnded=true;
      gameOver();
    }
  }
  // check if snake head hits the barrier

} // edit after adding the barrier
// void checkFoodTimeout() {
//   if (Level >= 3 && foodPlaced) {
//     unsigned long currentTime = millis();
    
//     // Calculate time elapsed since the food was spawned
//     unsigned long elapsedTime = (currentTime - foodSpawnTime) ;  // in milliseconds
    
//     if (elapsedTime >= 1 && elapsedTime <= 5000) {
//       countdown = 5 - (elapsedTime/1000);
//       displayCountdown(countdown);  // Update the countdown
//     }
    
//     // If 5 seconds have passed, remove the food
//     if (elapsedTime >= 5000) {
//       tft.fillRect(foodX, foodY, 10, 10, ILI9341_BLACK);  // Remove the food
//       foodVisible = false;
//       spawnFood(); // spwan new food after dissappearing
//     }
//   }
// }  // check this function may not need it/ edit and change
