#include <TFT_eSPI.h>
#include <SdFat.h>

struct Point3D {
  float x;
  float y;
  float z;
};

struct Edge {
  uint8_t a;
  uint8_t b;
};

#define POINT_NUM 20
Point3D points[POINT_NUM];

#define EDGE_NUM 30
Edge edges[EDGE_NUM];

#define MAX_FILES 6
const char* files[MAX_FILES] = {"/4","/6","/8","/10","/12","/20"};

#define S_BUTTON_PIN 17
#define R_BUTTON_PIN 16
#define L_BUTTON_PIN 15
#define ROLL_PIN 14

TFT_eSPI tft = TFT_eSPI();
SdFat SD;
File file;

#define SD_CS 4

#define SPEAKER_PIN 7

uint16_t shapeColor = TFT_WHITE;

void readShapeFromFile(const char* filename) {
  file = SD.open(filename);

  // Clear existing points and edges
  memset(points, 0, sizeof(points));
  memset(edges, 0, sizeof(edges));

  // Read number of points
  int numPoints = file.parseInt();

  // Read points
  for (int i = 0; i < numPoints; i++) {
    points[i].x = file.parseFloat();
    file.read(); // Skip comma
    points[i].y = file.parseFloat();
    file.read(); // Skip comma
    points[i].z = file.parseFloat();
    file.read(); // Skip newline
  }

  // Read number of edges
  uint8_t numEdges = file.parseInt();
  if (numEdges > EDGE_NUM) {
    file.close();
    return false;
  }

  // Read edges
  for (int i = 0; i < numEdges; i++) {
    edges[i].a = file.parseInt();
    file.read(); // Skip comma
    edges[i].b = file.parseInt();
    file.read(); // Skip newline
  }

  // Read color
  float r = file.parseFloat();
  file.read(); // Skip comma
  float g = file.parseFloat();
  file.read(); // Skip comma
  float b = file.parseFloat();
  file.read(); // Skip newline

  // Convert RGB (0-1) to RGB565 color format
  uint8_t r8 = (uint8_t)(r * 31) & 0x1F;
  uint8_t g8 = (uint8_t)(g * 63) & 0x3F;
  uint8_t b8 = (uint8_t)(b * 31) & 0x1F;
  shapeColor = (r8 << 11) | (g8 << 5) | b8;

  file.close();
}

void displayDiceType(const char* diceType, int amount) {
  tft.setTextSize(2); // Set text size

  // Clear the previous text area
  tft.fillRect(0, 0, 240, 40, TFT_BLACK); // Adjust the dimensions if needed

  tft.setTextColor(shapeColor, TFT_BLACK); // White text on black background
  // Draw "Roll: <diceType>"
  String rollText = "Dice: ";
  rollText += diceType;
  int rollTextWidth = tft.textWidth(rollText);
  tft.setCursor((240 - rollTextWidth) / 2, 10); // Center horizontally, position vertically
  tft.print(rollText);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); // White text on black background
  // Draw "Amount: <number>"
  String amountText = "Amount: ";
  amountText += String(amount);
  int amountTextWidth = tft.textWidth(amountText);
  tft.setCursor((240 - amountTextWidth) / 2, 30); // Center horizontally, position vertically
  tft.print(amountText);
}

void displayResult(int number) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // White text on black background
  tft.setTextSize(2); // Set text size

  // Clear the previous text area
  tft.fillRect(0, 190, 240, 50, TFT_BLACK); // Adjust the dimensions if needed

  String resultText = "Result: ";
  resultText += String(number);
  int resultTextWidth = tft.textWidth(resultText);
  tft.setCursor((240 - resultTextWidth) / 2, 200); // Center horizontally, position vertically
  tft.print(resultText);
}


