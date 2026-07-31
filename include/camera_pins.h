#pragma once

/*
 * Camera pin map — selected by the CAMERA_MODEL_* macro, which the PlatformIO
 * environment passes in as a build flag (see platformio.ini):
 *
 *   pio run -e seeed_xiao_esp32s3  ->  -DCAMERA_MODEL_XIAO_ESP32S3
 *   pio run -e esp32cam            ->  -DCAMERA_MODEL_AI_THINKER
 *
 * app_config.h falls back to the XIAO map if nothing is defined, so the code
 * still builds outside those environments.
 *
 * Defines:
 *   CAM_PIN_PWDN, CAM_PIN_RESET, CAM_PIN_XCLK
 *   CAM_PIN_SIOD, CAM_PIN_SIOC
 *   CAM_PIN_D0..D7, CAM_PIN_VSYNC, CAM_PIN_HREF, CAM_PIN_PCLK
 *   BOARD_NAME  — human-readable string for the boot log
 *
 * Adding a board: add an #elif block here, add a matching [env:...] to
 * platformio.ini with -DCAMERA_MODEL_<yours>, and — if its flash size or PSRAM
 * mode differs from the two below — a sdkconfig.defaults.<target> file.
 */

#include "app_config.h"

#if defined(CAMERA_MODEL_XIAO_ESP32S3)

/* ── Seeed Studio XIAO ESP32S3 Sense (OV2640) ───────────────────────────────
 * Pin map from Seeed's official camera_pins.h (CAMERA_MODEL_XIAO_ESP32S3).
 * PWDN and RESET are not wired out — leave at -1.                            */
#define BOARD_NAME "Seeed XIAO ESP32S3 Sense"
#define CAM_PIN_PWDN  -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK  10
#define CAM_PIN_SIOD  40
#define CAM_PIN_SIOC  39
#define CAM_PIN_D7    48
#define CAM_PIN_D6    11
#define CAM_PIN_D5    12
#define CAM_PIN_D4    14
#define CAM_PIN_D3    16
#define CAM_PIN_D2    18
#define CAM_PIN_D1    17
#define CAM_PIN_D0    15
#define CAM_PIN_VSYNC 38
#define CAM_PIN_HREF  47
#define CAM_PIN_PCLK  13

#elif defined(CAMERA_MODEL_AI_THINKER)

/* ── ESP32-CAM style boards (OV2640 / OV3660) ───────────────────────────────
 * The AI Thinker ESP32-CAM pinout. DFRobot and the other ESP32-CAM clones use
 * the same module and the same pins, so this one map covers them all.
 * RESET is not wired out — leave at -1.
 *
 * Note GPIO 0 doubles as XCLK and as the bootstrap pin: it must be pulled to
 * GND to enter the bootloader, and released before the camera will run.       */
#define BOARD_NAME "ESP32-CAM (AI Thinker pinout)"
#define CAM_PIN_PWDN  32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK  0
#define CAM_PIN_SIOD  26
#define CAM_PIN_SIOC  27
#define CAM_PIN_D7    35
#define CAM_PIN_D6    34
#define CAM_PIN_D5    39
#define CAM_PIN_D4    36
#define CAM_PIN_D3    21
#define CAM_PIN_D2    19
#define CAM_PIN_D1    18
#define CAM_PIN_D0    5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF  23
#define CAM_PIN_PCLK  22

#else
#error "No camera board selected — build with an env from platformio.ini, or define CAMERA_MODEL_XIAO_ESP32S3 / CAMERA_MODEL_AI_THINKER in app_config.h"
#endif
