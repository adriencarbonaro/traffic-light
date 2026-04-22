#include "button.h"
#include "config.h"
#include "http_server.h"
#include "led.h"
#include "mode_manager.h"
#include "utils.h"
#include "wifi.h"

/* Prototypes *****************************************************************/
/* Global pointers ************************************************************/
/* Structs - Enums ************************************************************/
/* Static functions ***********************************************************/
void app_main(void)
{
    wifi_init();

    http_server_init();

    led_init();

    mode_manager_init();

    button_init();
}
