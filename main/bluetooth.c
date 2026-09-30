#include "bluetooth.h"

#include <string.h>

#include "esp_log.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/ble_gap.h"

#include "services/gap/ble_svc_gap.h"

static const char *TAG = "BLUETOOTH";

#define BLE_DEVICE_NAME "ESP32-ENERGY"

static bool bluetooth_initialized = false;

/*
 * Địa chỉ BLE được sử dụng để advertising.
 */
static uint8_t own_addr_type;


/**
 * @brief Callback được gọi khi NimBLE host reset.
 *
 * @param reason Mã nguyên nhân reset của BLE host.
 */
static void bluetooth_on_reset(int reason)
{
    ESP_LOGW(
        TAG,
        "NimBLE host reset, reason=%d",
        reason
    );
}


/**
 * @brief Callback được gọi khi NimBLE host đã đồng bộ với controller.
 *
 * Đây là thời điểm thích hợp để xác định địa chỉ BLE
 * và bắt đầu advertising.
 */
static void bluetooth_on_sync(void)
{
    int rc;

    /*
     * Xác định loại địa chỉ BLE mà thiết bị sẽ sử dụng.
     *
     * ble_hs_id_infer_auto():
     * - Tự động chọn loại địa chỉ phù hợp.
     * - Kết quả được lưu vào own_addr_type.
     */
    rc = ble_hs_id_infer_auto(
        0,
        &own_addr_type
    );

    if (rc != 0)
    {
        ESP_LOGE(
            TAG,
            "Cannot determine BLE address type, rc=%d",
            rc
        );

        return;
    }

    ESP_LOGI(
        TAG,
        "BLE host synchronized"
    );


    /* ========================================================
     * Cấu hình BLE advertising data
     * ======================================================== */

    struct ble_hs_adv_fields fields;

    memset(
        &fields,
        0,
        sizeof(fields)
    );

    /*
     * Đưa tên thiết bị vào advertising packet.
     *
     * fields.name:
     * Con trỏ tới chuỗi tên thiết bị.
     *
     * fields.name_len:
     * Độ dài của chuỗi tên.
     *
     * fields.name_is_complete:
     * = 1 nghĩa là đây là tên đầy đủ của thiết bị.
     */
    fields.name = (uint8_t *)BLE_DEVICE_NAME;

    fields.name_len = strlen(
        BLE_DEVICE_NAME
    );

    fields.name_is_complete = 1;


    /*
     * ble_gap_adv_set_fields():
     *
     * Thiết lập advertising payload cho BLE.
     *
     * Trả về:
     * 0  -> thành công.
     * !=0 -> mã lỗi NimBLE.
     */
    rc = ble_gap_adv_set_fields(
        &fields
    );

    if (rc != 0)
    {
        ESP_LOGE(
            TAG,
            "Failed to set BLE advertising fields, rc=%d",
            rc
        );

        return;
    }


    ESP_LOGI(
        TAG,
        "BLE advertising data configured: %s",
        BLE_DEVICE_NAME
    );


    /* ========================================================
     * Cấu hình advertising parameters
     * ======================================================== */

    struct ble_gap_adv_params adv_params;

    memset(
        &adv_params,
        0,
        sizeof(adv_params)
    );

    /*
     * BLE_GAP_CONN_MODE_UND:
     *
     * Thiết bị advertising ở chế độ connectable,
     * không hướng tới một thiết bị cụ thể.
     */
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;


    /*
     * BLE_GAP_DISC_MODE_GEN:
     *
     * Cho phép thiết bị ở chế độ discoverable thông thường.
     */
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;


    /* ========================================================
     * Bắt đầu BLE advertising
     * ======================================================== */

    /*
     * ble_gap_adv_start():
     *
     * Bắt đầu phát advertising packet.
     *
     * own_addr_type:
     * Loại địa chỉ BLE đã được xác định ở trên.
     *
     * NULL:
     * Không giới hạn peer cụ thể.
     *
     * BLE_HS_FOREVER:
     * Advertising liên tục cho tới khi bị dừng.
     *
     * adv_params:
     * Các tham số advertising.
     *
     * NULL:
     * Không sử dụng callback kết thúc advertising.
     *
     * NULL:
     * Không truyền context riêng.
     */
    rc = ble_gap_adv_start(
        own_addr_type,
        NULL,
        BLE_HS_FOREVER,
        &adv_params,
        NULL,
        NULL
    );

    if (rc != 0)
    {
        ESP_LOGE(
            TAG,
            "Failed to start BLE advertising, rc=%d",
            rc
        );

        return;
    }

    ESP_LOGI(
        TAG,
        "BLE advertising started: %s",
        BLE_DEVICE_NAME
    );
}


