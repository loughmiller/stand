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
#define MIC_AR 39
#define AUDIO_INPUT_PIN A19  // (38) Input pin for audio data.
#define MIC_GAIN 37
#define MIC_POWER 36
#define MIC_GROUND 35

// WS2811_PORTDC: 2,14,7,8,6,20,21,5,15,22 - 10 way parallel


////////////////////////////////////////////////////////////////////////////////////////////////
// LED AND VISUALIZATION
////////////////////////////////////////////////////////////////////////////////////////////////

// GEOMETRY CONSTANTS
const uint_fast8_t rows = 200;
const uint_fast8_t columns = 10;
const uint_fast16_t numLEDs = rows * columns;

// COLORS
const uint_fast8_t saturation = 244;
const uint_fast8_t brightness = 255;

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
Spectrum2 * spectrum1;
Spectrum2 * spectrum2;
Spectrum2 * spectrum3;
Spectrum2 * spectrum4;

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
const uint_fast16_t noteCount{50};              // how many notes are we trying to detect
const uint_fast16_t notesBelowMiddleA{33};

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
  // Power peripherals
  pinMode(MIC_AR, OUTPUT);
  // pinMode(MIC_GAIN, OUTPUT);
  pinMode(MIC_POWER, OUTPUT);
  pinMode(MIC_GROUND, OUTPUT);

  digitalWrite(MIC_GROUND, LOW);
  digitalWrite(MIC_POWER, HIGH);


  Serial.begin(9600);	// Debugging only
  Serial.println("setup");

  randomSeed(analogRead(A4));

  noteDetectionSetup();

  // SETUP LEDS
  // Parallel  Pin layouts on the teensy 3/3.1:
  // // WS2811_PORTD: 2,14,7,8,6,20,21,5
  // // WS2811_PORTC: 15,22,23,9,10,13,11,12,28,27,29,30 (these last 4 are pads on the bottom of the teensy)
  // WS2811_PORTDC: 2,14,7,8,6,20,21,5,15,22,23,9,10,13,11,12 - 16 way parallel

  FastLED.addLeds<WS2811_PORTDC,columns>(leds, rows);
  FastLED.setDither(1);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 4000);
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

  // spectrum1 = new Spectrum2(columns, rows, (rows / 4) - 1, noteCount,
  //   pinkHue, saturation, true, leds);
  // spectrum2 = new Spectrum2(columns, rows, (rows / 4), noteCount,
  //   pinkHue, saturation, false, leds);
  // spectrum3 = new Spectrum2(columns, rows, ((rows / 4) * 3) - 1 , noteCount,
  //   pinkHue, saturation, true, leds);
  // spectrum4 = new Spectrum2(columns, rows, (rows / 4) * 3, noteCount,
  //   pinkHue, saturation, false, leds);

  spectrum1 = new Spectrum2(columns, rows, 0, noteCount,
    pinkHue, saturation, false, leds);
  spectrum2 = new Spectrum2(columns, rows, (rows / 2) - 1, noteCount,
    pinkHue, saturation, true, leds);
  spectrum3 = new Spectrum2(columns, rows, (rows / 2), noteCount,
    pinkHue, saturation, false, leds);
  spectrum4 = new Spectrum2(columns, rows, rows - 1, noteCount,
    pinkHue, saturation, true, leds);

  spectrum1->setDrift(1);
  spectrum2->setDrift(1);
  spectrum3->setDrift(1);
  spectrum4->setDrift(1);

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
//     // Serial.println();
//     // Serial.print(rfTime/loops);
//     // Serial.print("\t");
//     // Serial.print(fftTime/loops);
//     // Serial.print("\t");
//     // Serial.print(batteryTime/loops);
//     // Serial.print("\t");
//     // Serial.print(fastLEDTime/loops);
    Serial.println("");
  }

//   // uint_fast32_t loopZero = millis();

//   // uint_fast32_t loopOne = millis();
//   // rfTime += loopOne - loopZero;

//   // MAIN DISPLAY

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // NOTE DETECTION
  ////////////////////////////////////////////////////////////////////////////////////////////////
  noteDetectionLoop();

  spectrum1->display(noteMagnatudes);
  spectrum2->display(noteMagnatudes);
  spectrum3->display(noteMagnatudes);
  spectrum4->display(noteMagnatudes);

  // uint_least32_t loopThree = millis();
  // fftTime += loopThree - loopTwo;

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // \ NOTE DETECTION
  ////////////////////////////////////////////////////////////////////////////////////////////////

  sparkle->display();

  FastLED.show();

//   // fastLEDTime += millis() - loopThree;
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
  float newDensity = min(0.8, spectrum1->getDensity() + 0.05);
  spectrum1->setDensity(newDensity);
  spectrum2->setDensity(newDensity);
  spectrum3->setDensity(newDensity);
  spectrum4->setDensity(newDensity);
}

void decreaseDensity() {
  float newDensity = max(0.05, spectrum1->getDensity() - 0.05);
  spectrum1->setDensity(newDensity);
  spectrum2->setDensity(newDensity);
  spectrum3->setDensity(newDensity);
  spectrum4->setDensity(newDensity);
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

  // if (x % 2 == 0) {
  //   pos = pos + y;
  // } else {
  //   pos = pos + ((rows - 1) - y);
  // }

  return pos;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE DETECTION
////////////////////////////////////////////////////////////////////////////////////////////////

void noteDetectionSetup() {
  digitalWrite(MIC_AR, LOW);
  // digitalWrite(MIC_GAIN, HIGH);
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

    // note = note % noteCount;  // I'd like a side by side of this vs the above break

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
