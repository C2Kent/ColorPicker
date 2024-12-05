// Code by Colin Kent, December 5th 2024
// Uses the Waveshare LCD Driver library and writing functions
// Uses the AdaFruit TCS34725 Color Sensor library

#include <SPI.h>
#include "LCD_Driver.h"
#include "GUI_Paint.h"
#include <Wire.h>
#include "Adafruit_TCS34725.h"

byte gammatable[256];       //Convert measured RGB values in more realistic visulisation color on the RGB LED

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);   //Adafruit sensor setup library

  uint16_t rgb565;
  uint16_t textColor;

  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;

  
  float redCalibration = 1.0;    // Calibration factor for red
  float greenCalibration = 1.3; // Calibration factor for green
  float blueCalibration = 1.5;   // Calibration factor for blue

  int redMin = 30, redMax = 200;  //Minimum and maximum red values to account for sensor imperfections
  int greenMin = 40, greenMax = 160;  //Minimum and maximum green values to account for sensor imperfections
  int blueMin = 17, blueMax = 190;  //Minimum and maximum blue values to account for sensor imperfections

  bool modePushed = false;
  
  int state = 1;

  char hexCode[8];

  int modePin = 3; //Main and only button

  unsigned long previousMillis = 0; 
  const unsigned long interval = 5000; // 5 seconds

void setup()
{
  Serial.begin(9600);

  pinMode(6, INPUT);
  pinMode(5, INPUT);
 
  Config_Init();
  LCD_Init();
  Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, WHITE);

  #define commonAnode false

  LCD_Clear(rgb565); // Initialize screen with black
  update();  //Start the program
  colorModeButton(); //Display default (RGB)

    if (tcs.begin()) //Connect to the color sensor
  {
    tcs.setIntegrationTime(500);
    Serial.println("Ready");
  } 
  else
  {
    Serial.println("Sensor Error");
    while (1);
  }
  for (int i=0; i<256; i++) //Set up the gamma table for RGB conversion. Lines 69-84 generated with ChatGPT 4.
  {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;
    if (commonAnode)
    {
      gammatable[i] = 255 - x;
    } 
    else
    {
      gammatable[i] = x;
    }
  }
}

void loop()
{
  unsigned long currentMillis = millis();
  update();
  checkButtonPress(modePin, 300, 600);
  // Timer for restarting the sensor so it can restart after the main machine turns off
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Restart sensor
  tcs.begin();
  }
}

void defineRgb565Color(uint8_t red, uint8_t green, uint8_t blue) { //Lines 100-108 generated with ChatGPT4
    // Scale 8-bit RGB to 16-bit RGB565
    uint8_t r5 = red >> 3;   // Scale to 5 bits
    uint8_t g6 = green >> 2; // Scale to 6 bits
    uint8_t b5 = blue >> 3;  // Scale to 5 bits

    // Combine RGB565 components
    rgb565 = (r5 << 11) | (g6 << 5) | b5;
}

void rgbToHexString(uint8_t red, uint8_t green, uint8_t blue, char* hexString) { //Lines 110-113 generated with ChatGPT4
  // Format the string as "#RRGGBB"
  snprintf(hexString, 8, "#%02x%02x%02x", red, green, blue);
}

void textColorSwap(int r, int g, int b){ //Decides which color to display the text in (white or black) depending on the color displayed, for added contrast
  int rgbSum = r + g + b;
 if ((r <= 120 && g <= 120 && b <= 120) || rgbSum <= 250){
  textColor = 0xffff;
 }
 else {
  textColor = 0x0000;
 }
}

void update(){
  defineRgb565Color(red, green, blue);
  textColorSwap(red, green, blue);
 }

void colorModeButton(){ //Switch for displaying color values in RGB or Hex
  switch (state) {
  case 1:
  displayRGB();
  break;
  case 2:
  displayHex();
  break;
  }
}

void displayRGB(){
  const char* lineRGB = "RGB Value:";
  char rgbString[20];
  snprintf(rgbString, sizeof(rgbString), "%d, %d, %d", red, green, blue);

  int charWidth = 17;   // Character width
  int textWidth1 = charWidth * strlen(lineRGB); // Line Width
  int textWidth2 = charWidth * strlen(rgbString);
  int x1 = (LCD_WIDTH - textWidth1) / 2; // Center first line horizontally
  int x2 = (LCD_WIDTH - textWidth2) / 2; // Center second line horizontally

  Paint_DrawString_EN(x1, 100, lineRGB, &Font24, WHITE, textColor);
  Paint_DrawString_EN(x2, 130, rgbString, &Font24, WHITE, textColor);
}

