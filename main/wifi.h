#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include "esp_err.h"

esp_err_t wifi_init(void);

esp_err_t wifi_on(void);
esp_err_t wifi_off(void);

bool wifi_is_connected(void);
bool wifi_is_on(void);

#endif