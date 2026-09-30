#include "bme680_reg.h"

#include "esp_log.h"

#include "app_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include <stdbool.h>
#include <stddef.h>

#define BME680_PERIODIC_TASK_STACK_SIZE 3072
#define BME680_PERIODIC_TASK_PRIORITY 5

static const char *TAG = "BME680";

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t bme680_handle = NULL;

static bool initialized = false;

static TaskHandle_t periodic_task_handle = NULL;
static volatile bool periodic_task_running = false;
static uint32_t periodic_period_ms = 1000;

static esp_err_t bme680_read_register(uint8_t reg, uint8_t *data,
                                      size_t length) {
  if (bme680_handle == NULL) {
    return ESP_ERR_INVALID_STATE;
  }

  return i2c_master_transmit_receive(bme680_handle, &reg, 1, data, length, 100);
}

static esp_err_t bme680_write_register(uint8_t reg, uint8_t value) {
  if (bme680_handle == NULL) {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t buffer[2];

  buffer[0] = reg;
  buffer[1] = value;

  return i2c_master_transmit(bme680_handle, buffer, 2, 100);
}

static esp_err_t bme680_configure_measurement(void) {
  esp_err_t ret;

  ret = bme680_write_register(BME680_REG_CTRL_HUM, 0x01);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure humidity: %s", esp_err_to_name(ret));
    return ret;
  }

  /* --------------------------------------------------------
   * CTRL_MEAS = 0x24
   *
   * 001 001 00
   *
   * osrs_t = 001 -> Temperature 1x
   * osrs_p = 001 -> Pressure 1x
   * mode   = 00  -> Sleep
   * -------------------------------------------------------- */

  ret = bme680_write_register(BME680_REG_CTRL_MEAS, 0x24);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure measurement: %s", esp_err_to_name(ret));
    return ret;
  }

  ret = bme680_write_register(BME680_REG_CTRL_GAS_1, 0x00);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to disable gas measurement: %s",
             esp_err_to_name(ret));
    return ret;
  }

  /* --------------------------------------------------------
   * CONFIG = 0x00
   * -------------------------------------------------------- */
  ret = bme680_write_register(BME680_REG_CONFIG, 0x00);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure CONFIG register: %s",
             esp_err_to_name(ret));
    return ret;
  }
  return ESP_OK;
}

/* ============================================================
 * Wait until measurement is finished
 * ============================================================ */

/**
 * @brief Chờ BME680 hoàn thành measurement.
 *
 * STATUS bit 5:
 *
 * 1 -> đang đo
 * 0 -> đã đo xong
 *
 * Hàm kiểm tra tối đa 200 lần,
 * mỗi lần cách nhau 1 ms.
 *
 * @return
 * ESP_OK nếu measurement hoàn thành.
 * ESP_ERR_TIMEOUT nếu quá 200 ms.
 */
