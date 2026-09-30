#ifndef DISPLAY_OLED_H
#define DISPLAY_OLED_H

#include "esp_err.h"


esp_err_t display_oled_init(void);
esp_err_t display_oled_on(void);
esp_err_t display_oled_off(void);
esp_err_t display_oled_clear(void);
esp_err_t display_oled_show_mode(const char *mode);

#endif