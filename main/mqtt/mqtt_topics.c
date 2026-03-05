#include "esp_log.h"
#include "mqtt.h"
#include "types.h"
#include "config.h"
#include "utils.h"

/* Defines ********************************************************************/

#define TOPIC_MODE_PREFIX     "mode/"
#define TOPIC_MANUAL          "manual"

/* Prototypes *****************************************************************/

static void on_mode_set(const char* msg, uint16 msg_len);
static void on_mode_add(const char* msg, uint16 msg_len);
static void on_manual(const char* msg, uint16 msg_len);

/* Structs - Enums ************************************************************/

/* Static objects *************************************************************/

static const char *TAG = "mqtt_topics";

static const mqtt_config_t mqtt_config_table[] = {
    { MQTT_TOPIC_PREFIX TOPIC_MODE_PREFIX "set",    on_mode_set },
    { MQTT_TOPIC_PREFIX TOPIC_MODE_PREFIX "add",    on_mode_add },
    { MQTT_TOPIC_PREFIX TOPIC_MANUAL,               on_manual },
};

/* Static functions ***********************************************************/

static void on_mode_set(const char* msg, uint16 msg_len)
{
    ESP_LOGI(TAG, "%s: %.*s", __FUNCTION__, msg_len, msg);
}

static void on_mode_add(const char* msg, uint16 msg_len)
{
    ESP_LOGI(TAG, "%s: %.*s", __FUNCTION__, msg_len, msg);
}

static void on_manual(const char* msg, uint16 msg_len)
{
    ESP_LOGI(TAG, "%s: %.*s", __FUNCTION__, msg_len, msg);
}

/* Public functions ***********************************************************/
const mqtt_config_t* get_mqtt_config_table(void)
{
    return mqtt_config_table;
}

uint16 get_mqtt_config_table_size(void)
{
    return ARRAY_DIM(mqtt_config_table);
}
