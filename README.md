# ESP32-ES3C40P

> **Status: public project skeleton in active development.** This repository is a starting point for further work, not a finished product or production-ready firmware. Display and touch support are experimental and still need to be verified on the hardware.

This project provides a basic PlatformIO configuration for the LCDWIKI ES3C40P module with an ESP32-S3. The current program initializes the ST7796S display and FT6336U touch controller, then creates a simple LVGL demo screen. Application-specific features will be developed later.

## Hardware

| Component | Controller / interface | ESP32-S3 pins |
|---|---|---|
| 320 × 480 display | ST7796S, SPI | CS: GPIO10, DC: GPIO46, SCK: GPIO12, MOSI: GPIO11, MISO: GPIO13 |
| Backlight | GPIO control | GPIO45 |
| Touchscreen | FT6336U, I²C, address `0x38` | SDA: GPIO16, SCL: GPIO15 |
| Touch reset and interrupt | GPIO | RESET: GPIO18, INT: GPIO17 |

The display reset is shared with the ESP32-S3 module reset. The ST7796S initialization sequence in the code is based on LCDWIKI's ES3C40P resources.

## Requirements

- PlatformIO Core or PlatformIO for VS Code
- ESP32-S3 ES3C40P module with PSRAM
- USB cable that supports data transfer

LVGL and Arduino_GFX dependencies are installed by PlatformIO as specified in `platformio.ini`.

## Build, upload, and monitor

Run these commands from the project directory:

```sh
pio run -e es3c40p
pio run -e es3c40p -t upload
pio device monitor -e es3c40p
```

The serial monitor is configured for `115200` baud. If PlatformIO does not detect the correct port, set `upload_port` and `monitor_port` for the `es3c40p` environment in your local configuration.

## Display rotation

The third argument in the `display` constructor in `src/main.cpp` (after the bus pointer and reset pin) selects the rotation: `0`, `1`, `2`, or `3`. Rotations of 90° and 270° change the display orientation and dimensions. Before using either orientation, update the LVGL resolution, UI layout, and touch coordinates accordingly. The current display and UI configuration does not automatically synchronize these settings when this argument changes.

## Status and limitations

- This is a project skeleton. The LVGL demo screen is not the final application interface.
- The `es3c40p` environment has been built successfully, but correct image output on the physical module has not yet been confirmed.
- Touch and gesture handling are experimental and require further testing on the device.
- Changes to rotation, touch mapping, and panel settings require hardware testing.

## Hardware documentation

- [ES3C40P User Manual](https://www.lcdwiki.com/res/ES3C40P/4.0inch_IPS_ESP32-S3_ES3C40P_User_Manual.pdf)
- [ST7796S initialization sequence for ES3C40P](https://www.lcdwiki.com/res/ES3C40P/ST7796_Init.txt)
