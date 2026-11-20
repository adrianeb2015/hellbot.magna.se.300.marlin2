/**
 * Hellbot Magna SE (Wanhao 0327001 v2.0) — AT32F403ARGT7
 *
 * PINMAP REAL extraído del código fuente original de fábrica:
 *   github.com/carlynos/Magna-SE-32-bits-Open-Source-original-
 *   Inc/main_RobinNano.h + Src/gpio_RobinNano.c  (clon MKS Robin Nano)
 * Verificado también por lectura SWD de la configuración GPIO del
 * firmware de fábrica en ejecución (todas las salidas/niveles coinciden).
 *
 * Drivers: TMC2225 en modo STANDALONE (sin UART — usar A4988/TMC2208_STANDALONE).
 * SD: SPI1 compartido con flash (PB5/PB6/PB7), CS SD = PD2, CS flash = PA4.
 * TFT: MKS TS35-R V2.0 en SPI2 (PB13/PB14/PB15), CS TFT = PB8,
 *      touch XPT2046 CS = PA12, DC/RS = PA11.
 * X_MIN (PA13) y Z_MAX (PA14) son los pines SWD: el factory los usa como
 * entradas con pull-up sin remapear el SWJ.
 */

#include "env_validate.h"

#define BOARD_INFO_NAME "Hellbot Magna SE"

//
// Limit Switches (fábrica: entradas con pull-up, switch a GND)
//
#define X_STOP_PIN                          PA13   // XMIN (pin SWDIO del SWJ)
#define Y_STOP_PIN                          PB9    // YMIN
#define Z_MIN_PIN                           PC13   // ZMIN
#define Z_MAX_PIN                           PA14   // ZMAX (pin SWCLK del SWJ)

//
// Steppers (enable compartido como en fábrica)
//
#define X_STEP_PIN                          PB12
#define X_DIR_PIN                           PB10
#define X_ENABLE_PIN                        PB1

#define Y_STEP_PIN                          PB0
#define Y_DIR_PIN                           PC5
#define Y_ENABLE_PIN                        PB1

#define Z_STEP_PIN                          PA1
#define Z_DIR_PIN                           PA0
#define Z_ENABLE_PIN                        PC4

#define E0_STEP_PIN                         PC3
#define E0_DIR_PIN                          PC2
#define E0_ENABLE_PIN                       PB1

//
// Temperature Sensors
//
#define TEMP_0_PIN                          PC1    // TH1 (ADC)
#define TEMP_BED_PIN                        PC0    // TB  (ADC)

//
// Heaters / Fans
//
#define HEATER_0_PIN                        PC8    // HEATER1
#define HEATER_BED_PIN                      PC9    // BED
#define FAN0_PIN                            PC6    // E_FAN
#define FAN1_PIN                            PC7    // FAN2

//
// Misc
//
#define BEEPER_PIN                          PC12
#define FIL_RUNOUT_PIN                      PC14   // MT_DET
#define SERVO0_PIN                          PA8    // BL_TOUCH
#define Z_MIN_PROBE_PIN                     PA8    // BLTouch: señal probe en el mismo pin servo (PWM deploy/stow + entrada endstop)

//
// SD card: SPI1 compartido con flash (PB5=SCK, PB6=MISO, PB7=MOSI)
//
#define SCK_PIN                             PB5
#define MISO_PIN                            PB6
#define MOSI_PIN                            PB7
#define SD_SS_PIN                           PD2    // SD_CS

//
// TFT MKS TS35-R V2.0 (SPI2) + touch XPT2046
//
#define TFT_SCK_PIN                         PB13
#define TFT_MISO_PIN                        PB14
#define TFT_MOSI_PIN                        PB15
#define TFT_CS_PIN                          PB8
#define TFT_DC_PIN                          PA11   // RS
#define TFT_A0_PIN                 TFT_DC_PIN      // SPI TFT command/data select
#define TOUCH_CS_PIN                        PA12

#ifndef SDCARD_CONNECTION
  #define SDCARD_CONNECTION               ONBOARD
#endif

// Hellbot: EEPROM emulada en flash (ultimo sector 0x0807F000), backend HAL/AT32/eeprom/eeprom_flash.cpp
#define FLASH_EEPROM_EMULATION

// Hellbot: backlight y reset del panel TS35 (Src/spi_lcd.c del firmware de fabrica:
// LCD_GPIOInit -> PC10 = TFT_BLK, PC11 = TFT_RESET)
#define TFT_BACKLIGHT_PIN                   PC10
#define TFT_RESET_PIN                       PC11
