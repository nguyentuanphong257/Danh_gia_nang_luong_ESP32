#include "power_mode.h"

#include "wifi.h"
#include "bluetooth.h"
#include "display_oled.h"

#include "esp_err.h"

#include <stdio.h>


static void apply_esp32_power_mode(esp32_power_mode_t mode)
{
    switch (mode)
    {
        case ESP32_ACTIVE:

            printf("ESP32 power state: ACTIVE\n");

            break;


        case ESP32_MODEM_SLEEP:

            printf("ESP32 power state: MODEM SLEEP\n");

            break;


        case ESP32_LIGHT_SLEEP:

            printf("ESP32 power state: LIGHT SLEEP\n");

            break;


        case ESP32_DEEP_SLEEP:

            printf("ESP32 power state: DEEP SLEEP\n");

            break;


        default:

            printf("ESP32 power state: UNKNOWN\n");

            break;
    }
}

static void apply_wifi_state(wifi_state_t state)
{
    esp_err_t ret;


    switch (state)
    {
        case WIFI_OFF:


            ret = wifi_off();

            if (ret != ESP_OK)
            {
                printf(
                    "Failed to turn off Wi-Fi: %s\n",
                    esp_err_to_name(ret));
            }
            else
            {
                printf(
                    "Wi-Fi turned off successfully\n");
            }

            break;


        case WIFI_ON:

            ret = wifi_on();

            if (ret != ESP_OK)
            {
                printf(
                    "Failed to turn on Wi-Fi: %s\n",
                    esp_err_to_name(ret));
            }
            else
            {
                printf(
                    "Wi-Fi turned on successfully\n");
            }

            break;


        default:

            printf("Wi-Fi: UNKNOWN\n");

            break;
    }
}

static void apply_bluetooth_state(bluetooth_state_t state)
{
    esp_err_t ret;


    switch (state)
    {
        case BLUETOOTH_OFF:


            ret = bluetooth_off();

            if (ret != ESP_OK)
            {
                printf(
                    "Failed to turn off Bluetooth: %s\n",
                    esp_err_to_name(ret));
            }
            else
            {
                printf(
                    "Bluetooth turned off successfully\n");
            }

            break;


        case BLUETOOTH_ON:


            ret = bluetooth_on();

            if (ret != ESP_OK)
            {
                printf(
                    "Failed to turn on Bluetooth: %s\n",
                    esp_err_to_name(ret));
            }
            else
            {
                printf(
                    "Bluetooth turned on successfully\n");
            }

            break;


        default:

            printf("Bluetooth: UNKNOWN\n");

            break;
    }
}


static void apply_bme680_mode(bme680_mode_t mode)
{
    esp_err_t ret;


    switch (mode)
    {
        case BME680_MODE_SLEEP:

            printf("BME680: SLEEP\n");


            ret = bme680_set_mode(
                BME680_MODE_SLEEP);

            if (ret != ESP_OK)
            {
                printf(
                    "Failed to set BME680 SLEEP: %s\n",
                    esp_err_to_name(ret));
            }

            break;


        case BME680_MODE_FORCED:

            printf("BME680: FORCED\n");


            ret = bme680_set_mode(
                BME680_MODE_FORCED);

            if (ret != ESP_OK)
            {
                printf(
                    "Failed to set BME680 FORCED: %s\n",
                    esp_err_to_name(ret));
            }

            break;


        case BME680_MODE_NORMAL:

            printf("BME680: NORMAL\n");


            ret = bme680_set_mode(
                BME680_MODE_NORMAL);

            if (ret != ESP_OK)
            {
                printf(
                    "Failed to set BME680 NORMAL: %s\n",
                    esp_err_to_name(ret));
            }

            break;


        default:

            printf("BME680: UNKNOWN\n");

            break;
    }
}


