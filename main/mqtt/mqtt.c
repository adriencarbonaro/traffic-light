#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/projdefs.h"
#include "freertos/portmacro.h"
#include "mode_manager.h"
#include "mqtt.h"
#include "string.h"
#include "utils/types.h"
#include "utils.h"
#include "version.h"
#include "wifi.h"

typedef struct {
    const mqtt_config_t* config_table;
    uint16 config_table_size;
} mqtt_task_data_t;

/* Prototypes *****************************************************************/
static void send_version(void);

/* Structs - Enums ************************************************************/
/* Static const objects *******************************************************/
static const char* TAG = "mqtt";

/* Static objects *************************************************************/
static esp_mqtt_client_handle_t mqtt_client = NULL;
static mqtt_task_data_t task_data = {};

/* Static functions ***********************************************************/
static int is_topic(const char* config_topic,
                    uint16 config_topic_len,
                    const char* topic,
                    uint16 topic_len)
{
    ESP_LOGI(TAG, "comparing %s (%u) and %s (%u)", config_topic, config_topic_len, topic, topic_len);
    return (config_topic_len == topic_len) &&
           (memcmp(config_topic, topic, topic_len) == 0);
}

static void on_msg(const char* topic,
                   int topic_len,
                   const char* msg,
                   int msg_len)
{
    for (uint16 i = 0; i < task_data.config_table_size; i++)
    {
        mqtt_config_t item = task_data.config_table[i];
        if (is_topic(item.topic, strlen(item.topic), topic, topic_len))
        {
            item.handler(msg, msg_len);
            break;
        }
    }
}

static void mqtt_event_handler(void* event_handler_arg,
                               esp_event_base_t event_base,
                               int32 event_id,
                               void* event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
        {
            ESP_LOGI(TAG, "Connected to server");

            for (uint16 i = 0; i < task_data.config_table_size; i++)
            {
                mqtt_config_t config_table = task_data.config_table[i];
                ESP_LOGI(TAG, "Subscribing to topic %s", config_table.topic);
                esp_mqtt_client_subscribe(event->client,
                                          config_table.topic,
                                          0);
            }

            send_version();

            break;
        }

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from server");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Subscription success (msg_id=%d)", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "Unsubscription success (msg_id=%d)", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Published, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
        {
            ESP_LOGI(TAG, "Received message on topic %.*s: %.*s",
                     event->topic_len, event->topic,
                     event->data_len, event->data);

            on_msg(event->topic, event->topic_len, event->data, event->data_len);
            break;
        }

        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "Before connect...");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "Error");
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void publish(const char* topic, const char* msg)
{
    if (mqtt_client == NULL)
        return;

    esp_mqtt_client_publish(mqtt_client, topic, msg, 0, 1, 0);
}

static void send_version(void)
{
    publish(MQTT_TOPIC_PREFIX MQTT_TOPIC_VERSION, VERSION " - " BUILD_ID_SHORT);
}

/* Functions ******************************************************************/
void mqtt_init(void)
{
    memset(&task_data, 0, sizeof(mqtt_task_data_t));
    task_data.config_table = get_mqtt_config_table();
    task_data.config_table_size = get_mqtt_config_table_size();

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_URI,
        .session.keepalive = 60,
        .network.reconnect_timeout_ms = 5000,
    };

    mqtt_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(mqtt_client,
                                   ESP_EVENT_ANY_ID,
                                   mqtt_event_handler,
                                   mqtt_client);

    esp_mqtt_client_start(mqtt_client);
}
