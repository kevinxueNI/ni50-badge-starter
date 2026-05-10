#include "config_server.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "PCF85063.h"

static const char *TAG = "ConfigSrv";
static httpd_handle_t server = NULL;

/* ── Embedded HTML page (gzipped would be better but plain for simplicity) ── */
extern const char config_page_html_start[] asm("_binary_config_page_html_start");
extern const char config_page_html_end[]   asm("_binary_config_page_html_end");

/* ══════════════════════════════════════════════════════════════════
 *  Profile helpers
 * ══════════════════════════════════════════════════════════════════ */
esp_err_t config_load_profile(badge_profile_t *profile)
{
    FILE *f = fopen(CFG_PROFILE_PATH, "r");
    if (!f) return ESP_ERR_NOT_FOUND;

    memset(profile, 0, sizeof(*profile));
    if (fgets(profile->name, sizeof(profile->name), f)) {
        /* strip newline */
        char *nl = strchr(profile->name, '\n');
        if (nl) *nl = '\0';
    }
    if (fgets(profile->dept, sizeof(profile->dept), f)) {
        char *nl = strchr(profile->dept, '\n');
        if (nl) *nl = '\0';
    }
    fclose(f);
    ESP_LOGI(TAG, "Profile loaded: name='%s' dept='%s'", profile->name, profile->dept);
    return ESP_OK;
}

static esp_err_t save_profile(const char *name, const char *dept)
{
    /* Ensure config directory exists */
    struct stat st;
    if (stat(CFG_DIR, &st) != 0) {
        mkdir(CFG_DIR, 0755);
    }

    FILE *f = fopen(CFG_PROFILE_PATH, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open profile for writing");
        return ESP_FAIL;
    }
    fprintf(f, "%s\n%s\n", name, dept);
    fclose(f);
    ESP_LOGI(TAG, "Profile saved: name='%s' dept='%s'", name, dept);
    return ESP_OK;
}

bool config_has_custom_avatar(void)
{
    struct stat st;
    return (stat(CFG_AVATAR_PATH, &st) == 0 && st.st_size > 0);
}

bool config_has_custom_cartoon(void)
{
    struct stat st;
    return (stat(CFG_CARTOON_PATH, &st) == 0 && st.st_size > 0);
}

/* ══════════════════════════════════════════════════════════════════
 *  HTTP Handlers
 * ══════════════════════════════════════════════════════════════════ */

/* GET / — serve config page */
static esp_err_t handler_get_root(httpd_req_t *req)
{
    size_t len = config_page_html_end - config_page_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, config_page_html_start, len);
    return ESP_OK;
}

/* GET /api/profile — return current profile as JSON */
static esp_err_t handler_get_profile(httpd_req_t *req)
{
    badge_profile_t prof = {0};
    config_load_profile(&prof);
    /* If no profile, return defaults */
    if (prof.name[0] == '\0') strncpy(prof.name, "Kevin Xue", sizeof(prof.name) - 1);
    if (prof.dept[0] == '\0') strncpy(prof.dept, "NI China Innovation Center", sizeof(prof.dept) - 1);

    char json[256];
    snprintf(json, sizeof(json), "{\"name\":\"%s\",\"dept\":\"%s\"}", prof.name, prof.dept);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}

/* POST /api/profile — save name & dept */
static esp_err_t handler_post_profile(httpd_req_t *req)
{
    char buf[256] = {0};
    int received = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    buf[received] = '\0';

    /* Parse simple form: name=xxx&dept=xxx (URL-encoded) */
    char name[64] = {0}, dept[128] = {0};

    /* Simple parser — find name= and dept= */
    char *p_name = strstr(buf, "name=");
    char *p_dept = strstr(buf, "dept=");

    if (p_name) {
        p_name += 5;
        char *end = strchr(p_name, '&');
        size_t len = end ? (size_t)(end - p_name) : strlen(p_name);
        if (len >= sizeof(name)) len = sizeof(name) - 1;
        memcpy(name, p_name, len);
        name[len] = '\0';
    }
    if (p_dept) {
        p_dept += 5;
        char *end = strchr(p_dept, '&');
        size_t len = end ? (size_t)(end - p_dept) : strlen(p_dept);
        if (len >= sizeof(dept)) len = sizeof(dept) - 1;
        memcpy(dept, p_dept, len);
        dept[len] = '\0';
    }

    /* URL-decode spaces (+ → space, %XX) — simple version */
    for (char *c = name; *c; c++) { if (*c == '+') *c = ' '; }
    for (char *c = dept; *c; c++) { if (*c == '+') *c = ' '; }

    save_profile(name, dept);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

/* POST /api/upload?type=avatar|cartoon — receive raw image binary */
#define UPLOAD_BUF_SIZE 4096

static esp_err_t handler_post_upload(httpd_req_t *req)
{
    /* Determine target file from query param */
    char query[32] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing type param");
        return ESP_FAIL;
    }

    char type_val[16] = {0};
    if (httpd_query_key_value(query, "type", type_val, sizeof(type_val)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing type param");
        return ESP_FAIL;
    }

    const char *path = NULL;
    if (strcmp(type_val, "avatar") == 0) {
        path = CFG_AVATAR_PATH;
    } else if (strcmp(type_val, "cartoon") == 0) {
        path = CFG_CARTOON_PATH;
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid type");
        return ESP_FAIL;
    }

    /* Ensure config directory */
    struct stat st;
    if (stat(CFG_DIR, &st) != 0) {
        mkdir(CFG_DIR, 0755);
    }

    /* Receive and write to SD card */
    FILE *f = fopen(path, "wb");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File open failed");
        return ESP_FAIL;
    }

    int remaining = req->content_len;
    char *buf = malloc(UPLOAD_BUF_SIZE);
    if (!buf) {
        fclose(f);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Receiving %s upload, %d bytes", type_val, remaining);

    while (remaining > 0) {
        int to_read = remaining > UPLOAD_BUF_SIZE ? UPLOAD_BUF_SIZE : remaining;
        int received = httpd_req_recv(req, buf, to_read);
        if (received <= 0) {
            ESP_LOGE(TAG, "Upload receive error");
            break;
        }
        fwrite(buf, 1, received, f);
        remaining -= received;
    }

    free(buf);
    fclose(f);

    if (remaining == 0) {
        ESP_LOGI(TAG, "Upload %s complete: %s", type_val, path);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"ok\":true}");
    } else {
        unlink(path);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Incomplete upload");
    }
    return ESP_OK;
}

