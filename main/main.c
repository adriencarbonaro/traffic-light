#include "driver/gpio.h"
#include "esp_chip_info.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <string.h>

#include "button.h"
#include "config.h"
#include "http_server.h"
#include "led.h"
#include "mode_manager.h"
#include "utils.h"
#include "utils/types.h"
#include "wifi.h"

/* Prototypes *****************************************************************/
/* Global pointers ************************************************************/
/* Structs - Enums ************************************************************/
/* Static functions ***********************************************************/
void app_main(void)
{
    wifi_init();

    http_server_init();

    mode_manager_init();

    led_init();

    button_init();
}
