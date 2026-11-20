# Marlin 2.1.x — Hellbot Magna SE 300 (AT32F403ARGT7)

Fork de Marlin bugfix-2.1.x con soporte nativo para la **Hellbot Magna SE 300**
(placa clon "Wanhao 0327001 v2.0" con CPU **Artery AT32F403ARGT7**, clon
Cortex-M4 de STM32F407), incluyendo **Linear Advance**, pantalla **MKS TS35-R
V2.0** (ST7796 480x320 + touch XPT2046), **EEPROM emulada en flash**, **BLTouch**
y homing de Z conmutable endstop/probe.

Este README documenta TODO lo necesario para reproducir el port: pinout real de
la placa, limitaciones del hardware, parches al HAL AT32, bugs del core de
Arduino-AT32 encontrados (con sus fixes), build y flasheo.

---

## 1. Hardware (todo verificado en placa, no asumido)

| Componente | Valor |
|---|---|
| CPU | AT32F403ARGT7 (1 MB flash, **SOLO 96 KB SRAM visibles** en esta unidad) |
| SRAM | 0x20000000–0x20017FFF. El option-byte EOPB0 viene configurado a 96 KB de fábrica. El linker stock asume 224 KB → SP inválido → HardFault (loop en el handler, pantalla negra, sin serie) |
| Cristal | 8 MHz |
| Serie host | CH340 en **USART1 (PA9/PA10)**, 115200 |
| Steppers | TMC2225 en modo **STANDALONE** (sin UART a los drivers → sin M122). Microstepping real: **1/32** |
| Pantalla | MKS TS35-R V2.0 (variante resistiva): panel ST7796S 480x320 por SPI2, touch XPT2046 |
| SWD | PA13 = SWDIO = **pin del endstop X_MIN** (usado por el factory sin remapear el SWJ). Cuidado al conectar una sonda: puede interferir con el endstop X |

### 1.1 Pinout real (extraído del firmware de fábrica + verificado por SWD)

Fuente primaria: código fuente original de fábrica
(github.com/carlynos/Magna-SE-32-bits-Open-Source-original-, `main_RobinNano.h`
+ `gpio_RobinNano.c`) y lectura por SWD de la config GPIO del factory corriendo.

#### Steppers (EN compartido PB1, activo-bajo)

| Función | Pin |
|---|---|
| X_STEP / X_DIR | PB12 / PB10 |
| Y_STEP / Y_DIR | PB0 / PC5 |
| Z_STEP / Z_DIR | PA1 / **PA0** |
| E0_STEP / E0_DIR | PC3 / PC2 |
| EN (X/Y/E0) | PB1 |
| EN (Z) | PC4 |

> **PA0 es también TMR2_CH1** — y TMR2 es el timer de STEP del HAL. Ver la
> sección 4.2 (bug del Servo) antes de tocar cualquier código que use `PIN_MAP`.

#### Endstops (pull-up interno, switch a GND, activo-LOW)

| Función | Pin | Nota |
|---|---|---|
| X_MIN | **PA13** | Pin SWDIO del SWJ |
| Y_MIN | PB9 | |
| Z_MIN | PC13 | |
| Z_MAX | PA14 | Pin SWCLK del SWJ (sin usar) |

#### Heaters / fans / misc

| Función | Pin |
|---|---|
| TEMP_0 / TEMP_BED | PC1 / PC0 |
| HEATER_0 / HEATER_BED | PC8 / PC9 |
| FAN0 / FAN1 | PC6 / PC7 |
| BEEPER | PC12 |
| Filament runout | PC14 |
| SERVO0 (BLTouch) | PA8 |

#### TFT MKS TS35-R V2.0

| Función | Pin |
|---|---|
| SPI2: SCK / MISO / MOSI | PB13 / PB14 / PB15 |
| TFT_CS / DC(RS) | PB8 / PA11 |
| TOUCH_CS (XPT2046, comparte SPI2) | PA12 |
| Backlight / Reset | PC10 / PC11 |

