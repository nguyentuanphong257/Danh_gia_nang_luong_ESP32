#ifndef BME680_REG_H
#define BME680_REG_H

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdint.h>

/* ============================================================
 * BME680 I2C address
 * ============================================================ */

#define BME680_I2C_ADDR 0x77

/* ============================================================
 * BME680 Chip ID
 * ============================================================ */

#define BME680_CHIP_ID 0x61

/* ============================================================
 * BME680 register addresses
 * ============================================================ */

#define BME680_REG_CHIP_ID 0xD0
#define BME680_REG_STATUS 0x1D

#define BME680_REG_PRESS_MSB 0x1F
#define BME680_REG_PRESS_LSB 0x20
#define BME680_REG_PRESS_XLSB 0x21

#define BME680_REG_TEMP_MSB 0x22
#define BME680_REG_TEMP_LSB 0x23
#define BME680_REG_TEMP_XLSB 0x24

#define BME680_REG_HUM_MSB 0x25
#define BME680_REG_HUM_LSB 0x26

#define BME680_REG_CONFIG 0x75
#define BME680_REG_CTRL_MEAS 0x74
#define BME680_REG_CTRL_HUM 0x72
#define BME680_REG_CTRL_GAS_1 0x71

#define BME680_STATUS_MEASURING (1 << 5)

typedef enum {
  BME680_MODE_SLEEP = 0,
  BME680_MODE_FORCED,
  BME680_MODE_NORMAL

} bme680_mode_t;

/* ============================================================
 * Raw measurement data
 * ============================================================ */

typedef struct {
  uint32_t pressure_raw;
  uint32_t temperature_raw;
  uint16_t humidity_raw;

} bme680_raw_data_t;

/* ============================================================
 * Initialization
 * ============================================================ */

/**
 * @brief Khởi tạo BME680 và I2C bus.
 *
 * @return
 * ESP_OK nếu khởi tạo thành công.
 * ESP_ERR_* nếu xảy ra lỗi.
 */
esp_err_t bme680_reg_init(void);

/* ============================================================
 * Operating mode
 * ============================================================ */

/**
 * @brief Đặt chế độ hoạt động cho BME680.
 *
 * @param mode
 * BME680_MODE_SLEEP
 * BME680_MODE_FORCED
 * BME680_MODE_NORMAL
 *
 * @return
 * ESP_OK nếu thành công.
 */
esp_err_t bme680_set_mode(bme680_mode_t mode);

/* ============================================================
 * Raw data
 * ============================================================ */

/**
 * @brief Đọc dữ liệu raw Pressure / Temperature / Humidity.
 *
 * @param data
 * Con trỏ tới cấu trúc lưu dữ liệu raw.
 *
 * @return
 * ESP_OK nếu đọc thành công.
 */
esp_err_t bme680_read_raw(bme680_raw_data_t *data);

/* ============================================================
 * Chip ID
 * ============================================================ */

/**
 * @brief Đọc Chip ID của BME680.
 *
 * @param chip_id
 * Con trỏ lưu Chip ID.
 *
 * @return
 * ESP_OK nếu đọc thành công.
 */
esp_err_t bme680_read_chip_id(uint8_t *chip_id);

/* ============================================================
 * I2C bus
 * ============================================================ */

/**
 * @brief Lấy handle của I2C master bus đang dùng bởi BME680.
 *
 * Mục đích:
 * Cho các thiết bị khác, ví dụ OLED, sử dụng chung I2C bus.
 *
 * @return
 * i2c_master_bus_handle_t nếu bus đã được khởi tạo.
 * NULL nếu chưa khởi tạo.
 */
i2c_master_bus_handle_t bme680_get_i2c_bus(void);

/* ============================================================
 * Periodic Forced measurement
 * ============================================================ */

/**
 * @brief Bắt đầu kích hoạt BME680 Forced measurement định kỳ.
 *
 * Mỗi chu kỳ:
 * 1. Đặt BME680 sang Forced mode.
 * 2. Chờ measurement hoàn thành.
 * 3. BME680 tự quay về Sleep.
 * 4. Chờ tới chu kỳ tiếp theo.
 *
 * Hàm này được thiết kế cho bài toán đo năng lượng.
 * Trong task định kỳ không đọc dữ liệu raw và không in log.
 *
 * @param period_ms
 * Chu kỳ kích hoạt, đơn vị ms.
 *
 * @return
 * ESP_OK nếu task được tạo thành công.
 */
esp_err_t bme680_start_periodic_forced(uint32_t period_ms);

/**
 * @brief Dừng Forced measurement định kỳ.
 *
 * Sau khi dừng, BME680 được đưa về Sleep mode.
 *
 * @return
 * ESP_OK nếu thành công.
 */
esp_err_t bme680_stop_periodic_forced(void);

#endif /* BME680_REG_H */