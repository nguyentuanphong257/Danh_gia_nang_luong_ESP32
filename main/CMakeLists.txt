idf_component_register(
    SRCS
        "main.c"
        "power_mode.c"
        "wifi.c"
        "mqtt.c"
        "bme680_reg.c"
        "display_oled.c"
        "bluetooth.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        u8g2
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        mqtt
        driver
        bt
)