#### SD (SPI1 compartido con flash W25Q) — DESACTIVADA en esta config

| Función | Pin |
|---|---|
| SCK / MISO / MOSI | PB5 / PB6 / PB7 |
| CS SD / CS flash | PD2 / PA4 |

> La SD crashea el firmware: el SPI global del core va a SPI1 con los pines
> default del core, no a PB5/PB6/PB7. Pendiente mapear SPI1 en el core.

---

## 2. Configuración clave (Marlin/Configuration*.h)

- `MOTHERBOARD BOARD_WANHAO_0327001` (env `WANHAO_0327001`, offset de carga
  **0x7000** por el bootloader Hellbot; bin PLAIN renombrado `Robin_nano.bin`)
- `BAUDRATE 115200` (el CH340 no negocia 250000)
- `DEFAULT_AXIS_STEPS_PER_UNIT { 160, 160, 800, 400 }` — **valores reales del
  factory** (robin_nano_cfg.txt). TMC2225 strappeados a 1/32. Con 80/80/400/500
  los ejes van a MEDIA velocidad y el homing falla desde lejos.
- Cama 300x300, Z_MAX 400
- `MINIMUM_STEPPER_PULSE_NS 2000` — sin esto, `cycles.h` deriva pulsos de 0.5 µs
  que los drivers **ignoran** (motor energizado pero sin pasos)
- `INVERT_Z_DIR true` (verificado con DIR funcionando: `G1 Z-10` baja)
- `INVERT_X_DIR false`, `INVERT_Y_DIR false`, `INVERT_E0_DIR false`
- Pantalla: `MKS_TS35_V2_0` + `TFT_COLOR_UI` + `TOUCH_SCREEN` +
  `TFT_ROTATE_270`, backlight PC10 / reset PC11
- EEPROM: `FLASH_EEPROM_EMULATION` (backend propio, sector 0x0807F000, 4 KB,
  lejos del bin ~330 KB). M500/M501/M502 funcionan sin SD.
- SDSUPPORT **desactivada** (crashea, ver 1.1)
- `VALIDATE_HOMING_ENDSTOPS` activo (kill si el endstop no dispara durante el
  move de homing — muy útil en este port)
- `PINS_DEBUGGING` activo (M43) para diagnóstico en vivo

### 2.1 BLTouch + homing conmutable (custom de este fork)

- `BLTOUCH`, `USE_PROBE_FOR_Z_HOMING`, `Z_SAFE_HOMING`
- Probe en `Z_MIN_PROBE_PIN = PA8` (mismo pin que SERVO0)
- `NOZZLE_TO_PROBE_OFFSET` es PLACEHOLDER `{10,10,0}`: medir y ajustar con
  `M851 X.. Y.. Z..` + `M500`
- **`M730 S1/S0`** o menú Motion → "Home Z: probe": conmuta el homing de Z entre
  endstop PC13 (default al arrancar) y BLTouch.
  - Con probe OFF: ni siquiera mueve XY al centro (el endstop va en el carro, la
    posición XY es irrelevante) — homing Z directo.
  - Con probe ON: Z_SAFE_HOMING completo al punto seguro con offset del probe.

---

## 3. HAL AT32 — parches necesarios (ya en este árbol)

### 3.1 SRAM 96 KB

El script `buildroot/share/PlatformIO/scripts/hellbot_ram96k.py` (pre-script del
env) fuerza `_estack = 0x20018000` y `RAM 96K` en el linker AT32 antes de cada
build. **Sin esto el firmware HardFault al arrancar** (SP cae en RAM inexistente:
CFSR=0x1400 IBUSERR+PRECISERR). RAM usada: data+bss ≈ 12 KB + heap/stack.

### 3.2 Timers

`HAL/AT32/timers.cpp`: tras cada `Timer_SetInterruptBase()` hay que llamar
`Timer_SetEnable(TMRx, true)` — el core deja el contador PARADO; sin ISR de
temperatura el WDT (5 s) resetea el firmware en bucle cada ~4.7 s.