void displayHex(){
  const char* lineHex = "Hex Value:";
  rgbToHexString(red, green, blue, hexCode);

  int charWidth = 17;   // Character width
  int textWidth1 = charWidth * strlen(lineHex); // Line width
  int textWidth2 = charWidth * strlen(hexCode);
  int x1 = (LCD_WIDTH - textWidth1) / 2; // Center first line horizontally
  int x2 = (LCD_WIDTH - textWidth2) / 2; // Center second line horizontally

  Paint_DrawString_EN(x1, 100, lineHex, &Font24, WHITE, textColor);
  Paint_DrawString_EN(x2, 130, hexCode, &Font24, WHITE, textColor);
}

void toggleMode() {
    state = (state == 1) ? 2 : 1;
    LCD_Clear(rgb565); // Clear the screen
    colorModeButton(); // Update the display with the current mode
}

void sensor() { // Code inspired by the Adafruit TCS34725 colorview example code
        uint16_t redSense, greenSense, blueSense, clearSense;

        // Get raw data from the sensor
        tcs.setInterrupt(false);
        tcs.getRawData(&redSense, &greenSense, &blueSense, &clearSense);
        tcs.setInterrupt(true);

        // Avoid division by zero
        if (clearSense == 0) clearSense = 1;

        // Account for the clear channel
        float normalizedRed = (redSense / (float)clearSense) * 255.0;
        float normalizedGreen = (greenSense / (float)clearSense) * 255.0;
        float normalizedBlue = (blueSense / (float)clearSense) * 255.0;

        normalizedRed *= redCalibration;
        normalizedGreen *= greenCalibration;
        normalizedBlue *= blueCalibration;

        // Set normalized values 0–255
        normalizedRed = min(max(normalizedRed, 0), 255);
        normalizedGreen = min(max(normalizedGreen, 0), 255);
        normalizedBlue = min(max(normalizedBlue, 0), 255);

       // Map normalized values using min/max cutoffs
        if (normalizedRed >= redMax) {
            red = 255;
        } else if (normalizedRed <= redMin) {
            red = 0;
        } else {
            red = mapFloat((int)normalizedRed, redMin, redMax, 0, 255);
        }

        if (normalizedGreen >= greenMax) {
            green = 255;
        } else if (normalizedGreen <= greenMin) {
            green = 0;
        } else {
            green = mapFloat((int)normalizedGreen, greenMin, greenMax, 0, 255);
        }

        if (normalizedBlue >= blueMax) {
            blue = 255;
        } else if (normalizedBlue <= blueMin) {
            blue = 0;
        } else {
            blue = mapFloat((int)normalizedBlue, blueMin, blueMax, 0, 255);
        }

       red = gammatable[red];
       green = gammatable[green];
       blue = gammatable[blue];

        defineRgb565Color(red, green, blue); // Update the RGB565 value based on the sensor reading
        LCD_Clear(rgb565); // Display the new color
        colorModeButton(); // Display which color value to show on screen
        update(); // Refresh the display
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) { // Lines 236-238 generated with ChatGPT4
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

 void checkButtonPress(int buttonPin, unsigned long singleClickActionTime, unsigned long doubleClickActionTime) { // Lines 240-281 generated with ChatGPT4
    static unsigned long lastPressTime = 0; // Time of the last button press
    static unsigned long lastReleaseTime = 0; // Time of the last button release
    static int pressCount = 0;              // Count of consecutive presses
    static bool prevButtonState = HIGH;     // Previous state of the button

    unsigned long currentTime = millis();   // Get the current time
    bool currentButtonState = digitalRead(buttonPin); // Read the button state

    // Detect button press (transition from HIGH to LOW)
    if (currentButtonState == LOW && prevButtonState == HIGH) {
        pressCount++;
        lastPressTime = currentTime; // Update last press time
    }

    // Detect button release (transition from LOW to HIGH)
    if (currentButtonState == HIGH && prevButtonState == LOW) {
        lastReleaseTime = currentTime; // Update last release time
    }

    // Check for single or double click
    if (currentButtonState == HIGH) { // Only act when button is released
        if (pressCount == 1 && (currentTime - lastReleaseTime) > singleClickActionTime) {
            // Single-click detected
            sensor();
            Serial.println("Button Pressed");
            pressCount = 0; // Reset press count
        } else if (pressCount == 2 && (currentTime - lastPressTime) < doubleClickActionTime) {
            // Double-click detected
            toggleMode();
            Serial.println("Button Pressed");
            pressCount = 0; // Reset press count
        }

        // Reset press count if time exceeds the double-click window
        if (currentTime - lastPressTime > doubleClickActionTime) {
            pressCount = 0;
        }
    }

    prevButtonState = currentButtonState; // Update the previous state
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
