#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_http_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/projdefs.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"
#include <string.h>

#include "button.h"
#include "config.h"
#include "led.h"
#include "mode_manager.h"
#include "mqtt.h"
#include "utils.h"
#include "utils/types.h"
#include "wifi.h"

/* Prototypes *****************************************************************/
/* Global pointers ************************************************************/
/* Structs - Enums ************************************************************/
/* Static functions ***********************************************************/
void app_main(void)
{
    wifi_init(mqtt_init, NULL);

    mode_manager_init();

    led_init();

    button_init();
}
