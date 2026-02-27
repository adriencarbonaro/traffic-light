#include "wifi.h"

#include "config.h"

#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "types.h"

static const char *TAG = "wifi";

typedef struct {
    wifi_callback_t on_connected_callback;
    wifi_callback_t on_disconnected_callback;
} wifi_task_data_t;

static wifi_task_data_t wifi_task = {0};

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32 event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        if (wifi_task.on_connected_callback)
            wifi_task.on_connected_callback();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t *d = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGW(TAG, "Disconnected, reason: %d", d->reason);
        if (wifi_task.on_disconnected_callback)
            wifi_task.on_disconnected_callback();
    }
}

void wifi_init(wifi_callback_t on_connected, wifi_callback_t on_disconnected)
{
    memset(&wifi_task, 0, sizeof(wifi_task));
    wifi_task.on_connected_callback    = on_connected;
    wifi_task.on_disconnected_callback = on_disconnected;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_LOGI(TAG, "Wi-Fi config: ssid=%s password:%s",
             wifi_config.sta.ssid, wifi_config.sta.password);

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Wi-Fi initialized and connecting...");
}
