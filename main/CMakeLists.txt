set(srcs
    button/button.c
    led/led.c
    main.c
    modes/mode_manager.c
    mqtt/mqtt.c
    mqtt/mqtt_topics.c
    mqtt/mqtt.h
    utils/types.h
    utils/utils.h
    wifi/wifi.c
    wifi/wifi.h
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
                 "button"
                 "led"
                 "modes"
                 "mqtt"
                 "utils"
                 "wifi"
    REQUIRES esp_driver_gpio
             esp_http_client
             esp_wifi
             nvs_flash
             mqtt
)