void setup() {
  tft.begin();
  tft.fillScreen(TFT_BLACK);

  pinMode(S_BUTTON_PIN, INPUT_PULLUP);
  pinMode(R_BUTTON_PIN, INPUT_PULLUP);
  pinMode(L_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ROLL_PIN, INPUT_PULLUP);

  if (!SD.begin(SD_CS)) {
    //Serial.println(F("SD card initialization failed!"));
    return;
  }

  readShapeFromFile(files[0]);

  // Initial dice type display
  displayDiceType("4", 1);
}

void rotate(Point3D &point, float angleX, float angleY, float angleZ, const Point3D &center) {
  float radX = radians(angleX);
  float radY = radians(angleY);
  float radZ = radians(angleZ);

  float cosX = cos(radX), sinX = sin(radX);
  float cosY = cos(radY), sinY = sin(radY);
  float cosZ = cos(radZ), sinZ = sin(radZ);

  // Translate point to the origin
  float x = point.x - center.x;
  float y = point.y - center.y;
  float z = point.z - center.z;

  // Rotation around X-axis
  float tempY = y * cosX - z * sinX;
  float tempZ = y * sinX + z * cosX;
  y = tempY;
  z = tempZ;

  // Rotation around Y-axis
  float tempX = x * cosY + z * sinY;
  z = -x * sinY + z * cosY;
  x = tempX;

  // Rotation around Z-axis
  tempX = x * cosZ - y * sinZ;
  tempY = x * sinZ + y * cosZ;
  x = tempX;
  y = tempY;

  // Translate point back to its original position
  point.x = x + center.x;
  point.y = y + center.y;
  point.z = z + center.z;
}

void project(Point3D &point, Point3D &projPoint) {
  float distance = 400; // Distance from the viewer to the screen
  projPoint.x = point.x * (distance / (distance + point.z)) + 120; // Adjust for screen center
  projPoint.y = point.y * (distance / (distance + point.z)) + 120; // Adjust for screen center
}

float lerpToTarget(float targetSpeed, float deltaTime, float value) {
  if (value > targetSpeed) {
    value -= abs(value - targetSpeed) * deltaTime;
    if (value < targetSpeed) value = targetSpeed; // Clamp to target value
  } else if (value < targetSpeed) {
    value += abs(value - targetSpeed) * deltaTime;
    if (value > targetSpeed) value = targetSpeed; // Clamp to target value
  }
  return value;
}

constexpr uint8_t initialFrequency = 500;
constexpr uint8_t finalFrequency = 10;
constexpr uint8_t rollDuration = 200;
constexpr uint8_t numBeeps = 30;

unsigned long beepStartTime = 0;
uint8_t currentBeep = 0;
bool rolling = false;

void rollDiceSound() {
  if (rolling) {
    unsigned long elapsedTime = millis() - beepStartTime;
    if (elapsedTime >= 6) {
      beepStartTime = millis();
      int frequency = map(currentBeep * 6, 0, rollDuration, initialFrequency, finalFrequency);
      int beepDuration = map(currentBeep, 0, numBeeps, 50, 150);
      
      tone(SPEAKER_PIN, frequency, beepDuration);
      
      currentBeep++;
      if (currentBeep >= numBeeps) {
        rolling = false;
        noTone(SPEAKER_PIN);
      }
    }
  }
}


void playSwitch() {
  const int duration = 50;  // Duration of each tone in milliseconds
  const int pause = 10;     // Pause between tones in milliseconds
  
  // Play an ascending series of tones
  tone(SPEAKER_PIN, 1000, 50);
  delay(60);
  tone(SPEAKER_PIN, 1500, 50);
  delay(60);
  tone(SPEAKER_PIN, 2000, 50);
  delay(50);
  
  noTone(SPEAKER_PIN);  // Ensure the tone is stopped
}

void startRollDiceSound() {
  rolling = true;
  beepStartTime = millis();
  currentBeep = 0;
}



float deltatime = 0.1;
uint8_t pPressed = 0;
uint8_t d_num = 0;
Point3D pPoints[POINT_NUM];

uint8_t roll_num = 1;

Point3D speed = {10,10,10};

