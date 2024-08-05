#include <TFT_eSPI.h>
#include <SdFat.h>
#include <math.h>

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

#define MAX_FILES 6
const char* files[MAX_FILES] = {"/4","/6","/8","/10","/12","/20"};

#define BUTTON_PIN 6

TFT_eSPI tft = TFT_eSPI();
SdFat SD;
File file;

#define SD_CS 4

bool readShapeFromFile(const char* filename) {
  file = SD.open(filename);
  if (!file) {
    return false;
  }

  // Clear existing points and edges
  memset(points, 0, sizeof(points));
  memset(edges, 0, sizeof(edges));

  // Read number of points
  int numPoints = file.parseInt();
  if (numPoints > POINT_NUM) {
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
  return true;
}

void setup() {
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.drawCentreString("1d4", 120, 30, 2);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD card initialization failed!"));
    return;
  }

  if (readShapeFromFile(files[0])) {
    Serial.println(F("Shape loaded successfully"));
  } else {
    Serial.println(F("Failed to load shape"));
  }
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
bool pPressed = false;
int d_num = 0;
Point3D pPoints[POINT_NUM];

void loop() {
  
  unsigned long prev = millis();

  int val = digitalRead(BUTTON_PIN);   // read the input pin
  bool pressed = false;
  if (val == LOW) {
    pressed = true;
  }

  if (pressed && !pPressed) {
    d_num = (d_num+1)%MAX_FILES;
    readShapeFromFile(files[d_num]);
    Serial.println(files[d_num]);
    tft.fillCircle(120, 120, 70, TFT_BLACK);
  }

  Point3D center = {0, 0, 0};
  // Draw lines between projected points
  for (int i = 0; i < POINT_NUM; i++) {
    project(points[i], pPoints[i]);
    rotate(points[i], 10*deltatime, 10*deltatime, 10*deltatime, center); // Rotate 1 degree around all axes
  }

  
  tft.startWrite();
  //tft.fillCircle(120, 120, 70, TFT_BLACK);
  for (int i = 0; i < EDGE_NUM; i++) {
    tft.drawLine(pPoints[edges[i].a].x, pPoints[edges[i].a].y, pPoints[edges[i].b].x, pPoints[edges[i].b].y, TFT_BLACK);
    Point3D a;
    Point3D b;
    project(points[edges[i].a], a);
    project(points[edges[i].b], b);
    tft.drawLine(a.x, a.y, b.x, b.y, TFT_WHITE);
  }
  tft.endWrite();

  deltatime = (millis() - prev) / 1000.0;
}