static void apply_oled_state(
    oled_state_t state,
    const char *mode_name)
{
    esp_err_t ret;


    switch (state)
    {
        case OLED_OFF:


            ret = display_oled_off();

            if (ret != ESP_OK)
            {
                printf(
                    "Failed to turn off OLED: %s\n",
                    esp_err_to_name(ret));
            }
            else
            {
                printf(
                    "OLED turned off successfully\n");
            }

            break;


        case OLED_ON:

            ret = display_oled_on();

            if (ret != ESP_OK)
            {
                printf(
                    "Failed to turn on OLED: %s\n",
                    esp_err_to_name(ret));

                break;
            }


            ret = display_oled_show_mode(
                mode_name);

            if (ret != ESP_OK)
            {
                printf(
                    "Failed to display mode on OLED: %s\n",
                    esp_err_to_name(ret));
            }
            else
            {
                printf(
                    "OLED turned on and displaying %s\n",
                    mode_name);
            }

            break;


        default:

            printf("OLED: UNKNOWN\n");

            break;
    }
}


static void print_power_mode_summary(
    power_mode_t mode,
    const power_mode_config_t *config)
{
    const char *mode_name;


    switch (mode)
    {
        case MODE_M0:
            mode_name = "M0";
            break;

        case MODE_M1:
            mode_name = "M1";
            break;

        case MODE_M2:
            mode_name = "M2";
            break;

        case MODE_M3:
            mode_name = "M3";
            break;

        case MODE_M4:
            mode_name = "M4";
            break;

        case MODE_M5:
            mode_name = "M5";
            break;

        case MODE_M6:
            mode_name = "M6";
            break;

        case MODE_M7:
            mode_name = "M7";
            break;

        case MODE_M8:
            mode_name = "M8";
            break;

        default:
            mode_name = "UNKNOWN";
            break;
    }


    printf("\n");
    printf("========================================\n");
    printf("POWER MODE: %s\n", mode_name);
    printf("========================================\n");


    switch (config->esp32_power_mode)
    {
        case ESP32_ACTIVE:
            printf("ESP32      : ACTIVE\n");
            break;

        case ESP32_MODEM_SLEEP:
            printf("ESP32      : MODEM SLEEP\n");
            break;

        case ESP32_LIGHT_SLEEP:
            printf("ESP32      : LIGHT SLEEP\n");
            break;

        case ESP32_DEEP_SLEEP:
            printf("ESP32      : DEEP SLEEP\n");
            break;

        default:
            printf("ESP32      : UNKNOWN\n");
            break;
    }


    switch (config->wifi_state)
    {
        case WIFI_ON:
            printf("Wi-Fi      : ON\n");
            break;

        case WIFI_OFF:
            printf("Wi-Fi      : OFF\n");
            break;

        default:
            printf("Wi-Fi      : UNKNOWN\n");
            break;
    }


    switch (config->bluetooth_state)
    {
        case BLUETOOTH_ON:
            printf("Bluetooth  : ON\n");
            break;

        case BLUETOOTH_OFF:
            printf("Bluetooth  : OFF\n");
            break;

        default:
            printf("Bluetooth  : UNKNOWN\n");
            break;
    }


    switch (config->bme680_mode)
    {
        case BME680_MODE_SLEEP:
            printf("BME680     : SLEEP\n");
            break;

        case BME680_MODE_FORCED:
            printf("BME680     : FORCED\n");
            break;

        case BME680_MODE_NORMAL:
            printf("BME680     : NORMAL\n");
            break;

        default:
            printf("BME680     : UNKNOWN\n");
            break;
    }


    switch (config->oled_state)
    {
        case OLED_ON:
            printf("OLED       : ON\n");
            break;

        case OLED_OFF:
            printf("OLED       : OFF\n");
            break;

        default:
            printf("OLED       : UNKNOWN\n");
            break;
    }


    printf("========================================\n");
}


