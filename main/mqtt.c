#include "mqtt.h"

#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_err.h"

#include <stdbool.h>

static const char *TAG = "MQTT_MGR";


static esp_mqtt_client_handle_t s_mqtt_client = NULL;


static bool s_mqtt_connected = false;


static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    (void)handler_args;
    (void)event_base;


    esp_mqtt_event_handle_t event =
        event_data;


    switch ((esp_mqtt_event_id_t)event_id)
    {

        case MQTT_EVENT_CONNECTED:

            ESP_LOGI(
                TAG,
                "Đã kết nối tới HiveMQ Broker thành công."
            );

            s_mqtt_connected = true;

            break;

        case MQTT_EVENT_DISCONNECTED:

            ESP_LOGW(
                TAG,
                "Mất kết nối tới MQTT Broker."
            );

            s_mqtt_connected = false;

            break;

        case MQTT_EVENT_PUBLISHED:

            ESP_LOGD(
                TAG,
                "MQTT_EVENT_PUBLISHED, msg_id=%d",
                event->msg_id
            );

            break;

        case MQTT_EVENT_ERROR:

            ESP_LOGE(
                TAG,
                "Gặp lỗi MQTT Event Error."
            );

            break;

        default:

            break;
    }
}


void mqtt_manager_init(void)
{
    s_mqtt_connected = false;


    esp_mqtt_client_config_t mqtt_cfg =
    {
        .broker.address.uri =
            MQTT_BROKER_URI,
    };


    s_mqtt_client =
        esp_mqtt_client_init(
            &mqtt_cfg
        );


    if (s_mqtt_client == NULL)
    {
        ESP_LOGE(
            TAG,
            "Không thể khởi tạo thực thể MQTT Client."
        );

        return;
    }


    /*
     * Đăng ký MQTT event handler.
     */
    esp_err_t ret =
        esp_mqtt_client_register_event(
            s_mqtt_client,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler,
            NULL
        );


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Không thể đăng ký MQTT event handler: %s",
            esp_err_to_name(ret)
        );

        return;
    }

    ret =
        esp_mqtt_client_start(
            s_mqtt_client
        );


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Không thể start MQTT client: %s",
            esp_err_to_name(ret)
        );

        return;
    }


    ESP_LOGI(
        TAG,
        "MQTT client started."
    );
}


bool mqtt_manager_is_connected(void)
{
    return s_mqtt_connected;
}


bool mqtt_manager_publish(
    const char *topic,
    const char *payload,
    int qos)
{
    if (!s_mqtt_connected ||
        s_mqtt_client == NULL)
    {
        ESP_LOGE(
            TAG,
            "Không thể gửi dữ liệu. MQTT chưa sẵn sàng."
        );

        return false;
    }

    int msg_id =
        esp_mqtt_client_publish(
            s_mqtt_client,
            topic,
            payload,
            0,
            qos,
            0
        );


    if (msg_id < 0)
    {
        ESP_LOGE(
            TAG,
            "Lỗi khi đẩy tin nhắn lên Broker, msg_id = %d",
            msg_id
        );

        return false;
    }

    return true;
}