#include "mode_manager.h"
#include "version.h"
#include <esp_http_server.h>
#include <esp_log.h>

/* Prototypes *****************************************************************/

static esp_err_t on_version(httpd_req_t* req);
static esp_err_t on_command(httpd_req_t* req);

/* Defines ********************************************************************/

#define REQ(x, method) \
    static const httpd_uri_t x = {"/" #x, method, on_##x, NULL};

#define GET(x) static const httpd_uri_t x = {"/" #x, HTTP_GET, on_##x, NULL};
#define POST(x) static const httpd_uri_t x = {"/" #x, HTTP_POST, on_##x, NULL};

#define REGISTER(x) httpd_register_uri_handler(server, &x);

/* Global objects *************************************************************/

static const char* TAG = "http_server";

GET(version)
POST(command)

/* Static functions ***********************************************************/

static esp_err_t on_version(httpd_req_t* req)
{
    ESP_LOGI(TAG, "%s", __func__);

    size_t buf_len = 256;
    char buf[buf_len];

    snprintf(buf,
             buf_len,
             "{"
             "\"firmware\":\"traffic-light\","
             "\"version\":\"%s\","
             "\"build_date\":\"%s\","
             "\"build_time\":\"%s\","
             "\"chip\":\"ESP32-C3\""
             "}",
             BUILD_ID_SHORT,
             __DATE__,
             __TIME__);

    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static esp_err_t on_command(httpd_req_t* req)
{
    ESP_LOGI(TAG, "%s", __func__);

    size_t buf_len = 256;
    char buf[buf_len];
    int ret, remaining = req->content_len;

    while (remaining > 0)
    {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                continue;
            }
            return ESP_FAIL;
        }

        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        ESP_LOG_BUFFER_HEX(TAG, buf, ret);

        mode_manager_event(MODE_EVENT_SET, (void*)buf, ret);
    }

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* Public functions ***********************************************************/

void http_server_init(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGI(TAG, "Error starting server!");
        return;
    }

    ESP_LOGI(TAG, "Registering URI handlers");

    REGISTER(version)
    REGISTER(command)
}
