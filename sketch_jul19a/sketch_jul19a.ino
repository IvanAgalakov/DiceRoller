#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>
#include "pitches.h"

// struct Color {
//   int r;
//   int g;
//   int b;
// };

struct Point3D {
  float x;
  float y;
  float z;
};

struct Edge {
  int a;
  int b;
};

// #define POINT_NUM 8
// Point3D points[POINT_NUM] = {{-30,-30,-30},{30,-30,-30}, {-30,30,30}, {30,30,-30}, {30,-30,30}, {-30,30,-30}, {-30,-30,30}, {30,30,30}};
// #define EDGE_NUM 12
// Edge edges[EDGE_NUM] = {{0,1},{0,5},{0,6},{1,3},{1,4},{2,7},{2,6},{2,5},{3,7},{3,5},{4,7},{4,6}};
Point3D speed = {10,10,10};
#define POINT_NUM 12
#define PHI 1.618033988749895  // Golden ratio
#define SIZE 30
Point3D points[POINT_NUM] = {
    {0, SIZE, PHI*SIZE}, {0, -SIZE, PHI*SIZE}, {0, SIZE, -PHI*SIZE}, {0, -SIZE, -PHI*SIZE},
    {SIZE, PHI*SIZE, 0}, {-SIZE, PHI*SIZE, 0}, {SIZE, -PHI*SIZE, 0}, {-SIZE, -PHI*SIZE, 0},
    {PHI*SIZE, 0, SIZE}, {-PHI*SIZE, 0, SIZE}, {PHI*SIZE, 0, -SIZE}, {-PHI*SIZE, 0, -SIZE}
};

#define EDGE_NUM 30
Edge edges[EDGE_NUM] = {
    {0,1}, {0,4}, {0,5}, {0,8}, {0,9},
    {1,6}, {1,7}, {1,8}, {1,9},
    {2,3}, {2,4}, {2,5}, {2,10}, {2,11},
    {3,6}, {3,7}, {3,10}, {3,11},
    {4,5}, {4,8}, {4,10},
    {5,9}, {5,11},
    {6,7}, {6,8}, {6,10},
    {7,9}, {7,11},
    {8,10}, {9,11}
};

#define BUTTON_PIN 6

//Color color;

TFT_eSPI tft = TFT_eSPI();

#define SD_CS 4

bool readShapeFromFile(const char* filename) {
  File file = SD.open(filename);
  if (!file) {
    //Serial.println(F("Failed to open file for reading"));
    return false;
  }

  // Clear existing points and edges
  memset(points, 0, sizeof(points));
  memset(edges, 0, sizeof(edges));

  // Read number of points
  int numPoints = file.parseInt();
  if (numPoints > POINT_NUM) {
    //Serial.println(F("Too many points in file"));
    file.close();
    return false;
  }

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
  int numEdges = file.parseInt();
  if (numEdges > EDGE_NUM) {
    //Serial.println(F("Too many edges in file"));
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

  file.close();
  //Serial.println(F("Shape loaded successfully"));
  return true;
}

void setup() {
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.drawCentreString("1d4", 120, 40, 2);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!SD.begin(SD_CS)) {
    //Serial.println(F("SD card initialization failed!"));
    tone(7,NOTE_G4);
    delay(250);
    noTone(7);
    delay(250);
    tone(7,NOTE_C4);
    delay(500);
    noTone(7);
    return;
  }

  // File myFile = SD.open("log.txt", FILE_WRITE);
  // if (myFile) {
  //   //Serial.print("Writing to log.txt...");
  //   myFile.println("Power On");
  //   // close the file:
  //   myFile.close();
  //   //Serial.println("done.");
  // } else {
  //   // if the file didn't open, print an error:
  //   //Serial.println("error opening test.txt");
  // }

  if (readShapeFromFile("/4")) {
    //Serial.println(F("Shape loaded successfully"));
  } else {
    Serial.println(F("Failed to load shape"));
  }

  //color.r = 0;
  //color.g = 0;
  //color.b = 0;

  

  //randomSeed(analogRead(0)); // Seed the random number generator
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

float deltatime = 0.1;
Point3D projectedPoints[POINT_NUM];
bool pPressed = false;

#define MAX_FILES 6
String files[MAX_FILES] = {"/4","/6","/8","/10","/12","/20"};
int d_num = 0;

void loop() {
  
  //if (currentMillis - previousMillis >= interval) {
  unsigned long prev = millis();

  int val = digitalRead(BUTTON_PIN);   // read the input pin
  bool pressed = false;
  if (val == LOW) {
    Serial.println("a");
    pressed = true;
  }

  if (pressed && !pPressed) {
    d_num++;
    if (d_num >= MAX_FILES) {
      d_num = 0;
    }
    readShapeFromFile(files[d_num].c_str());
    tft.fillCircle(120, 120, 70, TFT_BLACK);
  }

  Point3D center = {0, 0, 0};

  Point3D pPoints[POINT_NUM];
  // Draw lines between projected points
  for (int i = 0; i < POINT_NUM; i++) {
    rotate(points[i], speed.x*deltatime, speed.y*deltatime, speed.z*deltatime, center); // Rotate 1 degree around all axes
    project(points[i], pPoints[i]);
  }

  
  tft.startWrite();
  for (int i = 0; i < EDGE_NUM; i++) {
    tft.drawLine(projectedPoints[edges[i].a].x, projectedPoints[edges[i].a].y, projectedPoints[edges[i].b].x, projectedPoints[edges[i].b].y, TFT_BLACK);
    tft.drawLine(pPoints[edges[i].a].x, pPoints[edges[i].a].y, pPoints[edges[i].b].x, pPoints[edges[i].b].y, TFT_WHITE);
  }
  tft.endWrite();
  

  for (int i = 0; i < POINT_NUM; i++) {
    projectedPoints[i] = pPoints[i];
  }

  deltatime = (millis() - prev) / 1000.0;

  // Update color randomly
  //color.r += random(-1, 1);
  //color.g += random(-1, 1);
  //color.b += random(-1, 1);
  //color.r = constrain(color.r, 0, 255);
  //color.g = constrain(color.g, 0, 255);
  //color.b = constrain(color.b, 0, 255);
  
  //}
}

// uint16_t read16(File &f) {
//   uint16_t result;
//   f.read((uint8_t *)&result, 2);
//   return result;
// }

// uint32_t read32(File &f) {
//   uint32_t result;
//   f.read((uint8_t *)&result, 4);
//   return result;
// }