static esp_err_t bme680_wait_measurement_done(void) {
  uint8_t status = 0;

  for (int i = 0; i < 200; i++) {
    esp_err_t ret = bme680_read_register(BME680_REG_STATUS, &status, 1);

    if (ret != ESP_OK) {
      return ret;
    }

    /*
     * Bit 5 = measuring.
     *
     * Nếu bit này = 0:
     * measurement đã hoàn thành.
     */

    if ((status & BME680_STATUS_MEASURING) == 0) {
      return ESP_OK;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  ESP_LOGE(TAG, "BME680 measurement timeout");

  return ESP_ERR_TIMEOUT;
}

static esp_err_t bme680_trigger_forced_quiet(void) {
  if (!initialized) {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t reg_value = 0;

  esp_err_t ret = bme680_read_register(BME680_REG_CTRL_MEAS, &reg_value, 1);

  if (ret != ESP_OK) {
    return ret;
  }

  reg_value &= ~0x03;

  reg_value |= 0x01;

  /* --------------------------------------------------------
   * Ghi CTRL_MEAS
   * -------------------------------------------------------- */

  ret = bme680_write_register(BME680_REG_CTRL_MEAS, reg_value);

  if (ret != ESP_OK) {
    return ret;
  }

  /* --------------------------------------------------------
   * Chờ measurement hoàn thành
   * -------------------------------------------------------- */

  return bme680_wait_measurement_done();
}

/* ============================================================
 * Periodic Forced measurement task
 * ============================================================ */

/**
 * @brief FreeRTOS task thực hiện Forced measurement định kỳ.
 *
 * Mỗi chu kỳ:
 *
 * 1. Trigger Forced measurement.
 * 2. Chờ measurement hoàn thành.
 * 3. BME680 tự quay về Sleep.
 * 4. Chờ tới chu kỳ tiếp theo.
 *
 * Không đọc raw data.
 * Không in log trong từng chu kỳ.
 */
static void bme680_periodic_forced_task(void *pvParameters) {
  (void)pvParameters;

  TickType_t last_wake_time = xTaskGetTickCount();

  while (periodic_task_running) {
    esp_err_t ret = bme680_trigger_forced_quiet();

    if (ret != ESP_OK) {
      /*
       * Không dùng ESP_LOGE ở mỗi chu kỳ
       * vì đây là task phục vụ đo năng lượng.
       *
       * Nếu xảy ra lỗi, task sẽ tiếp tục thử
       * ở chu kỳ kế tiếp.
       */
    }

    /*
     * Chờ tới chu kỳ tiếp theo.
     *
     * vTaskDelayUntil giúp chu kỳ ổn định hơn
     * so với vTaskDelay().
     */

    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(periodic_period_ms));
  }

  periodic_task_handle = NULL;

  vTaskDelete(NULL);
}

/* ============================================================
 * Initialize BME680
 * ============================================================ */

esp_err_t bme680_reg_init(void) {
  if (initialized) {
    return ESP_OK;
  }

  esp_err_t ret;

  /* --------------------------------------------------------
   * Create I2C master bus
   * -------------------------------------------------------- */

  i2c_master_bus_config_t bus_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_NUM_0,
      .sda_io_num = I2C_SDA_PIN,
      .scl_io_num = I2C_SCL_PIN,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };

  ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));

    return ret;
  }

  /* --------------------------------------------------------
   * Add BME680 device to I2C bus
   * -------------------------------------------------------- */

  i2c_device_config_t device_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = BME680_I2C_ADDR,
      .scl_speed_hz = I2C_FREQUENCY_HZ,
  };

  ret =
      i2c_master_bus_add_device(i2c_bus_handle, &device_config, &bme680_handle);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add BME680: %s", esp_err_to_name(ret));

    return ret;
  }

  /* --------------------------------------------------------
   * Read Chip ID
   * -------------------------------------------------------- */

  uint8_t chip_id = 0;

  ret = bme680_read_register(BME680_REG_CHIP_ID, &chip_id, 1);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read BME680 chip ID: %s", esp_err_to_name(ret));

    return ret;
  }

  ESP_LOGI(TAG, "BME680 Chip ID = 0x%02X", chip_id);

  if (chip_id != BME680_CHIP_ID) {
    ESP_LOGE(TAG, "Invalid BME680 Chip ID!");

    return ESP_ERR_NOT_FOUND;
  }

  /* --------------------------------------------------------
   * Configure measurement registers
   * -------------------------------------------------------- */

  ret = bme680_configure_measurement();

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure BME680 measurement");

    return ret;
  }

  initialized = true;

  ESP_LOGI(TAG, "BME680 initialized successfully");

  return ESP_OK;
}

/* ============================================================
 * Set BME680 operating mode
 * ============================================================ */