void loop() {
  unsigned long prev = millis();
  
  uint8_t pressed = 0;
  if (digitalRead(S_BUTTON_PIN) == LOW) {
    pressed = S_BUTTON_PIN;
  } else if (digitalRead(R_BUTTON_PIN) == LOW) {
    pressed = R_BUTTON_PIN;
  } else if (digitalRead(L_BUTTON_PIN) == LOW) {
    pressed = L_BUTTON_PIN;
  } else if (digitalRead(ROLL_PIN) == LOW) {
    pressed = ROLL_PIN;
  } 

  if (pPressed == 0) {
    switch(pressed) {
      case S_BUTTON_PIN:
        d_num = (d_num + 1) % MAX_FILES;
        readShapeFromFile(files[d_num]);
        //Serial.println(files[d_num]);
        tft.fillScreen(TFT_BLACK); // Clear the screen
        rolling = false;
        speed.x = 10;
        speed.y = 10;
        speed.z = 10;
        playSwitch();
        roll_num = 1;
        displayDiceType(files[d_num] + 1, roll_num); // Update dice type display (skip '/' character)
        break;
      case R_BUTTON_PIN:
        roll_num++;
        displayDiceType(files[d_num] + 1, roll_num);
        tone(SPEAKER_PIN, 1000, 50);
        delay(50);
        break;
      case L_BUTTON_PIN:
        roll_num--;
        if (roll_num < 1) {
          roll_num = 1;
        }
        displayDiceType(files[d_num] + 1, roll_num);
        tone(SPEAKER_PIN, 800, 50);
        delay(50);
        break;
      case ROLL_PIN:
          // Generate random value between 30 and 50
          speed.x = random(50, 81);
          speed.y = random(50, 81);
          speed.z = random(50, 81);

          // Randomly decide to flip the sign
          if (random(0, 2) == 0) speed.x = -speed.x;
          if (random(0, 2) == 0) speed.y = -speed.y;
          if (random(0, 2) == 0) speed.z = -speed.z;

          startRollDiceSound();

          int total = 0;
          for (int i = 0; i < roll_num; i++) {
            switch(d_num) {
              case 0:
                total += random(1,5);
                break;
              case 1:
                total += random(1,7);
                break;
              case 2:
                total += random(1,9);
                break;
              case 3:
                total += random(1,11);
                break;
              case 4:
                total += random(1,13);
                break;
              case 5:
                total += random(1,21);
                break;
              default:
                break;
            }
          }
          displayResult(total);

          break;

      default:
        break;
    }
  }
  rollDiceSound();

  Point3D center = {0, 0, 0};
  // Draw lines between projected points
  for (int i = 0; i < POINT_NUM; i++) {
    project(points[i], pPoints[i]);
    rotate(points[i], speed.x * deltatime, speed.y * deltatime, speed.z * deltatime, center); // Rotate 1 degree around all axes
  }

  tft.startWrite();
  // Draw edges
  for (int i = 0; i < EDGE_NUM; i++) {
    tft.drawLine(pPoints[edges[i].a].x, pPoints[edges[i].a].y, pPoints[edges[i].b].x, pPoints[edges[i].b].y, TFT_BLACK);
    Point3D a;
    Point3D b;
    project(points[edges[i].a], a);
    project(points[edges[i].b], b);
    tft.drawLine(a.x, a.y, b.x, b.y, shapeColor);
  }
  tft.endWrite();

  float targetSpeed = speed.x > 0 ? 10.0 : -10.0;
  speed.x = lerpToTarget(targetSpeed, deltatime, speed.x);
  
  targetSpeed = speed.y > 0 ? 10.0 : -10.0;
  speed.y = lerpToTarget(targetSpeed, deltatime, speed.y);
  
  targetSpeed = speed.z > 0 ? 10.0 : -10.0;
  speed.z = lerpToTarget(targetSpeed, deltatime, speed.z);

  deltatime = (millis() - prev) / 1000.0;
  pPressed = pressed;
}