/* GET /api/time — return current RTC time as JSON */
static esp_err_t handler_get_time(httpd_req_t *req)
{
    datetime_t now = {0};
    PCF85063_Read_Time(&now);

    char json[128];
    snprintf(json, sizeof(json),
             "{\"year\":%d,\"month\":%d,\"day\":%d,\"hour\":%d,\"minute\":%d,\"second\":%d}",
             now.year, now.month, now.day, now.hour, now.minute, now.second);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}

/* POST /api/time — set RTC time, body: year=2026&month=5&day=2&hour=14&minute=30&second=0 */
static esp_err_t handler_post_time(httpd_req_t *req)
{
    char buf[128] = {0};
    int received = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    buf[received] = '\0';

    datetime_t t = {0};
    char val[8] = {0};

    if (httpd_query_key_value(buf, "year", val, sizeof(val)) == ESP_OK) t.year = atoi(val);
    if (httpd_query_key_value(buf, "month", val, sizeof(val)) == ESP_OK) t.month = atoi(val);
    if (httpd_query_key_value(buf, "day", val, sizeof(val)) == ESP_OK) t.day = atoi(val);
    if (httpd_query_key_value(buf, "hour", val, sizeof(val)) == ESP_OK) t.hour = atoi(val);
    if (httpd_query_key_value(buf, "minute", val, sizeof(val)) == ESP_OK) t.minute = atoi(val);
    if (httpd_query_key_value(buf, "second", val, sizeof(val)) == ESP_OK) t.second = atoi(val);

    if (t.year < 2025 || t.year > 2099 || t.month == 0 || t.month > 12 || t.day == 0 || t.day > 31) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid time values");
        return ESP_FAIL;
    }

    PCF85063_Set_All(t);
    ESP_LOGI(TAG, "RTC time set via API: %d-%02d-%02d %02d:%02d:%02d",
             t.year, t.month, t.day, t.hour, t.minute, t.second);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

/* ══════════════════════════════════════════════════════════════════
 *  WiFi AP + Server lifecycle
 * ══════════════════════════════════════════════════════════════════ */

static void wifi_ap_init(void)
{
    /* NVS init */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    /* Network stack init (safe to call multiple times — errors ignored) */
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_AP_SSID,
            .ssid_len = strlen(CONFIG_AP_SSID),
            .password = CONFIG_AP_PASS,
            .max_connection = CONFIG_AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .channel = 1,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "WiFi AP started: SSID=%s PASS=%s", CONFIG_AP_SSID, CONFIG_AP_PASS);
}

static httpd_handle_t start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.stack_size = 8192;

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return NULL;
    }

    /* Register URI handlers */
    httpd_uri_t uri_root = {
        .uri = "/", .method = HTTP_GET, .handler = handler_get_root
    };
    httpd_uri_t uri_get_profile = {
        .uri = "/api/profile", .method = HTTP_GET, .handler = handler_get_profile
    };
    httpd_uri_t uri_post_profile = {
        .uri = "/api/profile", .method = HTTP_POST, .handler = handler_post_profile
    };
    httpd_uri_t uri_upload = {
        .uri = "/api/upload", .method = HTTP_POST, .handler = handler_post_upload
    };
    httpd_uri_t uri_get_time = {
        .uri = "/api/time", .method = HTTP_GET, .handler = handler_get_time
    };
    httpd_uri_t uri_post_time = {
        .uri = "/api/time", .method = HTTP_POST, .handler = handler_post_time
    };

    httpd_register_uri_handler(server, &uri_root);
    httpd_register_uri_handler(server, &uri_get_profile);
    httpd_register_uri_handler(server, &uri_post_profile);
    httpd_register_uri_handler(server, &uri_upload);
    httpd_register_uri_handler(server, &uri_get_time);
    httpd_register_uri_handler(server, &uri_post_time);

    ESP_LOGI(TAG, "HTTP server started on port %d", CONFIG_SERVER_PORT);
    return server;
}

esp_err_t config_server_start(void)
{
    ESP_LOGI(TAG, "Starting config server...");
    wifi_ap_init();
    start_http_server();
    return ESP_OK;
}

esp_err_t config_server_stop(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    esp_wifi_stop();
    ESP_LOGI(TAG, "Config server stopped");
    return ESP_OK;
}
