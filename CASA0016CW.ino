/***************************************************
 * Coffee Extraction Assistant
 * CASA 0016
 * Ziyi Wang
 * v1 - 8th Dec 2025
 ***************************************************/

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"

// -- OLED --
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(128, 64, &Wire);

// -- HX711 --
#define LOADCELL_DOUT 4
#define LOADCELL_SCK 5
HX711 scale;
float calibration_factor = 7050.0;

// -- Flow --
unsigned long lastTime = 0;
float lastWeight = 0;
float flowRate = 0; // g/s

// -- Temperature (placeholder) --
float tempC = -999;

// -- Suggestion Thresholds --
float idealFlowMin = 1.5; // g/s
float idealFlowMax = 2.5; // g/s
float idealTemp = 92.0;   // °C

// ------------------------------------------------
//                      SETUP
// ------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("Coffee Helper with Suggestions...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("OLED OK");
  display.display();
  delay(500);

  scale.begin(LOADCELL_DOUT, LOADCELL_SCK);
  scale.set_scale(calibration_factor);
  scale.tare();
}

// ------------------------------------------------
//                        LOOP
// ------------------------------------------------
void loop() {

  // -- Weight --
  float weight = scale.get_units(3);
  if (weight < 0) weight = 0;

  // -- Flow Calculation --
  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;

  if (dt > 0.2) {
    flowRate = (weight - lastWeight) / dt;
    if (flowRate < 0) flowRate = 0;
    lastWeight = weight;
    lastTime = now;
  }

  // -- Advice --
  char flowArrow = '-';
  char tempArrow = '-';

  // Flow Advice
  if (flowRate < idealFlowMin)      flowArrow = '^'; // Pour Faster
  else if (flowRate > idealFlowMax) flowArrow = 'v'; // Pour Slower
  else                              flowArrow = '-'; // Fine

  // Temp Advice
  if (tempC > -100) {  // If real Temp
    if (tempC < idealTemp - 1)      tempArrow = '^'; // Hotter
    else if (tempC > idealTemp + 1) tempArrow = 'v'; // Colder
    else                            tempArrow = '-';
  } else {
    tempArrow = '-';  // Without Temp sensor: "-"
  }

  // -- OLED Display --
  display.clearDisplay();
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("Coffee Extraction Helper");

  display.setCursor(0, 16);
  display.print("Weight: ");
  display.print(weight, 1);
  display.println(" g");

  display.setCursor(0, 28);
  display.print("Flow:   ");
  display.print(flowRate, 2);
  display.println(" g/s");

  display.setCursor(0, 40);
  display.print("Temp:   ");
  if (tempC < -100) display.println("--.- C");
  else {
    display.print(tempC, 1);
    display.println(" C");
  }

  // -- Advice Arrows --
  display.setCursor(0, 54);
  display.print("Flow ");
  display.print(flowArrow);
  display.print("   Temp ");
  display.print(tempArrow);

  display.display();

  delay(50);
}
