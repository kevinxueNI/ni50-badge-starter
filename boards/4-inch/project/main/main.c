/**
 * ESP32-S3-Touch-LCD-4 Badge Project
 * Board: Waveshare ESP32-S3-Touch-LCD-4 (480×480 RGB, GT911 touch)
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "lvgl.h"
#include "svc_profile.h"
#include "svc_rtc.h"
#include "sys_idle.h"
#include "ui/ui.h"

static const char *TAG = "badge";

/* Print heap stats */
static void print_heap_info(void)
{
    ESP_LOGI(TAG, "Free heap (internal): %lu KB",
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024);
    ESP_LOGI(TAG, "Free heap (PSRAM):    %lu KB",
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
    ESP_LOGI(TAG, "Min free heap (internal): %lu KB",
             (unsigned long)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) / 1024);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Badge app...");

    /* BSP initializes display (ST7701 480x480 RGB), touch (GT911), backlight */
    bsp_display_start();

    /* Bring up persisted badge profile before UI pages bind their text. */
    svc_profile_init();

    /* Bring up the shared clock source before any UI widgets read time. */
    svc_rtc_init();

    /* Lock LVGL mutex before touching UI */
    bsp_display_lock(0);

    /* Initialize full badge UI */
    ui_init();

    /* Idle dimming + touch wake shares the hardware backlight owner. */
    sys_idle_init();

    /* Start NI-style boot intro, then fade into the main framework */
    app_boot_start();

    bsp_display_unlock();

    /* Print memory stats after UI init */
    print_heap_info();

    ESP_LOGI(TAG, "UI ready.");
}
