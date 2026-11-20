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
 *
 */

#include "../../inc/MarlinConfig.h"

#if HAS_BED_PROBE

#include "../gcode.h"
#include "../../module/motion.h"
#include "../../module/probe.h"

/**
 * M730: Hellbot — seleccionar el dispositivo de homing de Z.
 *   M730 S0 : homing de Z por endstop Z_MIN (fin de carrera, sin probe)
 *   M730 S1 : homing de Z por probe (BLTouch)
 *   M730    : reportar el estado actual
 */
void GcodeSuite::M730() {
  if (parser.seenval('S')) {
    homing_with_probe = parser.value_bool();
    if (!homing_with_probe) probe.stow(); // Asegurar el probe recogido si se pasa a endstop
  }
  SERIAL_ECHO_START();
  SERIAL_ECHOLNPGM("Z homing via ", homing_with_probe ? "PROBE (BLTouch)" : "ENDSTOP (Z_MIN)", " - use M730 S0/S1");
}

#endif // HAS_BED_PROBE
