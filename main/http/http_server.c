#include "mode_manager.h"
#include "version.h"
#include <esp_http_server.h>
#include <esp_log.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Prototypes *****************************************************************/

static esp_err_t on_version(httpd_req_t* req);
static esp_err_t on_version_json(httpd_req_t* req);
static esp_err_t on_get_modes(httpd_req_t* req);
static esp_err_t on_get_mode(httpd_req_t* req);
static esp_err_t on_command(httpd_req_t* req);

/* Defines ********************************************************************/

#define REQ(x, method) \
    static const httpd_uri_t x##_uri = {"/" #x, method, on_##x, NULL};

#define GET(x) REQ(x, HTTP_GET)
#define POST(x) REQ(x, HTTP_POST)

#define REGISTER(x) httpd_register_uri_handler(server, &x##_uri);

/* Global objects *************************************************************/

static const char* TAG = "http_server";

GET(version)
GET(version_json)
GET(get_modes)
GET(get_mode)
POST(command)

/* Static functions ***********************************************************/

/* Appends a length-prefixed string: [ len : uint16 LE ] [ bytes ]. */
static size_t write_lp_str(uint8_t* dst, const char* s)
{
    uint16_t n = (uint16_t)strlen(s);
    dst[0] = (uint8_t)(n & 0xff);
    dst[1] = (uint8_t)((n >> 8) & 0xff);
    memcpy(dst + 2, s, n);
    return 2 + n;
}

static esp_err_t on_version(httpd_req_t* req)
{
    ESP_LOGI(TAG, "%s", __func__);

    /* Just the version string (DESCRIBE), length-prefixed. The full identity
       is available as JSON from /version_json. */
    uint8_t buf[256];
    size_t off = 0;
    off += write_lp_str(&buf[off], DESCRIBE);

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_send(req, (const char*)buf, off);

    return ESP_OK;
}

static esp_err_t on_version_json(httpd_req_t* req)
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
             DESCRIBE,
             __DATE__,
             __TIME__);

    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static esp_err_t on_get_modes(httpd_req_t* req)
{
    ESP_LOGI(TAG, "%s", __func__);

    uint8_t* buf = NULL;
    size_t len = 0;
    if (mode_manager_get(&buf, &len) != ESP_OK || buf == NULL)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "modes");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_send(req, (const char*)buf, len);

    free(buf);
    return ESP_OK;
}

static esp_err_t on_get_mode(httpd_req_t* req)
{
    ESP_LOGI(TAG, "%s", __func__);

    uint8_t* buf = NULL;
    size_t len = 0;
    if (mode_manager_get_active(&buf, &len) != ESP_OK || buf == NULL)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "mode");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_send(req, (const char*)buf, len);

    free(buf);
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

        mode_manager_event((void*)buf, ret);
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
    REGISTER(version_json)
    REGISTER(get_modes)
    REGISTER(get_mode)
    REGISTER(command)
}
