#pragma once

#include "esp_err.h"

#define CONFIG_AP_SSID      "NI-Badge"
#define CONFIG_AP_PASS      "ni502026"
#define CONFIG_AP_MAX_CONN  2
#define CONFIG_SERVER_IP    "192.168.4.1"
#define CONFIG_SERVER_PORT  80

/* SD card paths for user-customizable data */
#define CFG_DIR             "/sdcard/config"
#define CFG_PROFILE_PATH    "/sdcard/config/profile.txt"
#define CFG_AVATAR_PATH     "/sdcard/config/avatar.bin"
#define CFG_CARTOON_PATH    "/sdcard/config/cartoon.bin"

/* Profile fields (plain text, line-based) */
typedef struct {
    char name[64];
    char dept[128];
} badge_profile_t;

/**
 * Start WiFi AP + HTTP config server.
 * Call after SD card is mounted.
 */
esp_err_t config_server_start(void);

/**
 * Stop WiFi AP + HTTP config server.
 */
esp_err_t config_server_stop(void);

/**
 * Load profile from SD card. Returns ESP_OK if file exists.
 */
esp_err_t config_load_profile(badge_profile_t *profile);

/**
 * Check if custom avatar exists on SD card.
 */
bool config_has_custom_avatar(void);

/**
 * Check if custom cartoon exists on SD card.
 */
bool config_has_custom_cartoon(void);
