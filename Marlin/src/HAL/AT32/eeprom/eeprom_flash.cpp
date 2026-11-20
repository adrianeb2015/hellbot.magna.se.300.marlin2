/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2026 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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
 */

/**
 * AT32F403A: EEPROM emulation in the LAST 4KB flash sector (0x0807F000).
 * The Marlin binary occupies ~310KB, far below this sector. Erase+rewrite
 * only happen on access_finish() when the buffer changed (M500).
 */

#ifdef ARDUINO_ARCH_AT32

#include "../../../inc/MarlinConfig.h"

#if ENABLED(FLASH_EEPROM_EMULATION)

#include "../../shared/eeprom_api.h"

#ifndef MARLIN_EEPROM_SIZE
  #define MARLIN_EEPROM_SIZE 0x1000 // 4KB (one AT32 sector)
#endif
size_t PersistentStore::capacity() { return MARLIN_EEPROM_SIZE - eeprom_exclude_size; }

#define EEPROM_FLASH_ADDR 0x0807F000UL

#define _ALIGN(x) __attribute__ ((aligned(x)))
static uint8_t _ALIGN(4) eeprom_buffer[MARLIN_EEPROM_SIZE];
static bool eeprom_dirty = false;

bool PersistentStore::access_start() {
  // Load current sector into RAM buffer
  const uint32_t *src = (const uint32_t *)EEPROM_FLASH_ADDR;
  uint32_t *dst = (uint32_t *)eeprom_buffer;
  for (uint16_t i = 0; i < MARLIN_EEPROM_SIZE / 4; i++)
    dst[i] = src[i];
  eeprom_dirty = false;
  return true;
}

bool PersistentStore::access_finish() {
  if (!eeprom_dirty) return true;

  // Only rewrite if the content actually differs from flash
  const uint32_t *src = (const uint32_t *)EEPROM_FLASH_ADDR;
  const uint32_t *buf = (const uint32_t *)eeprom_buffer;
  bool same = true;
  for (uint16_t i = 0; i < MARLIN_EEPROM_SIZE / 4; i++) {
    if (src[i] != buf[i]) { same = false; break; }
  }
  if (same) return true;

  #define FLASH_TIMEOUT 50000
  flash_unlock();
  flash_status_type st = flash_sector_erase(EEPROM_FLASH_ADDR);
  if (st != FLASH_OPERATE_DONE) { flash_lock(); return false; }

  for (uint32_t off = 0; off < MARLIN_EEPROM_SIZE; off += 4) {
    uint32_t w;
    memcpy(&w, eeprom_buffer + off, 4);
    st = flash_word_program(EEPROM_FLASH_ADDR + off, w);
    if (st != FLASH_OPERATE_DONE) { flash_lock(); return false; }
  }
  flash_lock();
  eeprom_dirty = false;
  return true;
}

bool PersistentStore::read_data(int &pos, uint8_t *value, size_t size, uint16_t *crc, const bool writing/*=true*/) {
  for (size_t i = 0; i < size; i++) {
    uint8_t c = eeprom_buffer[pos];
    if (writing && value) *value++ = c;
    crc16(crc, &c, 1);
    pos++;
  }
  return false;
}

bool PersistentStore::write_data(int &pos, const uint8_t *value, size_t size, uint16_t *crc) {
  while (size--) {
    uint8_t v = *value;
    if (v != eeprom_buffer[pos]) {
      eeprom_buffer[pos] = v;
      eeprom_dirty = true;
    }
    crc16(crc, &v, 1);
    pos++;
    value++;
  }
  return false;
}

#endif // FLASH_EEPROM_EMULATION
#endif // ARDUINO_ARCH_AT32