/**
 * @brief BLE host task.
 *
 * NimBLE sẽ chạy trong task FreeRTOS riêng.
 *
 * @param param Không sử dụng.
 */
static void bluetooth_host_task(void *param)
{
    (void)param;

    ESP_LOGI(
        TAG,
        "BLE host task started"
    );

    /*
     * nimble_port_run():
     *
     * Chạy event loop của NimBLE.
     *
     * Hàm này tiếp tục chạy trong task
     * cho tới khi NimBLE được dừng.
     */
    nimble_port_run();


    /*
     * Khi NimBLE được stop,
     * hàm trên mới kết thúc.
     */
    nimble_port_freertos_deinit();

    ESP_LOGI(
        TAG,
        "BLE host task stopped"
    );
}


/**
 * @brief Khởi tạo Bluetooth LE.
 *
 * @return ESP_OK nếu thành công.
 */
esp_err_t bluetooth_on(void)
{
    int rc;
    esp_err_t ret;


    /*
     * Nếu Bluetooth đã được khởi tạo
     * thì không khởi tạo lại.
     */
    if (bluetooth_initialized)
    {
        ESP_LOGI(
            TAG,
            "Bluetooth is already initialized"
        );

        return ESP_OK;
    }


    ESP_LOGI(
        TAG,
        "Initializing Bluetooth LE..."
    );


    /*
     * nimble_port_init():
     *
     * Khởi tạo NimBLE host/controller
     * và thiết lập transport cần thiết.
     */
    ret = nimble_port_init();

    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "nimble_port_init() failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    /*
     * Callback khi BLE host reset.
     */
    ble_hs_cfg.reset_cb =
        bluetooth_on_reset;


    /*
     * Callback khi BLE host synchronized.
     *
     * Sau khi stack sẵn sàng,
     * bluetooth_on_sync() sẽ được gọi.
     */
    ble_hs_cfg.sync_cb =
        bluetooth_on_sync;


    /*
     * Khởi tạo GAP service.
     */
    ble_svc_gap_init();


    /*
     * Đặt tên thiết bị BLE.
     */
    rc = ble_svc_gap_device_name_set(
        BLE_DEVICE_NAME
    );

    if (rc != 0)
    {
        ESP_LOGE(
            TAG,
            "Failed to set BLE device name, rc=%d",
            rc
        );

        return ESP_FAIL;
    }


    /*
     * Khởi động FreeRTOS task cho NimBLE host.
     *
     * Sau khi stack synchronized,
     * bluetooth_on_sync() sẽ được gọi.
     */
    nimble_port_freertos_init(
        bluetooth_host_task
    );


    bluetooth_initialized = true;


    ESP_LOGI(
        TAG,
        "Bluetooth LE initialized successfully"
    );


    return ESP_OK;
}


/**
 * @brief Tắt Bluetooth advertising.
 *
 * @return ESP_OK nếu thành công.
 */
esp_err_t bluetooth_off(void)
{
    int rc;


    /*
     * Nếu Bluetooth chưa từng được khởi tạo,
     * không cần thực hiện gì.
     */
    if (!bluetooth_initialized)
    {
        ESP_LOGI(
            TAG,
            "Bluetooth is already OFF"
        );

        return ESP_OK;
    }


    /*
     * Dừng BLE advertising.
     */
    rc = ble_gap_adv_stop();

    if (rc != 0)
    {
        ESP_LOGW(
            TAG,
            "BLE advertising stop returned rc=%d",
            rc
        );
    }
    else
    {
        ESP_LOGI(
            TAG,
            "BLE advertising stopped"
        );
    }


    /*
     * Trong thiết kế thí nghiệm hiện tại,
     * mode được chọn một lần khi ESP32 khởi động.
     *
     * Vì vậy ta không deinit toàn bộ NimBLE controller
     * ở đây để tránh việc khởi tạo/deinit stack nhiều lần
     * trong quá trình thử nghiệm.
     */

    return ESP_OK;
}


/**
 * @brief Kiểm tra trạng thái khởi tạo Bluetooth.
 *
 * @return true nếu đã khởi tạo, false nếu chưa.
 */
bool bluetooth_is_initialized(void)
{
    return bluetooth_initialized;
}