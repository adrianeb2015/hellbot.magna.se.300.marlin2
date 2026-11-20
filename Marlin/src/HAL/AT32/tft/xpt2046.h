/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "../../../inc/MarlinConfig.h"

#include <SPI.h>

// The XPT2046 shares the TFT SPI bus, so its pins are the TFT pins.
#ifndef TOUCH_MISO_PIN
  #define TOUCH_MISO_PIN TFT_MISO_PIN
#endif
#ifndef TOUCH_MOSI_PIN
  #define TOUCH_MOSI_PIN TFT_MOSI_PIN
#endif
#ifndef TOUCH_SCK_PIN
  #define TOUCH_SCK_PIN  TFT_SCK_PIN
#endif
#if !PIN_EXISTS(TOUCH_CS)
  #error "TOUCH_CS_PIN is not defined."
#endif

#ifndef TOUCH_INT_PIN
  #define TOUCH_INT_PIN  -1
#endif

// The XPT2046 tops out well below the display clock
#ifndef SPI_CLOCK_MAX_TOUCH
  #define SPI_CLOCK_MAX_TOUCH 2500000  // 2.5 MHz
#endif

#ifndef TFT_SPI_MODE
  #define TFT_SPI_MODE SPI_MODE0
#endif

#define XPT2046_DFR_MODE        0x00
#define XPT2046_SER_MODE        0x04
#define XPT2046_CONTROL         0x80

enum XPTCoordinate : uint8_t {
  XPT2046_X  = 0x10 | XPT2046_CONTROL | XPT2046_DFR_MODE,
  XPT2046_Y  = 0x50 | XPT2046_CONTROL | XPT2046_DFR_MODE,
  XPT2046_Z1 = 0x30 | XPT2046_CONTROL | XPT2046_DFR_MODE,
  XPT2046_Z2 = 0x40 | XPT2046_CONTROL | XPT2046_DFR_MODE,
};

#ifndef XPT2046_Z1_THRESHOLD
  #define XPT2046_Z1_THRESHOLD 10
#endif

class XPT2046 {
private:
  static bool isBusy() { return false; }

  static uint16_t getRawData(const XPTCoordinate coordinate);
  static bool isTouched();

  static void dataTransferBegin();
  static void dataTransferEnd();

  static uint16_t hardwareIO(uint16_t data);
  static uint16_t softwareIO(uint16_t data);
  static uint16_t IO(uint16_t data=0);

public:
  static void init();
  static bool getRawPoint(int16_t * const x, int16_t * const y);
};
