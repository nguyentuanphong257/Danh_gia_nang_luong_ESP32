#include <stdio.h>

#include "wifi.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_SSID           "Son Hai"
#define WIFI_PASS           "13251971"
#define WIFI_MAXIMUM_RETRY  5

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static const char *TAG = "WIFI";

static bool wifi_initialized = false;
static bool wifi_started = false;

static int wifi_retry_count = 0;

static EventGroupHandle_t wifi_event_group = NULL;


static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (wifi_retry_count < WIFI_MAXIMUM_RETRY)
        {
            wifi_retry_count++;

            ESP_LOGW(TAG,
                     "Wi-Fi disconnected. Retry %d/%d",
                     wifi_retry_count,
                     WIFI_MAXIMUM_RETRY);

            esp_wifi_connect();
        }
        else
        {
            xEventGroupSetBits(
                wifi_event_group,
                WIFI_FAIL_BIT
            );
        }
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG,
                 "Wi-Fi connected. IP: " IPSTR,
                 IP2STR(&event->ip_info.ip));

        wifi_retry_count = 0;

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }
}


esp_err_t wifi_init(void)
{
    if (wifi_initialized)
    {
        return ESP_OK;
    }

    esp_err_t ret;

    wifi_event_group = xEventGroupCreate();

    if (wifi_event_group == NULL)
    {
        ESP_LOGE(TAG,
                 "Cannot create Wi-Fi event group");

        return ESP_ERR_NO_MEM;
    }

    ret = esp_netif_init();

    if (ret != ESP_OK &&
        ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG,
                 "esp_netif_init failed: %s",
                 esp_err_to_name(ret));

        return ret;
    }

    ret = esp_event_loop_create_default();

    if (ret != ESP_OK &&
        ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG,
                 "esp_event_loop_create_default failed: %s",
                 esp_err_to_name(ret));

        return ret;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ret = esp_wifi_init(&cfg);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "esp_wifi_init failed: %s",
                 esp_err_to_name(ret));

        return ret;
    }

    ret = esp_event_handler_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL
    );

    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = esp_event_handler_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL
    );

    if (ret != ESP_OK)
    {
        return ret;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ret = esp_wifi_set_mode(WIFI_MODE_STA);

    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = esp_wifi_set_config(
        WIFI_IF_STA,
        &wifi_config
    );

    if (ret != ESP_OK)
    {
        return ret;
    }

    wifi_initialized = true;

    ESP_LOGI(TAG,
             "Wi-Fi initialized successfully");

    return ESP_OK;
}


esp_err_t wifi_on(void)
{
    esp_err_t ret;

    if (!wifi_initialized)
    {
        ret = wifi_init();

        if (ret != ESP_OK)
        {
            return ret;
        }
    }

    if (wifi_started)
    {
        ESP_LOGI(TAG,
                 "Wi-Fi is already started");

        return ESP_OK;
    }

    wifi_retry_count = 0;

    ret = esp_wifi_start();

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "esp_wifi_start failed: %s",
                 esp_err_to_name(ret));

        return ret;
    }

    wifi_started = true;

    ESP_LOGI(TAG,
             "Wi-Fi started. Connecting to AP...");

    return ESP_OK;
}


esp_err_t wifi_off(void)
{
    if (!wifi_initialized)
    {
        return ESP_OK;
    }

    if (!wifi_started)
    {
        return ESP_OK;
    }

    esp_err_t ret = esp_wifi_stop();

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "esp_wifi_stop failed: %s",
                 esp_err_to_name(ret));

        return ret;
    }

    wifi_started = false;

    xEventGroupClearBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT
    );

    ESP_LOGI(TAG,
             "Wi-Fi turned off successfully");

    return ESP_OK;
}


bool wifi_is_connected(void)
{
    if (wifi_event_group == NULL)
    {
        return false;
    }

    EventBits_t bits =
        xEventGroupGetBits(wifi_event_group);

    return (bits & WIFI_CONNECTED_BIT) != 0;
}


bool wifi_is_on(void)
{
    return wifi_started;
}