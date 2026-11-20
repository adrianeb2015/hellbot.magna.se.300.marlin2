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

#ifndef LCD_READ_ID
  #define LCD_READ_ID 0x04   // Read display identification information (0xD3 on ILI9341)
#endif
#ifndef LCD_READ_ID4
  #define LCD_READ_ID4 0xD3   // Read display identification information (0xD3 on ILI9341)
#endif

#ifndef TFT_MISO_PIN
  #define TFT_MISO_PIN -1   // Panels that never reply omit this
#endif

#ifndef TFT_SPI_MODE
  #define TFT_SPI_MODE SPI_MODE0
#endif

// The XPT2046 tops out well below the display clock
#ifndef SPI_CLOCK_MAX_TFT
  #define SPI_CLOCK_MAX_TFT 20000000  // 20 MHz - ILI9488 is happy with this
#endif

#define DATA_SIZE_8BIT  SPI_FRAME_8BIT
#define DATA_SIZE_16BIT SPI_FRAME_16BIT
#define DATASIZE_8BIT   DATA_SIZE_8BIT
#define DATASIZE_16BIT  DATA_SIZE_16BIT
#define TFT_IO_DRIVER   TFT_SPI
#define DMA_MAX_WORDS   0xFFFF
#define DMA_MINC_ENABLE 1
#define DMA_MINC_DISABLE 0

class TFT_SPI {
private:
  static uint8_t dataSize;

  static uint32_t readID(const uint16_t inReg);
  static void transmit(uint16_t data);
  static void transmit(uint32_t memoryIncrease, uint16_t *data, uint16_t count);

public:
  // Dedicated SPIClass instance for the TFT bus (SPI2), shared with XPT2046.
  // Marlin's global `SPI` object stays bound to SPI1 for the SD card.
  static SPIClass &SPIx;

  static void init();
  static uint32_t getID();

  // All transfers are blocking, so there is never anything in flight.
  static bool isBusy() { return false; }
  static void abort() { dataTransferEnd(); }
  static void dataTransferAbort() { dataTransferEnd(); }

  static void dataTransferBegin(uint16_t dataWidth=DATASIZE_16BIT);
  static void dataTransferEnd();

  static void writeData(uint16_t data) { transmit(data); }
  static void writeReg(const uint16_t inReg) {
    WRITE(TFT_A0_PIN, LOW);
    SPIx.transfer(uint8_t(inReg));  // Commands are always a single byte
    WRITE(TFT_A0_PIN, HIGH);
  }

  // No DMA engine is used, so the "DMA" entry points are the blocking ones.
  static void writeSequence_DMA(uint16_t *data, uint16_t count) { writeSequence(data, count); }
  static void writeMultiple_DMA(uint16_t color, uint16_t count) { writeMultiple(color, count); }

  static void writeSequence(uint16_t *data, uint16_t count) {
    dataTransferBegin();
    transmit(DMA_MINC_ENABLE, data, count);
    dataTransferEnd();
  }
  static void writeMultiple(uint16_t color, uint32_t count) {
    dataTransferBegin();
    while (count > 0) {
      const uint16_t chunk = count > DMA_MAX_WORDS ? DMA_MAX_WORDS : (uint16_t)count;
      SPIx.write(color, chunk);
      count -= chunk;
    }
    dataTransferEnd();
  }
};