void power_mode_apply(power_mode_t mode)
{
    power_mode_config_t config;

    const char *mode_name;

    switch (mode)
    {
        case MODE_M0:
            mode_name = "M0";
            break;

        case MODE_M1:
            mode_name = "M1";
            break;

        case MODE_M2:
            mode_name = "M2";
            break;

        case MODE_M3:
            mode_name = "M3";
            break;

        case MODE_M4:
            mode_name = "M4";
            break;

        case MODE_M5:
            mode_name = "M5";
            break;

        case MODE_M6:
            mode_name = "M6";
            break;

        case MODE_M7:
            mode_name = "M7";
            break;

        case MODE_M8:
            mode_name = "M8";
            break;

        default:
            mode_name = "UNKNOWN";
            break;
    }


    switch (mode)
    {
       
        case MODE_M0:

            config.esp32_power_mode = ESP32_ACTIVE;

            config.wifi_state = WIFI_OFF;

            config.bluetooth_state = BLUETOOTH_OFF;

            config.bme680_mode = BME680_MODE_SLEEP;

            config.oled_state = OLED_OFF;

            break;


        case MODE_M1:

            config.esp32_power_mode = ESP32_ACTIVE;

            config.wifi_state = WIFI_OFF;

            config.bluetooth_state = BLUETOOTH_OFF;

            config.bme680_mode = BME680_MODE_FORCED;

            config.oled_state = OLED_OFF;

            break;


        case MODE_M2:

            config.esp32_power_mode = ESP32_ACTIVE;

            config.wifi_state = WIFI_OFF;

            config.bluetooth_state = BLUETOOTH_OFF;

            config.bme680_mode = BME680_MODE_FORCED;

            config.oled_state = OLED_ON;

            break;


        case MODE_M3:

            config.esp32_power_mode = ESP32_ACTIVE;

            config.wifi_state = WIFI_ON;

            config.bluetooth_state = BLUETOOTH_OFF;

            config.bme680_mode = BME680_MODE_FORCED;

            config.oled_state = OLED_ON;

            break;


        case MODE_M4:

            config.esp32_power_mode = ESP32_ACTIVE;

            config.wifi_state = WIFI_OFF;

            config.bluetooth_state = BLUETOOTH_ON;

            config.bme680_mode = BME680_MODE_FORCED;

            config.oled_state = OLED_ON;

            break;


        case MODE_M5:

            config.esp32_power_mode = ESP32_ACTIVE;

            config.wifi_state = WIFI_ON;

            config.bluetooth_state = BLUETOOTH_ON;

            config.bme680_mode = BME680_MODE_FORCED;

            config.oled_state = OLED_ON;

            break;


        case MODE_M6:

            config.esp32_power_mode = ESP32_MODEM_SLEEP;

            config.wifi_state = WIFI_ON;

            config.bluetooth_state = BLUETOOTH_OFF;

            config.bme680_mode = BME680_MODE_FORCED;

            config.oled_state = OLED_ON;

            break;

        case MODE_M7:

            config.esp32_power_mode = ESP32_LIGHT_SLEEP;

            config.wifi_state = WIFI_OFF;

            config.bluetooth_state = BLUETOOTH_OFF;

            config.bme680_mode = BME680_MODE_FORCED;

            config.oled_state = OLED_ON;

            break;


        case MODE_M8:

            config.esp32_power_mode = ESP32_DEEP_SLEEP;

            config.wifi_state = WIFI_OFF;

            config.bluetooth_state = BLUETOOTH_OFF;

            config.bme680_mode = BME680_MODE_SLEEP;

            config.oled_state = OLED_OFF;

            break;

        default:

            printf(
                "Unknown power mode: %d\n",
                mode);

            config.esp32_power_mode = ESP32_ACTIVE;

            config.wifi_state = WIFI_OFF;

            config.bluetooth_state = BLUETOOTH_OFF;

            config.bme680_mode = BME680_MODE_SLEEP;

            config.oled_state = OLED_OFF;

            break;
    }


    print_power_mode_summary(
        mode,
        &config);

    apply_esp32_power_mode(
        config.esp32_power_mode);


    apply_wifi_state(
        config.wifi_state);


    apply_bluetooth_state(
        config.bluetooth_state);


    apply_bme680_mode(
        config.bme680_mode);


    apply_oled_state(
        config.oled_state,
        mode_name);
}