esp_err_t bme680_set_mode(bme680_mode_t mode) {
  if (!initialized) {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t reg_value;

  /*
   * Đọc CTRL_MEAS hiện tại.
   */

  esp_err_t ret = bme680_read_register(BME680_REG_CTRL_MEAS, &reg_value, 1);

  if (ret != ESP_OK) {
    return ret;
  }

  ESP_LOGI(TAG, "CTRL_MEAS before = 0x%02X", reg_value);

  /*
   * Xóa 2 bit mode[1:0].
   *
   * Các bit khác:
   * - osrs_t
   * - osrs_p
   *
   * được giữ nguyên.
   */

  reg_value &= ~0x03;

  /* --------------------------------------------------------
   * Select operating mode
   * -------------------------------------------------------- */

  switch (mode) {
  case BME680_MODE_SLEEP:

    /*
     * mode = 00
     */

    reg_value |= 0x00;

    break;

  case BME680_MODE_FORCED:

    /*
     * mode = 01
     */

    reg_value |= 0x01;

    break;

  case BME680_MODE_NORMAL:

    /*
     * mode = 10
     */

    reg_value |= 0x02;

    break;

  default:

    return ESP_ERR_INVALID_ARG;
  }

  /* --------------------------------------------------------
   * Write CTRL_MEAS
   * -------------------------------------------------------- */

  ret = bme680_write_register(BME680_REG_CTRL_MEAS, reg_value);

  if (ret != ESP_OK) {
    return ret;
  }

  /*
   * Đọc lại CTRL_MEAS để xác nhận
   * thanh ghi thực sự đã được ghi.
   */

  uint8_t verify_value = 0;

  ret = bme680_read_register(BME680_REG_CTRL_MEAS, &verify_value, 1);

  if (ret != ESP_OK) {
    return ret;
  }

  ESP_LOGI(TAG, "CTRL_MEAS after  = 0x%02X", verify_value);

  ESP_LOGI(TAG, "BME680 mode changed to %d", mode);

  return ESP_OK;
}

/* ============================================================
 * Read raw sensor data
 * ============================================================ */

esp_err_t bme680_read_raw(bme680_raw_data_t *data) {
  if (data == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  if (!initialized) {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t buffer[8];

  /*
   * Đọc liên tiếp 8 byte:
   *
   * 0x1F PRESS_MSB
   * 0x20 PRESS_LSB
   * 0x21 PRESS_XLSB
   *
   * 0x22 TEMP_MSB
   * 0x23 TEMP_LSB
   * 0x24 TEMP_XLSB
   *
   * 0x25 HUM_MSB
   * 0x26 HUM_LSB
   */

  esp_err_t ret = bme680_read_register(BME680_REG_PRESS_MSB, buffer, 8);

  if (ret != ESP_OK) {
    return ret;
  }

  /* --------------------------------------------------------
   * Pressure raw
   * -------------------------------------------------------- */

  data->pressure_raw = ((uint32_t)buffer[0] << 12) |
                       ((uint32_t)buffer[1] << 4) | ((uint32_t)buffer[2] >> 4);

  /* --------------------------------------------------------
   * Temperature raw
   * -------------------------------------------------------- */

  data->temperature_raw = ((uint32_t)buffer[3] << 12) |
                          ((uint32_t)buffer[4] << 4) |
                          ((uint32_t)buffer[5] >> 4);

  /* --------------------------------------------------------
   * Humidity raw
   * -------------------------------------------------------- */

  data->humidity_raw = ((uint16_t)buffer[6] << 8) | buffer[7];

  return ESP_OK;
}

/* ============================================================
 * Read Chip ID
 * ============================================================ */

esp_err_t bme680_read_chip_id(uint8_t *chip_id) {
  if (chip_id == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  if (!initialized) {
    return ESP_ERR_INVALID_STATE;
  }

  return bme680_read_register(BME680_REG_CHIP_ID, chip_id, 1);
}

/* ============================================================
 * Get I2C bus handle
 * ============================================================ */

i2c_master_bus_handle_t bme680_get_i2c_bus(void) { return i2c_bus_handle; }

/* ============================================================
 * Start periodic Forced measurement
 * ============================================================ */

esp_err_t bme680_start_periodic_forced(uint32_t period_ms) {
  if (!initialized) {
    return ESP_ERR_INVALID_STATE;
  }

  if (period_ms < 100) {
    return ESP_ERR_INVALID_ARG;
  }

  /*
   * Nếu task đã chạy thì không tạo task thứ hai.
   */

  if (periodic_task_handle != NULL) {
    return ESP_OK;
  }

  periodic_period_ms = period_ms;

  periodic_task_running = true;

  BaseType_t task_ret =
      xTaskCreate(bme680_periodic_forced_task, "bme680_periodic",
                  BME680_PERIODIC_TASK_STACK_SIZE, NULL,
                  BME680_PERIODIC_TASK_PRIORITY, &periodic_task_handle);

  if (task_ret != pdPASS) {
    periodic_task_running = false;
    periodic_task_handle = NULL;

    ESP_LOGE(TAG, "Failed to create periodic measurement task");

    return ESP_ERR_NO_MEM;
  }

  ESP_LOGI(TAG, "Periodic Forced measurement started: period=%lu ms",
           (unsigned long)period_ms);

  return ESP_OK;
}

/* ============================================================
 * Stop periodic Forced measurement
 * ============================================================ */

esp_err_t bme680_stop_periodic_forced(void) {
  /*
   * Nếu task không chạy thì chỉ cần đưa sensor về Sleep.
   */

  if (periodic_task_handle == NULL) {
    if (initialized) {
      return bme680_set_mode(BME680_MODE_SLEEP);
    }

    return ESP_OK;
  }

  /*
   * Báo cho task dừng.
   */

  periodic_task_running = false;

  /*
   * Chờ task tự kết thúc.
   *
   * Tối đa khoảng 100 ms.
   */

  for (int i = 0; i < 100; i++) {
    if (periodic_task_handle == NULL) {
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }

  /*
   * Đưa BME680 về Sleep.
   */

  if (initialized) {
    return bme680_set_mode(BME680_MODE_SLEEP);
  }

  return ESP_OK;
}