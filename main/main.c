#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "bme680_reg.h"
#include "display_oled.h"
#include "power_mode.h"
#include "wifi.h"

#include "nvs_flash.h"
#include "mqtt.h"


static const char *TAG = "MAIN";


#define CURRENT_POWER_MODE    MODE_M6


static bool mode_requires_mqtt(power_mode_t mode)
{
    switch (mode)
    {
        case MODE_M3:
        case MODE_M5:
            return true;

        default:
            return false;
    }
}


static const char *power_mode_to_string(power_mode_t mode)
{
    switch (mode)
    {
        case MODE_M0:
            return "M0";

        case MODE_M1:
            return "M1";

        case MODE_M2:
            return "M2";

        case MODE_M3:
            return "M3";

        case MODE_M4:
            return "M4";

        case MODE_M5:
            return "M5";

        case MODE_M6:
            return "M6";

        case MODE_M7:
            return "M7";

        case MODE_M8:
            return "M8";

        default:
            return "UNKNOWN";
    }
}


static void mqtt_task(void *pvParameters)
{
    (void)pvParameters;

    ESP_LOGI(
        TAG,
        "Waiting for Wi-Fi connection..."
    );


    int wifi_wait_count = 0;


    while (!wifi_is_connected())
    {
        vTaskDelay(
            pdMS_TO_TICKS(500)
        );


        wifi_wait_count++;

        if (wifi_wait_count >= 60)
        {
            ESP_LOGE(
                TAG,
                "Wi-Fi connection timeout."
            );


            vTaskDelete(NULL);

            return;
        }
    }


    ESP_LOGI(
        TAG,
        "Wi-Fi is ready. Starting MQTT..."
    );


    mqtt_manager_init();


    ESP_LOGI(
        TAG,
        "Waiting for MQTT Broker connection..."
    );


    int mqtt_wait_count = 0;


    while (!mqtt_manager_is_connected())
    {
        vTaskDelay(
            pdMS_TO_TICKS(500)
        );


        mqtt_wait_count++;

        if (mqtt_wait_count >= 60)
        {
            ESP_LOGE(
                TAG,
                "MQTT connection timeout."
            );


            vTaskDelete(NULL);

            return;
        }
    }


    ESP_LOGI(
        TAG,
        "MQTT is ready. Starting periodic publish."
    );


    while (1)
    {
        const char *mode_string =
            power_mode_to_string(
                CURRENT_POWER_MODE
            );

        char payload[256];


        snprintf(
            payload,
            sizeof(payload),
            "{"
            "\"device\":\"ESP32-ENERGY\","
            "\"mode\":\"%s\","
            "\"status\":\"active\","
            "\"message\":\"MQTT test\""
            "}",
            mode_string
        );



        bool ret = mqtt_manager_publish(
            MQTT_TOPIC_DATA,
            payload,
            0
        );


        if (ret)
        {
      
            ESP_LOGI(
                TAG,
                "MQTT publish successful."
            );
        }
        else
        {
            ESP_LOGW(
                TAG,
                "MQTT publish failed."
            );
        }


        vTaskDelay(
            pdMS_TO_TICKS(5000)
        );
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();


    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(
            TAG,
            "NVS partition needs to be erased."
        );


        ret = nvs_flash_erase();


        if (ret != ESP_OK)
        {
            ESP_LOGE(
                TAG,
                "NVS erase failed: %s",
                esp_err_to_name(ret)
            );

            return;
        }


        ret = nvs_flash_init();
    }


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "NVS initialization failed: %s",
            esp_err_to_name(ret)
        );

        return;
    }


    esp_log_level_set(
        "wifi",
        ESP_LOG_WARN
    );


    esp_log_level_set(
        "NimBLE",
        ESP_LOG_WARN
    );


    ESP_LOGI(
        TAG,
        "========================================"
    );

    ESP_LOGI(
        TAG,
        "ESP32 POWER MODE TEST"
    );

    ESP_LOGI(
        TAG,
        "========================================"
    );

    ret = bme680_reg_init();


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "BME680 initialization failed: %s",
            esp_err_to_name(ret)
        );

        return;
    }

    ret = display_oled_init();


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "OLED initialization failed: %s",
            esp_err_to_name(ret)
        );

        return;
    }

    ESP_LOGI(
        TAG,
        "Applying power mode..."
    );


    power_mode_apply(
        CURRENT_POWER_MODE
    );


    if (mode_requires_mqtt(CURRENT_POWER_MODE))
    {
        BaseType_t task_ret = xTaskCreate(
            mqtt_task,
            "mqtt_task",
            4096,
            NULL,
            5,
            NULL
        );


        if (task_ret != pdPASS)
        {
            ESP_LOGE(
                TAG,
                "Failed to create MQTT task."
            );

            return;
        }


        ESP_LOGI(
            TAG,
            "MQTT task created."
        );
    }
    else
    {
        ESP_LOGI(
            TAG,
            "MQTT is disabled for this power mode."
        );
    }


    while (1)
    {
        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }
}