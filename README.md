# Dice Roller
This is code which can be used for a TFT display, using the TFT_eSPI and SdFat libraries.

The following pin mapping is expected:
| PIN  | USES |
| ------------- | ------------- |
| A3  | Switch Button  |
| A2  | Right Button  |
| A1  | Left Button  |
| A0  | Roll Button  |
| D13  | Display SCL / SD SCK  |
| D12  | SD Miso  |
| D11  | Display SDA / SD Mosi  |
| D10  | Display CS  |
| D9  | Display DC  |
| D8  | Display RST  |
| D7  | Speaker Pin  |
| D4  | SD CS  |
| **GND**  | All Buttons / SD GND / Display GND / Speaker GND  |
| **5V**  | SD Card VCC |
| **3v3**  | Display VCC |


The two folders present are the Arduino Sketch in the `/diceroller` folder, use this sketch for your arduino nano being used for the project.

The second folder `/Visualizer` contains python code for displaying and editing the dice files in the format needed to be put on the SD card. 
Once you have made these files name them 4, 6, 8, 10, 12, 20 and put them in the root directory of the microSD card, they will be loaded automatically.
