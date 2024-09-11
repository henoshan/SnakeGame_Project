#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

#define TFT_CS    10
#define TFT_DC    8
#define TFT_RST   9
#define JOYSTICK_X A0
#define JOYSTICK_Y A1
#define JOYSTICK_BUTTON 7

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

int snakeX[100], snakeY[100]; // Snake body positions
int snakeLength = 2; // Start with length of 2
int foodX, foodY; // Food position
int score = 0;
int snakeDir = 1;

int joyX, joyY;
const int screenWidth = 240;
const int screenHeight = 320;

void setup() {
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);

  // Initialize snake position
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = screenWidth / 2 - i * 10;
    snakeY[i] = screenHeight / 2;
  }

  spawnFood();
  drawFood();
  displayScore();
}

//void spawnFood() {
//  foodX = random(0, screenWidth / 10) * 10;
//  foodY = random(0, screenHeight / 10) * 10;
//}

void spawnFood() {
  bool foodPlaced = false;
  while (!foodPlaced) {
    foodX = random(0, screenWidth / 10) * 10;
    foodY = random(0, screenHeight / 10) * 10;
    
    // Check if the food is not placed on the snake's body
    bool isOnSnake = false;
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) {
        isOnSnake = true;
        break;
      }
    }
    
    if (!isOnSnake) {
      foodPlaced = true;
    }
  }
}


void drawFood() {
  tft.fillRect(foodX, foodY, 10, 10, ILI9341_GREEN);
}

void displayScore() {
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.fillRect(0, 0, 100, 20, ILI9341_BLACK);
  tft.print("Score: ");
  tft.print(score);
}

void loop() {
  readJoystick();
  moveSnake();
  checkCollision();
  delay(100); // Controls speed
}

void readJoystick() {
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

void moveSnake() {
  // Erase the tail segment from the display
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

  // Wrap the snake around the screen edges
  if (snakeX[0] >= screenWidth) snakeX[0] = 0;
  if (snakeX[0] < 0) snakeX[0] = screenWidth - 10;
  if (snakeY[0] >= screenHeight) snakeY[0] = 0;
  if (snakeY[0] < 0) snakeY[0] = screenHeight - 10;

  drawSnake();
}

void drawSnake() {
  // Draw the head
  tft.fillRect(snakeX[0], snakeY[0], 10, 10, ILI9341_WHITE);

  // Draw the body
  for (int i = 1; i < snakeLength; i++) {
    tft.fillRect(snakeX[i], snakeY[i], 10, 10, ILI9341_BLUE);
  }
}

void checkCollision() {
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
    drawFood();
  }

  // Check if the snake eats itself
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      gameOver();
    }
  }
}


void gameOver() {
  tft.fillScreen(ILI9341_RED);
  tft.setCursor(50, 150);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.println("Game Over");

  while (true); // Stop the game
}
