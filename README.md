# stand
Simple sound reactive stand.  Heavily based on the wizard staff.

## Hardware

1. Teensy 3.2
2. [Adafruit Microphone](https://www.adafruit.com/product/1713?gclid=Cj0KCQjwzbv7BRDIARIsAM-A6-0N_Y3rPPO2U0hVi-99e05u9NTAavjTPQEBxR4LVZKZus9y4a0CU98aAgd7EALw_wcB)
3. Some WS2812B LEDs

## How to get going

Download and install Visual Studio code and install the PlatformIO plugin.

Run:

```
git submodule update --init --recursive

~/.platformio/penv/bin/pio run --target upload
```
