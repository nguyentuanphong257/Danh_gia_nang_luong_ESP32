#include "display_oled.h"

#include "bme680_reg.h"

#include "u8g2.h"

#include "driver/i2c_master.h"

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdint.h>
#include <stdio.h>

static const char *TAG = "OLED";

#define OLED_I2C_ADDRESS       0x3C
#define OLED_I2C_SPEED_HZ      400000

static u8g2_t u8g2;

static i2c_master_dev_handle_t oled_device = NULL;

static bool oled_initialized = false;


static uint8_t u8x8_byte_esp32_i2c(
    u8x8_t *u8x8,
    uint8_t msg,
    uint8_t arg_int,
    void *arg_ptr)
{
    static uint8_t buffer[136];
    static uint16_t buf_idx = 0;

    switch (msg)
    {
        case U8X8_MSG_BYTE_INIT:
        {
            
            break;
        }

        case U8X8_MSG_BYTE_START_TRANSFER:
        {
            buf_idx = 0;
            break;
        }

        case U8X8_MSG_BYTE_SEND:
        {
            uint8_t *data = (uint8_t *)arg_ptr;

            while (arg_int > 0)
            {
                if (buf_idx < sizeof(buffer))
                {
                    buffer[buf_idx++] = *data;
                }

                data++;
                arg_int--;
            }

            break;
        }

        case U8X8_MSG_BYTE_END_TRANSFER:
        {
            if (oled_device == NULL)
            {
                ESP_LOGE(
                    TAG,
                    "OLED I2C device is NULL"
                );

                return 0;
            }

            esp_err_t ret = i2c_master_transmit(
                oled_device,
                buffer,
                buf_idx,
                100
            );

            if (ret != ESP_OK)
            {
                ESP_LOGE(
                    TAG,
                    "OLED I2C transmit failed: %s",
                    esp_err_to_name(ret)
                );

                return 0;
            }

            break;
        }

        default:
            return 0;
    }

    return 1;
}


static uint8_t u8x8_gpio_and_delay_esp32(
    u8x8_t *u8x8,
    uint8_t msg,
    uint8_t arg_int,
    void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_DELAY_MILLI:
        {
           
            vTaskDelay(
                pdMS_TO_TICKS(arg_int)
            );

            break;
        }

        default:
            break;
    }

    return 1;
}

esp_err_t display_oled_init(void)
{
    if (oled_initialized)
    {
        return ESP_OK;
    }

    ESP_LOGI(
        TAG,
        "Initializing SH1106 OLED..."
    );

    i2c_master_bus_handle_t bus =
        bme680_get_i2c_bus();

    if (bus == NULL)
    {
        ESP_LOGE(
            TAG,
            "BME680 I2C bus is not available"
        );

        return ESP_ERR_INVALID_STATE;
    }


    i2c_device_config_t dev_cfg =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_I2C_ADDRESS,
        .scl_speed_hz = OLED_I2C_SPEED_HZ,
    };

    esp_err_t ret = i2c_master_bus_add_device(
        bus,
        &dev_cfg,
        &oled_device
    );

    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to add OLED to I2C bus: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }

    u8g2_Setup_sh1106_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_esp32_i2c,
        u8x8_gpio_and_delay_esp32
    );

    u8g2_SetI2CAddress(
        &u8g2,
        OLED_I2C_ADDRESS << 1
    );


    u8g2_InitDisplay(&u8g2);


    u8g2_SetPowerSave(
        &u8g2,
        0
    );

    oled_initialized = true;

    display_oled_clear();

    ESP_LOGI(
        TAG,
        "OLED initialized successfully"
    );

    return ESP_OK;
}

esp_err_t display_oled_on(void)
{
    if (!oled_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    u8g2_SetPowerSave(
        &u8g2,
        0
    );

    return ESP_OK;
}


esp_err_t display_oled_off(void)
{
    if (!oled_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    u8g2_SetPowerSave(
        &u8g2,
        1
    );

    return ESP_OK;
}


esp_err_t display_oled_clear(void)
{
    if (!oled_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    u8g2_ClearBuffer(&u8g2);

    u8g2_SendBuffer(&u8g2);

    return ESP_OK;
}


esp_err_t display_oled_show_mode(
    const char *mode)
{
    if (!oled_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (mode == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    u8g2_SetPowerSave(
        &u8g2,
        0
    );

    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFont(
        &u8g2,
        u8g2_font_7x14_tf
    );

    u8g2_DrawStr(
        &u8g2,
        0,
        14,
        "POWER MODE"
    );

    char mode_text[20];

    snprintf(
        mode_text,
        sizeof(mode_text),
        "MODE: %s",
        mode
    );

    u8g2_DrawStr(
        &u8g2,
        0,
        32,
        mode_text
    );

    u8g2_SetFont(
        &u8g2,
        u8g2_font_6x10_tf
    );

    u8g2_DrawStr(
        &u8g2,
        0,
        48,
        "ESP32 ENERGY TEST"
    );

    u8g2_DrawStr(
        &u8g2,
        0,
        62,
        "OLED: ON"
    );

    u8g2_SendBuffer(&u8g2);

    ESP_LOGI(TAG,"OLED displaying mode %s",mode);

    return ESP_OK;
}