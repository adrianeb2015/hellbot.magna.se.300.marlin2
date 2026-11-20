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

#include "../../platforms.h"

#ifdef ARDUINO_ARCH_AT32

#include "../../../inc/MarlinConfig.h"

#if HAS_SPI_TFT

#include "tft_spi.h"

//#define DEBUG_TFT_IO
#define DEBUG_OUT ENABLED(DEBUG_TFT_IO)
#include "../../../core/debug_out.h"

// Dedicated SPIClass on SPI2 for the TFT + touch bus. The global `SPI`
// object instantiated in HAL_SPI.cpp stays bound to SPI1 for the SD card.
static SPIClass tftSPI(SPI_CLASS_2_SPI);
SPIClass &TFT_SPI::SPIx = tftSPI;

uint8_t TFT_SPI::dataSize = DATA_SIZE_16BIT;

void TFT_SPI::init() {
  #if PIN_EXISTS(TFT_RESET)
    OUT_WRITE(TFT_RESET_PIN, HIGH);
    delay(100);
  #endif

  OUT_WRITE(TFT_A0_PIN, HIGH);
  OUT_WRITE(TFT_CS_PIN, HIGH);

  // begin() configures SPI2 on PB13/PB14/PB15 at 8-bit, mode 0, MSB first,
  // with software CS — which is exactly what the TS35-R panel needs. CS is
  // driven by hand around each transfer.
  SPIx.begin();
  SPIx.setClock(SPI_CLOCK_MAX_TFT);
  SPIx.setDataMode(TFT_SPI_MODE);
  SPIx.setDataSize(DATA_SIZE_16BIT);
  dataSize = DATA_SIZE_16BIT;
}

void TFT_SPI::dataTransferBegin(uint16_t dataWidth) {
  dataSize = uint8_t(dataWidth);
  SPIx.setDataSize(dataWidth);
  WRITE(TFT_CS_PIN, LOW);
}

void TFT_SPI::dataTransferEnd() {
  WRITE(TFT_CS_PIN, HIGH);
  SPIx.setDataSize(DATA_SIZE_16BIT);
  dataSize = DATA_SIZE_16BIT;
}

void TFT_SPI::transmit(uint16_t data) {
  if (dataSize == DATA_SIZE_8BIT)
    SPIx.transfer(uint8_t(data));
  else
    SPIx.write(data);
}

void TFT_SPI::transmit(uint32_t memoryIncrease, uint16_t *data, uint16_t count) {
  if (dataSize == DATA_SIZE_8BIT) {
    uint8_t *bytes = (uint8_t *)data;
    const uint16_t n = uint16_t(count) * 2;
    if (memoryIncrease == DMA_MINC_ENABLE)
      SPIx.write(bytes, n);
    else
      for (uint16_t i = 0; i < n; i += 2) SPIx.write(bytes[0]);
  }
  else if (memoryIncrease == DMA_MINC_ENABLE)
    SPIx.write(data, count);
  else
    for (uint16_t i = 0; i < count; i++) SPIx.write(data[0]);
}

#include "../../../lcd/tft_io/tft_ids.h"

uint32_t TFT_SPI::getID() {
  DEBUG_ECHOLNPGM("TFT_SPI::getID()");

  uint32_t id = readID(LCD_READ_ID);
  #if ENABLED(DEBUG_TFT_IO)
    char debug_register[3], debug_value[5];
    sprintf_P(debug_register, PSTR("%02X"), LCD_READ_ID);
    sprintf_P(debug_value, PSTR("%04X"), uint16_t(id));
    DEBUG_ECHOLNPGM("  readID(0x", debug_register, ") : 0x", debug_value);
  #endif

  if ((id & 0xFFFF) == 0 || (id & 0xFFFF) == 0xFFFF) {
    id = readID(LCD_READ_ID4);
    #if ENABLED(DEBUG_TFT_IO)
      sprintf_P(debug_register, PSTR("%02X"), LCD_READ_ID4);
      sprintf_P(debug_value, PSTR("%04X"), uint16_t(id));
      DEBUG_ECHOLNPGM("  readID(0x", debug_register, ") : 0x", debug_value);
    #endif
  }

  #ifdef TFT_DEFAULT_DRIVER
    if ((id & 0xFFFF) == 0 || (id & 0xFFFF) == 0xFFFF) {
      id = TFT_DEFAULT_DRIVER;
      #if ENABLED(DEBUG_TFT_IO)
        sprintf_P(debug_value, PSTR("%04X"), uint16_t(id));
        DEBUG_ECHOLNPGM("  Fallback to TFT_DEFAULT_DRIVER : 0x", debug_value);
      #endif
    }
  #endif

  return id;
}

uint32_t TFT_SPI::readID(const uint16_t inReg) {
  uint32_t data = 0;

  #if PIN_EXISTS(TFT_MISO)
    // The ID read must be slow. Many panels also return the value shifted by
    // one bit, which is corrected on the way out.
    SPIx.setClock(SPI_CLOCK_MAX_TFT / 16);
    SPIx.setDataSize(DATA_SIZE_8BIT);
    WRITE(TFT_CS_PIN, LOW);

    WRITE(TFT_A0_PIN, LOW);
    SPIx.transfer(uint8_t(inReg));
    WRITE(TFT_A0_PIN, HIGH);

    for (uint8_t i = 0; i < 4; ++i)
      data = (data << 8) | SPIx.transfer(0x00);

    WRITE(TFT_CS_PIN, HIGH);
    SPIx.setClock(SPI_CLOCK_MAX_TFT);
    SPIx.setDataSize(DATA_SIZE_16BIT);
    dataSize = DATA_SIZE_16BIT;
  #endif

  return data >> 7;
}

#endif // HAS_SPI_TFT
#endif // ARDUINO_ARCH_AT32
