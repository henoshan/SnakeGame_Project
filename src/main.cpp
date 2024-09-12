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

#define HS_Add 0 // high score address

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Global variables
int selectedOption = 0;  // 0: New Game, 1: High Score
//Game variables
int snakeX[100], snakeY[100]; // Snake body positions
int snakeLength = 2; // Start with length of 2
int snakeDir = 1;
int joyX, joyY;
int foodX, foodY; // Food position
int Food = 0;
int score = 0;
int gameEnded = 0; //flat for game over

const int screenWidth = 320; //in pixels
const int screenHeight = 240; // in pixels
const int ScoreBoundaryX = 210;
const int ScoreBoundaryY = 20;

//Function Prototypes
// for menu
void handleJoystickInput();
void drawMenu();
// for game
void newGame();
void readJoystick(); 
void spawnFood();
void drawFood();
void drawSnake();
void moveSnake();
void checkCollision();
void displayScore();
void gameOver();

void setup() {
  Serial.begin(9600);  // Start serial communication
  tft.begin();
  tft.setRotation(1);  // Set rotation to 1 (landscape)(0-3)
  tft.fillScreen(ILI9341_BLACK);

  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);

  // Seed the random number generator with a value from an unconnected analog pin
  randomSeed(analogRead(A3));

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
      newGame();
      delay(2000);
      tft.fillScreen(ILI9341_BLACK); // clean the display
      drawMenu(); // draw menu when exiting the game
    } else if (selectedOption == 1) {
      Serial.println("Showing High Score"); // Show high score for 3 seconds
    }
  }
  delay(100);
}

void handleJoystickInput(void) {
  int yValue = analogRead(JOYSTICK_Y);

  // Detect movement up or down
  if ((yValue > 600 ) & (selectedOption != 1)) { // Move to "High Score" only if not already selected
      selectedOption = 1;
      drawMenu();
      // delay(200); // Debounce
  } else if ((yValue < 400) &(selectedOption != 0)) { // Move to "New Game" only if not already selected
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

// void updateMenu() {
//   // Change the color of the previously selected option to white
//   if (lastSelectedOption == 0) {
//     tft.setTextColor(ILI9341_WHITE);
//     tft.setTextSize(4);
//     tft.setCursor(40, 60);
//     tft.println("New Game");
//   } else if (lastSelectedOption == 1) {
//     tft.setTextColor(ILI9341_WHITE);
//     tft.setTextSize(4);
//     tft.setCursor(40, 160);
//     tft.println("High Score");
//   }

//   // Highlight the newly selected option
//   if (selectedOption == 0) {
//     tft.setTextColor(ILI9341_YELLOW);
//     tft.setTextSize(4);
//     tft.setCursor(40, 60);
//     tft.println("New Game");
//   } else if (selectedOption == 1) {
//     tft.setTextColor(ILI9341_YELLOW);
//     tft.setTextSize(4);
//     tft.setCursor(40, 160);
//     tft.println("High Score");
//   }
// }
void newGame(void){
  tft.fillScreen(ILI9341_BLACK);
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = screenWidth / 2 - i * 10;
    snakeY[i] = screenHeight / 2;
  }
  spawnFood();
  displayScore();

  while(!gameEnded){
    readJoystick();
    moveSnake();
    checkCollision();
    delay(100); // Controls speed
  }
}
void readJoystick(void) {
  joyX = analogRead(JOYSTICK_X);
  joyY = analogRead(JOYSTICK_Y);

  if (joyX < 400 && snakeDir != 1) {
    snakeDir = 3; // Left
  } else if (joyX > 600 && snakeDir != 3) {
    snakeDir = 1; // Right
  } else if (joyY < 400 && snakeDir != 2) {
    snakeDir = 0; // Up
  } else if (joyY > 600 && snakeDir != 0) {
    snakeDir = 2; // Down
  }
}
void spawnFood(void) {
  bool foodPlaced = false;
  
  while (!foodPlaced) {
    // Randomly generate food coordinates
    foodX = random(0, screenWidth / 10) * 10;
    foodY = random(0, screenHeight/ 10) * 10;
    
    // Check if the food is on the snake's head
    bool isOnHead = (snakeX[0] == foodX && snakeY[0] == foodY);
    
    // Check if the food is in the score display area (top-right corner)
    bool isInScoreArea = (foodX >= 200 && foodX <= 300 && foodY >= 0 && foodY <= 20);
    
    // Place food, if it's not on the snake's head and not in the score area
    if (!isOnHead && !isInScoreArea) {
      //foodPlaced 
      foodPlaced = true;
      drawFood();
      Food++;
      // Serial.println("Food Coordinates: (" + String(foodX) + ", " + String(foodY) + "), " + String(Food));

    }
  }
}

void drawFood(void){
  tft.fillRect(foodX, foodY, 10, 10, ILI9341_GREEN);
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
  if((snakeX[0] <= ScoreBoundaryX)){
    if (snakeY[0] >= screenHeight) snakeY[0] = 0;
    if (snakeY[0] < 0) snakeY[0] = screenHeight - 10;
  }else if((snakeX[0] > ScoreBoundaryX) && (snakeDir==0 || snakeDir==2)){
    if (snakeY[0] >=screenHeight) snakeY[0] = ScoreBoundaryY;
    if (snakeY[0] < ScoreBoundaryY) snakeY[0] = screenHeight - 10;
  }
  drawSnake();
}

void checkCollision(void) {
  // Check if the snake eats food
  if (snakeX[0] == foodX && snakeY[0] == foodY) {
    // Increase snake length
    snakeLength++;
    score++;
    displayScore();
    // Remove the current food from the display
    tft.fillRect(foodX, foodY, 10, 10, ILI9341_BLACK);
    // Spawn new food
    spawnFood();
    // drawFood();
  }
  // Check if the snake eats itself
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      gameOver();
    }
  }
}

void displayScore(void) {
  tft.setCursor(210, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.fillRect(210, 0, 100, 20, ILI9341_BLACK);
  tft.print("Score ");
  tft.print(score);
}

void gameOver(void) {
  tft.fillScreen(ILI9341_RED);
  tft.setCursor(50, 150);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.println("Game Over!!");

  gameEnded=1;
}
