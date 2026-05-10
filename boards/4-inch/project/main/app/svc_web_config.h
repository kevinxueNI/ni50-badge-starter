#pragma once

#include <stdbool.h>
#include "esp_err.h"

typedef enum {
    SVC_WEB_CONFIG_STATE_STOPPED = 0,
    SVC_WEB_CONFIG_STATE_STARTING,
    SVC_WEB_CONFIG_STATE_RUNNING,
    SVC_WEB_CONFIG_STATE_ERROR,
} svc_web_config_state_t;

esp_err_t svc_web_config_start(void);
esp_err_t svc_web_config_stop(void);
svc_web_config_state_t svc_web_config_get_state(void);
bool svc_web_config_is_running(void);
const char *svc_web_config_get_ssid(void);
const char *svc_web_config_get_password(void);
const char *svc_web_config_get_url(void);
