#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Khởi tạo Bluetooth Low Energy và bắt đầu advertising.
 *
 * Hàm này:
 * 1. Khởi tạo NimBLE stack.
 * 2. Cấu hình GAP.
 * 3. Đặt tên thiết bị BLE.
 * 4. Bắt đầu BLE advertising.
 *
 * @return
 * ESP_OK nếu khởi tạo thành công.
 * Mã lỗi ESP-IDF nếu có lỗi.
 */
esp_err_t bluetooth_on(void);

/**
 * @brief Dừng BLE advertising.
 *
 * Trong kiến trúc hiện tại, Bluetooth được lựa chọn một lần
 * khi khởi động mode. Vì vậy nếu mode yêu cầu BLUETOOTH_OFF,
 * bluetooth_on() sẽ không được gọi và Bluetooth controller
 * không được khởi tạo.
 *
 * Nếu Bluetooth đã được khởi tạo, hàm này dừng advertising.
 *
 * @return
 * ESP_OK nếu thành công.
 * Mã lỗi ESP-IDF nếu có lỗi.
 */
esp_err_t bluetooth_off(void);

/**
 * @brief Kiểm tra Bluetooth đã được khởi tạo hay chưa.
 *
 * @return
 * true  - Bluetooth đã được khởi tạo.
 * false - Bluetooth chưa được khởi tạo.
 */
bool bluetooth_is_initialized(void);

#endif