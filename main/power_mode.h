#ifndef POWER_MODE_H
#define POWER_MODE_H

#include "bme680_reg.h"


typedef enum
{
    MODE_M0 = 0,
    MODE_M1,
    MODE_M2,
    MODE_M3,
    MODE_M4,
    MODE_M5,
    MODE_M6,
    MODE_M7,
    MODE_M8
} power_mode_t;


/* ============================================================
 * Wi-Fi state
 * ============================================================ */

typedef enum
{
    WIFI_OFF = 0,
    WIFI_ON
} wifi_state_t;


typedef enum
{
    BLUETOOTH_OFF = 0,
    BLUETOOTH_ON
} bluetooth_state_t;

typedef enum
{
    OLED_OFF = 0,
    OLED_ON
} oled_state_t;

typedef enum
{
    ESP32_ACTIVE = 0,
    ESP32_MODEM_SLEEP,
    ESP32_LIGHT_SLEEP,
    ESP32_DEEP_SLEEP
} esp32_power_mode_t;


typedef struct
{
    esp32_power_mode_t esp32_power_mode;
    wifi_state_t wifi_state;
    bluetooth_state_t bluetooth_state;
    bme680_mode_t bme680_mode;
    oled_state_t oled_state;
} power_mode_config_t;

void power_mode_apply(power_mode_t mode);

#endif