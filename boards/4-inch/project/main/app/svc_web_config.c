#include "svc_web_config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_check.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "svc_rtc.h"

#define WEB_CONFIG_SSID       "NI-Badge-4"
#define WEB_CONFIG_PASSWORD   "ni50badge"
#define WEB_CONFIG_URL        "http://192.168.4.1"
#define WEB_CONFIG_MAX_CONN   4

static const char *TAG = "svc_webcfg";

static svc_web_config_state_t s_state = SVC_WEB_CONFIG_STATE_STOPPED;
static bool s_stack_ready;
static httpd_handle_t s_server;
static esp_netif_t *s_ap_netif;

static const char s_portal_html[] =
    "<!doctype html><html><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>NI Badge Portal</title>"
    "<style>body{font-family:Segoe UI,Arial,sans-serif;background:#0d1117;color:#c9d1d9;margin:0;padding:20px;}"
    ".card{max-width:520px;margin:0 auto;background:#161b22;border:1px solid #30363d;border-radius:14px;padding:18px;}"
    "h1{font-size:22px;margin:0 0 8px;color:#f1c40f;}"
    "h2{font-size:16px;margin:18px 0 8px;color:#c9d1d9;}"
    ".muted{color:#8b949e;font-size:14px;}"
    ".kv{margin:8px 0;font-size:15px;}input{width:100%;box-sizing:border-box;padding:12px;border-radius:10px;border:1px solid #30363d;background:#0d1117;color:#c9d1d9;}"
    "button{margin-top:12px;width:100%;padding:12px;border:0;border-radius:10px;background:#00a651;color:#fff;font-weight:600;}"
    "code{color:#f1c40f;}</style></head><body><div class=\"card\">"
    "<h1>NI Badge Web Portal</h1><div class=\"muted\">RTC sync and device status for the 4-inch badge.</div>"
    "<h2>Status</h2><div id=\"status\" class=\"kv\">Loading...</div>"
    "<h2>Current Time</h2><div id=\"now\" class=\"kv\">Loading...</div>"
    "<h2>Set Time</h2><div class=\"muted\">Tap Sync to set badge time from your phone.</div>"
    "<input id=\"time\" type=\"datetime-local\"><button onclick=\"syncTime()\">Sync Time</button>"
    "</div><script>"
    "async function loadStatus(){const r=await fetch('/api/status');const d=await r.json();"
    "document.getElementById('status').innerHTML='Portal: <code>'+d.state+'</code><br>SSID: <code>'+d.ssid+'</code><br>Password: <code>'+d.password+'</code><br>URL: <code>'+d.url+'</code><br>RTC: <code>'+(d.rtc_hw?'hardware':'software')+'</code>'; }"
    "async function loadTime(){const r=await fetch('/api/time');const d=await r.json();const pad=v=>String(v).padStart(2,'0');"
    "document.getElementById('now').textContent=d.year+'-'+pad(d.month)+'-'+pad(d.day)+' '+pad(d.hour)+':'+pad(d.minute)+':'+pad(d.second);"
    "const n=new Date();document.getElementById('time').value=n.getFullYear()+'-'+pad(n.getMonth()+1)+'-'+pad(n.getDate())+'T'+pad(n.getHours())+':'+pad(n.getMinutes());}"
    "async function syncTime(){const dt=new Date();const p=new URLSearchParams({year:dt.getFullYear(),month:dt.getMonth()+1,day:dt.getDate(),hour:dt.getHours(),minute:dt.getMinutes(),second:dt.getSeconds()});"
    "await fetch('/api/time',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p.toString()});await loadTime();}"
    "loadStatus();loadTime();</script></body></html>";

static const char *web_state_text(svc_web_config_state_t state)
{
    switch (state) {
    case SVC_WEB_CONFIG_STATE_STARTING:
        return "starting";
    case SVC_WEB_CONFIG_STATE_RUNNING:
        return "running";
    case SVC_WEB_CONFIG_STATE_ERROR:
        return "error";
    case SVC_WEB_CONFIG_STATE_STOPPED:
    default:
        return "stopped";
    }
}

static esp_err_t http_root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, s_portal_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t http_status_handler(httpd_req_t *req)
{
    char json[256];
    snprintf(json, sizeof(json),
             "{\"state\":\"%s\",\"ssid\":\"%s\",\"password\":\"%s\",\"url\":\"%s\",\"rtc_hw\":%s}",
             web_state_text(s_state), WEB_CONFIG_SSID, WEB_CONFIG_PASSWORD, WEB_CONFIG_URL,
             svc_rtc_is_hw_available() ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}

static esp_err_t http_time_get_handler(httpd_req_t *req)
{
    svc_rtc_time_t now = {0};
    char json[160];

    if (svc_rtc_get_now(&now) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "time unavailable");
        return ESP_FAIL;
    }

    snprintf(json, sizeof(json),
             "{\"year\":%u,\"month\":%u,\"day\":%u,\"hour\":%u,\"minute\":%u,\"second\":%u}",
             now.year, now.month, now.day, now.hour, now.minute, now.second);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}

