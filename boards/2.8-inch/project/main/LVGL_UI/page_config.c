/**
 * page_config.c — Page 7: Configuration info page
 *
 * Shows WiFi connection instructions (text only, no heavy canvas/QR rendering).
 * WiFi AP + HTTP server start in a separate FreeRTOS task.
 */

#include "page_config.h"
#include "config_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "PageConfig";

/* Colors matching Badge_UI theme */
#define COLOR_BG_DARK       lv_color_hex(0x0D1B2A)
#define COLOR_ACCENT_YELLOW lv_color_hex(0xF5C518)
#define COLOR_TEXT_WHITE     lv_color_hex(0xFFFFFF)

/* ── Config server task ── */
static void config_server_task(void *arg)
{
    /* Small delay to let LVGL finish init */
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Starting config server in background task...");
    config_server_start();
    vTaskDelete(NULL);
}

void build_page_config(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Config");
    lv_obj_set_style_text_color(title, COLOR_ACCENT_YELLOW, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    /* WiFi Info Box */
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, 220, 180);
    lv_obj_align(box, LV_ALIGN_CENTER, 0, 6);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(box, COLOR_ACCENT_YELLOW, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_radius(box, 12, 0);
    lv_obj_set_style_pad_all(box, 12, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_wifi = lv_label_create(box);
    lv_label_set_text(lbl_wifi,
        LV_SYMBOL_WIFI " WiFi:\n"
        "NI-Badge\n\n"
        LV_SYMBOL_EYE_OPEN " Pass:\n"
        "ni502026");
    lv_obj_set_style_text_color(lbl_wifi, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(lbl_wifi, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_line_space(lbl_wifi, 2, 0);
    lv_obj_align(lbl_wifi, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *lbl_url_title = lv_label_create(box);
    lv_label_set_text(lbl_url_title, LV_SYMBOL_GPS " URL");
    lv_obj_set_style_text_color(lbl_url_title, COLOR_ACCENT_YELLOW, 0);
    lv_obj_set_style_text_font(lbl_url_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_url_title, LV_ALIGN_TOP_LEFT, 0, 92);

    lv_obj_t *url_panel = lv_obj_create(box);
    lv_obj_set_size(url_panel, 196, 42);
    lv_obj_align(url_panel, LV_ALIGN_TOP_LEFT, 0, 116);
    lv_obj_set_style_bg_color(url_panel, lv_color_hex(0x0B1320), 0);
    lv_obj_set_style_bg_opa(url_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(url_panel, 0, 0);
    lv_obj_set_style_radius(url_panel, 8, 0);
    lv_obj_set_style_pad_all(url_panel, 0, 0);
    lv_obj_clear_flag(url_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_url = lv_label_create(url_panel);
    lv_label_set_text(lbl_url, "192.168.4.1");
    lv_obj_set_style_text_color(lbl_url, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(lbl_url, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_url);

    /* Bottom hint */
    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "Connect WiFi, then open 192.168.4.1");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);

    /* Start config server in a separate task (not blocking LVGL) */
    xTaskCreatePinnedToCore(config_server_task, "cfg_srv", 4096, NULL, 3, NULL, 0);
}
