/***************************************************
 * Coffee Extraction Assistant
 * CASA 0016
 * Ziyi Wang
 * v3 - 10th Dec 2025
 * Calibration and Improvement
 ***************************************************/

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"
#include <Adafruit_MAX31865.h>

// -- OLED --
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(128, 64, &Wire);

// -- HX711 --
#define LOADCELL_DOUT 4
#define LOADCELL_SCK 5
HX711 scale;
float calibration_factor = 400.0;

// -- MAX31865 --
Adafruit_MAX31865 thermo = Adafruit_MAX31865(10, 11, 12, 13); 
#define RREF      430.0
#define RNOMINAL  100.0   // PT100

// -- Flow --
unsigned long lastTime = 0;
float lastWeight = 0;
float flowRate = 0;  // g/s

// -- Temperature (placeholder) --
float tempC = -999;
float tempOffset = 20.0;   // Temp offset

// -- Advice Thresholds --
float idealFlowMin = 1.5;  // g/s
float idealFlowMax = 2.5;  // g/s
float idealTempMin = 88.0;    // °C
float idealTempMax = 96.0;    // °C

// -- Reset Logic --
bool resetPending = false;
unsigned long resetTriggerTime = 0;  // The moment exceed 2000g

// ------------------------------------------------
//                      SETUP
// ------------------------------------------------
void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1)
      ;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("OK");
  display.display();
  delay(500);

  scale.begin(LOADCELL_DOUT, LOADCELL_SCK);
  scale.set_scale(calibration_factor);
  scale.tare();
  thermo.begin(MAX31865_3WIRE);
}

// ------------------------------------------------
//      FULLSCREEN COUNTDOWN (RESET X.Xs)
// ------------------------------------------------
void showResetCountdown(float remainingSec) {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);

  // Centered title
  display.setCursor(20, 10);
  display.println("RESET...");

  // Centered seconds
  display.setTextSize(3);
  display.setCursor(30, 35);
  display.print(remainingSec, 1);
  display.print("s");

  display.display();
}

// ------------------------------------------------
//                        LOOP
// ------------------------------------------------
void loop() {
    // -- Temperature Read (PT100) --
  uint16_t rtd = thermo.readRTD();
  tempC = thermo.temperature(RNOMINAL, RREF);
  tempC += tempOffset;

  uint8_t fault = thermo.readFault();
  if (fault) {
    tempC = -999;
    thermo.clearFault();
  }

  // -- Weight --
  float weight = scale.get_units(3);
  if (weight < 0) weight = 0;

  // -- RESET Logic --
  // Step 1: detect instantaneous >2000g to start countdown
  if (!resetPending && weight > 2000) {
    resetPending = true;
    resetTriggerTime = millis();
  }

  // Step 2: countdown, then reset after 3 seconds
  // If auto-reset countdown is active
  if (resetPending) {

    float elapsed = (millis() - resetTriggerTime) / 1000.0;
    float remaining = 3.0 - elapsed;

    if (remaining > 0) {
      // Show fullscreen countdown
      showResetCountdown(remaining);
      return;  // Do not draw normal UI
    } else {
      // Time's up: perform RESET
      scale.tare();
      lastWeight = 0;
      resetPending = false;

      // Show 0.0 screen briefly
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(30, 25);
      display.println("RESET");
      display.display();
      delay(300);
    }
  }

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
  if (flowRate < idealFlowMin) flowArrow = '^';       // Pour Faster
  else if (flowRate > idealFlowMax) flowArrow = 'v';  // Pour Slower
  else flowArrow = '-';

  // Temp Advice
  if (tempC > -100) {                                 // If real Temp
    if (tempC < idealTempMin) tempArrow = '^';       // Hotter
    else if (tempC > idealTempMax) tempArrow = 'v';  // Colder
    else tempArrow = '-';
  } else {
    tempArrow = '-';  // Without Temp sensor: "-"
  }

  // -- OLED Display --
  display.clearDisplay();
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("Coffee Extraction Assistant");

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