static esp_err_t http_time_post_handler(httpd_req_t *req)
{
    char body[128] = {0};
    char value[8] = {0};
    svc_rtc_time_t next = {0};
    int received = httpd_req_recv(req, body, sizeof(body) - 1);

    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing body");
        return ESP_FAIL;
    }
    body[received] = '\0';

    if (httpd_query_key_value(body, "year", value, sizeof(value)) == ESP_OK) next.year = (uint16_t)atoi(value);
    if (httpd_query_key_value(body, "month", value, sizeof(value)) == ESP_OK) next.month = (uint8_t)atoi(value);
    if (httpd_query_key_value(body, "day", value, sizeof(value)) == ESP_OK) next.day = (uint8_t)atoi(value);
    if (httpd_query_key_value(body, "hour", value, sizeof(value)) == ESP_OK) next.hour = (uint8_t)atoi(value);
    if (httpd_query_key_value(body, "minute", value, sizeof(value)) == ESP_OK) next.minute = (uint8_t)atoi(value);
    if (httpd_query_key_value(body, "second", value, sizeof(value)) == ESP_OK) next.second = (uint8_t)atoi(value);

    if (!svc_rtc_is_time_valid(&next)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid time");
        return ESP_FAIL;
    }

    if (svc_rtc_set_now(&next) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to set time");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

static esp_err_t web_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t uri_root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = http_root_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t uri_status = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = http_status_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t uri_time_get = {
        .uri = "/api/time",
        .method = HTTP_GET,
        .handler = http_time_get_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t uri_time_post = {
        .uri = "/api/time",
        .method = HTTP_POST,
        .handler = http_time_post_handler,
        .user_ctx = NULL,
    };

    config.max_uri_handlers = 8;
    config.stack_size = 6144;

    ESP_RETURN_ON_ERROR(httpd_start(&s_server, &config), TAG, "httpd_start failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server, &uri_root), TAG, "root handler failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server, &uri_status), TAG, "status handler failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server, &uri_time_get), TAG, "time get handler failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server, &uri_time_post), TAG, "time post handler failed");
    return ESP_OK;
}

static esp_err_t web_stack_prepare(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    if (!s_stack_ready) {
        wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();

        s_ap_netif = esp_netif_create_default_wifi_ap();
        ESP_RETURN_ON_FALSE(s_ap_netif != NULL, ESP_ERR_NO_MEM, TAG, "create_default_wifi_ap failed");
        ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_cfg), TAG, "esp_wifi_init failed");
        ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "esp_wifi_set_storage failed");
        s_stack_ready = true;
    }

    return ESP_OK;
}

esp_err_t svc_web_config_start(void)
{
    wifi_config_t wifi_cfg = {0};
    esp_err_t ret;

    if (s_state == SVC_WEB_CONFIG_STATE_RUNNING) {
        return ESP_OK;
    }

    s_state = SVC_WEB_CONFIG_STATE_STARTING;

    ret = web_stack_prepare();
    if (ret != ESP_OK) {
        s_state = SVC_WEB_CONFIG_STATE_ERROR;
        return ret;
    }

    strncpy((char *)wifi_cfg.ap.ssid, WEB_CONFIG_SSID, sizeof(wifi_cfg.ap.ssid) - 1);
    strncpy((char *)wifi_cfg.ap.password, WEB_CONFIG_PASSWORD, sizeof(wifi_cfg.ap.password) - 1);
    wifi_cfg.ap.ssid_len = strlen(WEB_CONFIG_SSID);
    wifi_cfg.ap.channel = 1;
    wifi_cfg.ap.max_connection = WEB_CONFIG_MAX_CONN;
    wifi_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;

    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        s_state = SVC_WEB_CONFIG_STATE_ERROR;
        return ret;
    }

    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_cfg);
    if (ret != ESP_OK) {
        s_state = SVC_WEB_CONFIG_STATE_ERROR;
        return ret;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        s_state = SVC_WEB_CONFIG_STATE_ERROR;
        return ret;
    }

    ret = web_server_start();
    if (ret != ESP_OK) {
        esp_wifi_stop();
        s_state = SVC_WEB_CONFIG_STATE_ERROR;
        return ret;
    }

    s_state = SVC_WEB_CONFIG_STATE_RUNNING;
    ESP_LOGI(TAG, "Portal ready at %s (SSID=%s)", WEB_CONFIG_URL, WEB_CONFIG_SSID);
    return ESP_OK;
}

esp_err_t svc_web_config_stop(void)
{
    esp_err_t ret = ESP_OK;

    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }

    if (s_stack_ready) {
        ret = esp_wifi_stop();
        if (ret != ESP_OK && ret != ESP_ERR_WIFI_NOT_STARTED) {
            s_state = SVC_WEB_CONFIG_STATE_ERROR;
            return ret;
        }
    }

    s_state = SVC_WEB_CONFIG_STATE_STOPPED;
    return ESP_OK;
}

svc_web_config_state_t svc_web_config_get_state(void)
{
    return s_state;
}

bool svc_web_config_is_running(void)
{
    return s_state == SVC_WEB_CONFIG_STATE_RUNNING;
}

const char *svc_web_config_get_ssid(void)
{
    return WEB_CONFIG_SSID;
}

const char *svc_web_config_get_password(void)
{
    return WEB_CONFIG_PASSWORD;
}

const char *svc_web_config_get_url(void)
{
    return WEB_CONFIG_URL;
}
