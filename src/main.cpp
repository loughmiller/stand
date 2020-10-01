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
const uint_fast8_t rows = 36;
const uint_fast8_t columns = 4;
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

// random sparkle object
Sparkle * sparkle;

// 4 note visualization objects:
Spectrum2 * spectrum;

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

// FUNCTIONS
void increaseBrightness();
void decreaseBrightness();
void increaseDensity();
void decreaseDensity();


////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE DETECTION
////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE DETECTION CONSTANTS
const uint_fast16_t fftSize{256};               // Size of the FFT.  Realistically can only be at most 256
const uint_fast16_t fftBinSize{8};              // Hz per FFT bin  -  sample rate is fftSize * fftBinSize
const uint_fast16_t sampleCount{fftSize * 2};   // Complex FFT functions require a coefficient for the imaginary part of the
                                                // input.  This makes the sample array 2x the fftSize
const float middleA{440.0};                     // frequency of middle A.  Needed for freqeuncy to note conversion
const uint_fast16_t sampleIntervalMs{1000000 / (fftSize * fftBinSize)};  // how often to get a sample, needed for IntervalTimer

// FREQUENCY TO NOTE CONSTANTS - CALCULATE HERE: https://docs.google.com/spreadsheets/d/1CPcxGFB7Lm6xJ8CePfCF0qXQEZuhQ-nI1TC4PAiAd80/edit?usp=sharing
const uint_fast16_t noteCount{rows};              // how many notes are we trying to detect
const uint_fast16_t notesBelowMiddleA{20};

// NOTE DETECTION GLOBALS
float samples[sampleCount*2];
uint_fast16_t sampleCounter = 0;
float sampleBuffer[sampleCount];
float magnitudes[fftSize];
float noteMagnatudes[noteCount];
arm_cfft_radix4_instance_f32 fft_inst;
IntervalTimer samplingTimer;

// NOTE DETECTION FUNCTIONS
void noteDetectionSetup();        // run this once during setup
void noteDetectionLoop();         // run this once per loop
void samplingCallback();


void setup() {
  Serial.begin(9600);	// Debugging only

  // wait 4 seconds for serial to come up, or bail
  while(!Serial && millis()<4000);

  Serial.println("setup");

  randomSeed(analogRead(A4));

  noteDetectionSetup();

  // SETUP LEDS
  // Parallel  Pin layouts on the teensy 3/3.1:
  // // WS2811_PORTD: 2,14,7,8,6,20,21,5

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

  Serial.println("Streaks Setup");

  sparkle = new Sparkle(numLEDs, 0, 0, leds, 10000);
  Serial.println("Sparkles!");

  spectrum = new Spectrum2(columns, rows, 0, noteCount,
    pinkHue, saturation, false, leds);

  spectrum->setDrift(50);
  spectrum->setDensity(.1);

  Serial.println("setup complete");
  setupTime = millis();
}

uint_fast32_t loggingTimestamp = 0;
bool latch  = false;

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
    Serial.print("\tmagnitude: ");
    Serial.print(spectrum->getMagnitude());
    Serial.println("");
  }

  // MAIN DISPLAY
  noteDetectionLoop();

  spectrum->display(noteMagnatudes);
  sparkle->display();

  float magnitude = spectrum->getMagnitude();

  if ((magnitude < 4000 && !latch) || (magnitude < 5000 && latch)) {
    uint_fast8_t hue = (currentTime / 1000) % 256;
    setAll(CHSV(hue, saturation, 96));

    uint_fast8_t magT = floor((int)magnitude/1000);
    uint_fast8_t magH = floor(((int)magnitude % 1000)/100);

    for(uint_fast8_t i = 0; i < magT; i++) {
      leds[xy2Pos(0, i)] = off;
    }

    for(uint_fast8_t j = 0; j < magH; j++) {
      leds[xy2Pos(1, j)] = off;
    }

    latch = true;
  } else {
    latch = false;
  }

  FastLED.show();
}

// /LOOP

// ACTIONS

void increaseBrightness() {
  currentBrightness = min((currentBrightness + 16), (uint_fast8_t) 240);
  FastLED.setBrightness(currentBrightness);
}

void decreaseBrightness() {
  currentBrightness = max((currentBrightness - 16), (uint_fast8_t)16);
  FastLED.setBrightness(currentBrightness);
}

void increaseDensity() {
  float newDensity = min(0.8, spectrum->getDensity() + 0.05);
  spectrum->setDensity(newDensity);
}

void decreaseDensity() {
  float newDensity = max(0.05, spectrum->getDensity() - 0.05);
  spectrum->setDensity(newDensity);
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

////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE DETECTION
////////////////////////////////////////////////////////////////////////////////////////////////

void noteDetectionSetup() {
  pinMode(AUDIO_INPUT_PIN, INPUT);
  analogReadResolution(10);
  analogReadAveraging(16);
  arm_cfft_radix4_init_f32(&fft_inst, fftSize, 0, 1);
  samplingTimer.begin(samplingCallback, sampleIntervalMs);
}

void noteDetectionLoop() {
  // copy the last N samples into a buffer
  memcpy(sampleBuffer, samples + (sampleCounter + 1), sizeof(float) * sampleCount);

  // FFT magic
  arm_cfft_radix4_f32(&fft_inst, sampleBuffer);
  arm_cmplx_mag_f32(sampleBuffer, magnitudes, fftSize);

  for (uint_fast16_t i=0; i<noteCount; i++) {
    noteMagnatudes[i] = 0;
  }

  // We're suppose to ignore the top half of the FFT results due to aliasing issues, however
  // we're not trying to sound good, we're trying to look good, and in this context it seems ok
  // There are bigger harmonic(?) issues due to our low sample rate and/or small FFT size.  This
  // looks cool, so we're rolling with it.  This should allow us ~60 notes, but we only need 50
  // due to current staff geometery.
  for (uint_fast16_t i=1; i<fftSize; i++) {
    // Serial.print(magnitudes[i]);
    // Serial.print("\t");
    float frequency = i * (fftBinSize);
    uint_fast16_t note = roundf(12 * (log(frequency / middleA) / log(2))) + notesBelowMiddleA;

    if (note < 0) {
      continue;
    }

    if (note >= noteCount) {
      break;
    }

    // Multiple bins can point to the same note, use the largest magnitude
    noteMagnatudes[note] = max(noteMagnatudes[note], magnitudes[i]);
  }
  // Serial.println("");
}

void samplingCallback() {
  // Read from the ADC and store the sample data
  float sampleData = (float)analogRead(AUDIO_INPUT_PIN);

  // storing the data twice in the ring buffer array allows us to do a single memcopy
  // to get an ordered buffer of the last N samples
  uint_fast16_t sampleIndex = (sampleCounter) * 2;
  uint_fast16_t sampleIndex2 = sampleIndex + sampleCount;
  samples[sampleIndex] = sampleData;
  samples[sampleIndex2] = sampleData;

  // Complex FFT functions require a coefficient for the imaginary part of the
  // input.  Since we only have real data, set this coefficient to zero.
  samples[sampleIndex+1] = 0.0;
  samples[sampleIndex2+1] = 0.0;

  sampleCounter++;
  sampleCounter = sampleCounter % fftSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// \ NOTE DETECTION
////////////////////////////////////////////////////////////////////////////////////////////////
