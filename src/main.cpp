#include <Arduino.h>
#include <FastLED.h>
#define ARM_MATH_CM4
#include <arm_math.h>
#include <algorithm>    // std::sort
#include <Visualization.h>
#include <Sparkle.h>
#include <Spectrum2.h>

using namespace std;

// // DEFINE PINS HERE
#define AUDIO_INPUT_PIN A12  // (38) Input pin for audio data.

// WS2811_PORTD: 2,14,7,8,6,20,21,5 - 8 way parallel


////////////////////////////////////////////////////////////////////////////////////////////////
// LED AND VISUALIZATION
////////////////////////////////////////////////////////////////////////////////////////////////

// GEOMETRY CONSTANTS
const uint_fast8_t rows = 100;
const uint_fast8_t columns = 1;
const uint_fast16_t numLEDs = rows * columns;

// COLORS
const uint_fast8_t saturation = 244;
const uint_fast8_t brightness = 224;

const uint_fast8_t pinkHue = 240;
const uint_fast8_t blueHue = 137;
const uint_fast8_t greenHue = 55;
CRGB off = 0x000000;

// STATE
uint_fast8_t currentBrightness = brightness;

// LED display array
CRGB leds[numLEDs];


// FUNTION DEFINITIONS
void clear();
void setAll(CRGB color);
uint_fast16_t xy2Pos(uint_fast16_t x, uint_fast16_t y);

////////////////////////////////////////////////////////////////////////////////////////////////
// ACTIONS / CONTROLS
////////////////////////////////////////////////////////////////////////////////////////////////

// STATE
uint_fast32_t loops = 0;
uint_fast32_t setupTime = 0;


void setup() {
  Serial.begin(9600);	// Debugging only

  // wait 10 seconds for serial to come up, or bail
  // pio device monitor must be started in this 10 second window
  // this is a raspberry pi thing
  while(!Serial && millis() < 10000);

  Serial.println("setup");


  FastLED.addLeds<WS2811_PORTD,columns>(leds, rows);
  FastLED.setDither(1);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2400);
  FastLED.setBrightness(currentBrightness);

  // INDICATE BOOT SEQUENCE
  setAll(0x001000);
  FastLED.show();
  delay(3000);

  // DISPLAY STUFF
  clear();
  FastLED.show();
  Serial.println("cleared");

  Serial.println("setup complete");
  setupTime = millis();
}

uint_fast32_t loggingTimestamp = 0;

// LOOP
void loop() {

  loops++;
  clear();  // this just sets the array, no reason it can't be at the top
  uint_fast32_t currentTime = millis();

  // put things we want to log here
  if (currentTime > loggingTimestamp + 5000) {
  // if (false) {
    loggingTimestamp = currentTime;

    Serial.print(currentTime);

    Serial.print("\tFrame Rate: ");
    Serial.print(loops / ((currentTime - setupTime) / 1000));
    Serial.println("");
  }

  // MAIN DISPLAY
  setAll(0x000040);

  // uint_fast16_t i = (loops/10) % numLEDs;
  // leds[i] = 0x0000FF;

  FastLED.show();
}

// /LOOP


void setAll(CRGB color) {
  for (uint_fast16_t i=0; i<numLEDs; i++) {
    leds[i] = color;
  }
}

void clear() {
  setAll(off);
}

uint_fast16_t xy2Pos(uint_fast16_t x, uint_fast16_t y) {
  uint_fast16_t pos = x * rows;
  pos = pos + y;

  return pos;
}
