#include <Arduino.h>
#include <FastLED.h>

using namespace std;

// // DEFINE PINS HERE
// WS2811_PORTD: 2,14,7,8,6,20,21,5 - 8 way parallel


////////////////////////////////////////////////////////////////////////////////////////////////
// LED AND VISUALIZATION
////////////////////////////////////////////////////////////////////////////////////////////////

// GEOMETRY CONSTANTS
const uint_fast8_t rows = 88;
const uint_fast8_t columns = 4;
const uint_fast16_t numLEDs = rows * columns;

CRGB off = 0x000000;

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

  // SETUP LEDS
  // Parallel  Pin layouts on the teensy 3/3.1:
  // // WS2811_PORTD: 2,14,7,8,6,20,21,5

  FastLED.addLeds<WS2811_PORTD,columns>(leds, rows);
  FastLED.setDither(1);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2400);
  FastLED.setBrightness(200);

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
bool latch  = false;

// LOOP
void loop() {

  CRGB c = CHSV(25, 60, 75);

  setAll(c);
  FastLED.show();
  delay(1000);
}

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