TMR2 = step timer, TMR4 = temperature timer.

### 3.3 Serie

- CH340 está en **USART1** (`SERIAL_PORT 1`), no USART3.
- El core configura RX sin pull-up → parche en
  `libcore/HardwareSerial.cpp` (`gpio_pull = GPIO_PULL_UP`), ya aplicado si se
  usa el core patcheado.
- `MarlinSerial.h`: `AT32SerialAdapter` con `emergency_state` +
  `#include e_parser.h`; `emergency_parser.cpp`: guardar la llamada a update()
  con `ENABLED(EMERGENCY_PARSER)`; `sd/SDCard.cpp`: include de SdVolume dentro
  de `#if HAS_SDCARD`.

### 3.4 Servo (HAL/AT32/Servo.h + Servo.cpp — wrapper propio)

Wrapper `MarlinServo` (estilo HC32) con fixes:

- `typedef uint16_t servo_angle_t` (no existe en el core)
- forward-declaration de la clase antes del `extern MarlinServo* servos[]`
- `servos[]` y `servoCount` **sin static** (el .cpp los definía static y el .h
  extern → undefined reference)
- `attach(const int apin)` para permitir `attach(-1)`
- **CRÍTICO**: `attach()` hace no-op si ya está attached (ver 4.1)
- `OPTIMISTIC_SERVO_READ`

### 3.5 TFT (HAL/AT32/tft + port)

Backend TFT propio sobre SPI2 con presets `MKS_TS35_V2_0`: backlight PC10,
reset PC11, `TFT_A0_PIN` = DC. Touch XPT2046 compartiendo SPI2 con CS PA12.
`SDCARD_EEPROM_EMULATION` en pins desactivado a favor de FLASH_EEPROM_EMULATION.

---

## 4. Bugs encontrados en ArduinoCore-AT32F4 (importante para quien porte)

### 4.1 `Servo::attach()` — hijack del pin PA0 / Z_DIR (CRÍTICO)

En `cores/arduino/Servo.cpp`:

```cpp
// Servo::attach(pin):
if (attached) { detach(); }   // detach() termina con  pin = 0 !
...
configure_timer();  // usa pin=0 → reconfigura TMR2 como timer de servo (50 Hz)
configure_pin();    // usa pin=0 → gpio_init(PA0, GPIO_MODE_MUX)
```

- PA0 es **Z_DIR** en esta placa y también **TMR2_CH1**. Resultado: el DIR de Z
  queda muxado a AF del timer (CFGLR nibble = 0x9), `digitalWrite` no tiene
  efecto, el eje Z se mueve en un solo sentido y TMR2 (¡el timer de steps!)
  queda reconfigurado a 50 Hz.
- Trigger: cualquier `move()` del servo después del attach inicial (deploy/stow
  del BLTouch, M280, toggle de homing).
- **Fix en este fork**: el wrapper `MarlinServo::attach` hace no-op si ya está
  attached. (Fix alternativo: corregir el core para no pisar `pin` en detach.)

### 4.2 Otros bugs del core ya conocidos

- `Timer_SetInterruptBase()` deja el contador parado (ver 3.2)
- RX de serie sin pull-up (ver 3.3)
- `gpio_init()` global siempre sobre GPIOA en Servo (rompe si el servo está en
  otro puerto)
- SPI global va a SPI1 con pines default (rompe la SD real en PB5-7)

---

## 5. Comportamiento de endstops / homing (custom)

### 5.1 USE_Z_MIN restaurado

Marlin stock compila FUERA el endstop Z_MIN cuando se usa `USE_PROBE_FOR_Z_HOMING`
con probe en pin propio (`Conditionals-5-post.h` no define `USE_Z_MIN`) → la
impresora queda sin fin de carrera Z y el homing "se va al infinito". Parche:
`USE_Z_MIN` se define siempre que exista el pin; `endstops.cpp` ya ignora Z_MIN
mientras `z_probe_enabled == true`, lo que da el comportamiento deseado:

