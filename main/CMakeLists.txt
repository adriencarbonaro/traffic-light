set(srcs
    button/button.c
    led/led.c
    led/led_default_modes.c
    main.c
    modes/mode_manager.c
    modes/mode_decoder.c
    http/http_server.c
    utils/utils.h
    wifi/wifi.c
    wifi/wifi.h
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
                 "button"
                 "http"
                 "led"
                 "modes"
                 "utils"
                 "wifi"
    REQUIRES esp_driver_gpio
             esp_http_client
             esp_http_server
             esp_wifi
             nvs_flash
             mqtt
)

# Add build directory to resolve version.h
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${CMAKE_BINARY_DIR}/"
)