- Probe OFF → homing Z por PC13 (y sin mover XY al centro)
- Probe ON → homing Z por BLTouch en el centro de la cama

### 5.2 M730 — seleccionar dispositivo de homing de Z

```
M730      ; reportar estado
M730 S0   ; homing Z por endstop Z_MIN (PC13)
M730 S1   ; homing Z por probe (BLTouch)
```

También en el menú LCD: **Motion → "Home Z: probe"**. El flag NO se persiste en
EEPROM: siempre arranca en modo endstop (seguro si el BLTouch no está instalado).

Archivos: `module/motion.{h,cpp}` (flag `homing_with_probe` + gates de
deploy/stow/bump), `gcode/probe/M730.cpp`, `lcd/menu/menu_motion.cpp`,
`language_en.h` (`MSG_HOME_Z_PROBE`), `ini/features.ini` (build_src_filter).

---

## 6. Calibración de la máquina (EEPROM de fábrica)

Valores de `robin_nano_cfg.txt` del firmware original (referencia):

```
steps/mm        160 / 160 / 800 / 400
cama            300 x 300 x 400
invert dir      X0 Y0 Z1 E1
```

**Importante tras flashear**: la EEPROM en flash puede contener settings viejos.
Ejecutar `M502` (factory reset) y `M500` (guardar) o los pasos viejos volverán
por `M501` y el homing volverá a fallar.

---

## 7. Build

```bash
pip install platformio   # o usar el core dir propio
cd hellbot-marlin2
pio run -e WANHAO_0327001
# salida: .pio/build/WANHAO_0327001/Robin_nano.bin  (PLAIN, ~330 KB)
```

Requisitos del env (`ini/at32.ini`): platform
`MarlinFirmware/platform-at32`, framework `ArduinoCore-AT32F4` (fork
MarlinFirmware, branch `marlin`), toolchain arm-none-eabi 12.x.

Salida: bin PLAIN a cargar en 0x08007000 (offset 0x7000) — el updater SD de la
placa lo escribe ahí sin transformaciones (verificado byte a byte).

## 8. Flasheo

1. **Por SD (normal)**: copiar `Robin_nano.bin` a la SD, la placa lo flashea al
   arrancar (updater del bootloader Hellbot). Restauración de fábrica:
   flashear el `Robin_nano.bin` original de Hellbot (`Firmware - Magna SE 300`).
2. **Por SWD**: probe ESP32 (firmware esp32-stlink-probe de AVATAR3905, parcheado
   para aceptar el DPIDR del AT32). Puerto 5555 protocolo nativo:
   `'P'` program+verify (4 KB/llamada, ~9 min), `'X'` leer, `'H'`/`'C'` halt/resume,
   `'M'` escribir memoria, `'A'` ping. GDB-RSP 3333 solo flash. Solo un cliente
   TCP a la vez. Scripts en el árbol hermano `klipper-magna-se/swd/`.

## 9. Estado / pendientes

- ✅ Marlin 2.1.x estable (0 reboots), serie 115200, Linear Advance compilado
- ✅ Pantalla TS35-R + touch con calibración persistente
- ✅ EEPROM flash emulation (M500/M501)
- ✅ Homing X/Y/Z por endstop + toggle probe/endstop (M730)
- ✅ BLTouch compilado (deploy/stow/probing) — falta instalar en máquina
- ✅ PA0/Z_DIR bug documentado y evitado
- ⬜ SD card (crashea; mapear SPI1 a PB5-7 en el core)
- ⬜ Medir `NOZZLE_TO_PROBE_OFFSET` real (placeholder {10,10,0})
- ⬜ Pressure advance: `M900 K0.05` como punto de partida y torre de calibración
- ⬜ (opcional) EOPB0 por SWD para habilitar los 224 KB de SRAM
