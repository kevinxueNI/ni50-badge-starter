/**
 * page_config.c - Settings page
 * Brightness (hardware), Time setting, WiFi, About, Factory Reset
 */
#include "ui.h"
#include "svc_web_config.h"
#include "sys_idle.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_mac.h"
#include <stdio.h>

static const char *TAG = "page_cfg";

/* Declared in widget_statusbar.c */
extern void widget_statusbar_set_time(int hour, int min);
extern int  widget_statusbar_get_hour(void);
extern int  widget_statusbar_get_min(void);

/* ── Brightness ── */
static void brightness_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    ESP_LOGI(TAG, "Brightness: %d%%", val);
    sys_idle_set_user_brightness((uint8_t)val);
}

/* ── Time set ── */
static lv_obj_t *roller_hour = NULL;
static lv_obj_t *roller_min  = NULL;
static lv_obj_t *lbl_portal_state = NULL;
static lv_obj_t *lbl_portal_meta = NULL;
static lv_obj_t *btn_portal = NULL;
static lv_obj_t *lbl_portal_btn = NULL;

static void portal_refresh_ui(void)
{
    svc_web_config_state_t state;
    lv_color_t state_color = lv_color_hex(0x8B949E);
    const char *state_text = "OFFLINE";
    char meta[160];

    if (!lbl_portal_state || !lbl_portal_meta || !lbl_portal_btn) {
        return;
    }

    state = svc_web_config_get_state();
    switch (state) {
    case SVC_WEB_CONFIG_STATE_STARTING:
        state_text = "STARTING";
        state_color = lv_color_hex(0xF1C40F);
        break;
    case SVC_WEB_CONFIG_STATE_RUNNING:
        state_text = "RUNNING";
        state_color = lv_color_hex(0x00A651);
        break;
    case SVC_WEB_CONFIG_STATE_ERROR:
        state_text = "ERROR";
        state_color = lv_color_hex(0xE74C3C);
        break;
    case SVC_WEB_CONFIG_STATE_STOPPED:
    default:
        break;
    }

    lv_label_set_text(lbl_portal_state, state_text);
    lv_obj_set_style_text_color(lbl_portal_state, state_color, 0);

    snprintf(meta, sizeof(meta),
             "SSID: %s\nPASS: %s\nURL: %s",
             svc_web_config_get_ssid(), svc_web_config_get_password(), svc_web_config_get_url());
    lv_label_set_text(lbl_portal_meta, meta);
    lv_label_set_text(lbl_portal_btn, svc_web_config_is_running() ? "Stop Portal" : "Start Portal");
}

static void portal_status_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    portal_refresh_ui();
}

static void time_set_cb(lv_event_t *e)
{
    (void)e;
    int h = lv_roller_get_selected(roller_hour);
    int m = lv_roller_get_selected(roller_min);
    widget_statusbar_set_time(h, m);
    ESP_LOGI(TAG, "Time set to %02d:%02d", h, m);
}

static void portal_toggle_cb(lv_event_t *e)
{
    esp_err_t ret;

    (void)e;
    if (svc_web_config_is_running()) {
        ret = svc_web_config_stop();
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Web portal stopped");
        }
    } else {
        ret = svc_web_config_start();
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Web portal started: %s", svc_web_config_get_url());
        }
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Portal toggle failed: %s", esp_err_to_name(ret));
    }
    portal_refresh_ui();
}

/* ── Screen-off timeout ── */
static lv_obj_t *timeout_btns[3];
static const uint32_t timeout_values[3] = {10000, 120000, 0}; /* 10s, 2min, never */

static void timeout_update_styles(int selected)
{
    for (int i = 0; i < 3; i++) {
        if (timeout_btns[i]) {
            lv_obj_set_style_bg_color(timeout_btns[i],
                (i == selected) ? lv_color_hex(0x00A651) : lv_color_hex(0x30363D), 0);
        }
    }
}

static void timeout_btn_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    sys_idle_set_timeout(timeout_values[idx]);
    timeout_update_styles(idx);
}

/* ── Factory reset ── */
static void reset_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGW(TAG, "Factory reset triggered");
    esp_restart();
}

/* ── Page create ── */
lv_obj_t *page_config_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "SETTINGS");
    lv_obj_set_style_text_color(title, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_letter_space(title, 2, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    int y_pos = 38;

    /* ── Brightness section (left half) ── */
    lv_obj_t *lbl_br = lv_label_create(parent);
    lv_label_set_text(lbl_br, LV_SYMBOL_SETTINGS " Brightness");
    lv_obj_set_style_text_color(lbl_br, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(lbl_br, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(lbl_br, 30, y_pos);

    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_width(slider, 190);
    lv_obj_set_pos(slider, 30, y_pos + 30);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, sys_idle_get_user_brightness(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x00A651), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x00A651), LV_PART_KNOB);
    lv_obj_add_event_cb(slider, brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ── Screen Off section (right half) ── */
    lv_obj_t *lbl_so = lv_label_create(parent);
    lv_label_set_text(lbl_so, LV_SYMBOL_EYE_CLOSE " Screen Off");
    lv_obj_set_style_text_color(lbl_so, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(lbl_so, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(lbl_so, 260, y_pos);

    static const char *timeout_labels[3] = {"10s", "2min", "Never"};
    uint32_t cur_timeout = sys_idle_get_timeout();
    int cur_sel = 1; /* default 2min */
    for (int i = 0; i < 3; i++) {
        if (timeout_values[i] == cur_timeout) { cur_sel = i; break; }
    }

    for (int i = 0; i < 3; i++) {
        timeout_btns[i] = lv_btn_create(parent);
        lv_obj_set_size(timeout_btns[i], 54, 30);
        lv_obj_set_pos(timeout_btns[i], 260 + i * 58, y_pos + 26);
        lv_obj_set_style_bg_color(timeout_btns[i],
            (i == cur_sel) ? lv_color_hex(0x00A651) : lv_color_hex(0x30363D), 0);
        lv_obj_set_style_radius(timeout_btns[i], 6, 0);
        lv_obj_set_style_shadow_width(timeout_btns[i], 0, 0);
        lv_obj_add_event_cb(timeout_btns[i], timeout_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *bl = lv_label_create(timeout_btns[i]);
        lv_label_set_text(bl, timeout_labels[i]);
        lv_obj_set_style_text_color(bl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
        lv_obj_center(bl);
    }

    y_pos += 74;

    /* ── Time section ── */
    lv_obj_t *lbl_time = lv_label_create(parent);
    lv_label_set_text(lbl_time, LV_SYMBOL_BELL " Set Time");
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(lbl_time, 30, y_pos);

    /* Hour roller */
    roller_hour = lv_roller_create(parent);
    lv_roller_set_options(roller_hour,
        "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n"
        "12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
        LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(roller_hour, 3);
    lv_obj_set_pos(roller_hour, 90, y_pos + 28);
    lv_obj_set_width(roller_hour, 70);
    lv_obj_set_style_bg_color(roller_hour, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_text_color(roller_hour, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_border_color(roller_hour, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_text_font(roller_hour, &lv_font_montserrat_18, 0);
    lv_obj_set_style_bg_color(roller_hour, lv_color_hex(0x00A651), LV_PART_SELECTED);
    lv_roller_set_selected(roller_hour, widget_statusbar_get_hour(), LV_ANIM_OFF);

    lv_obj_t *colon = lv_label_create(parent);
    lv_label_set_text(colon, ":");
    lv_obj_set_style_text_color(colon, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(colon, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(colon, 168, y_pos + 50);

    /* Minute roller */
    roller_min = lv_roller_create(parent);
    char min_opts[256] = "";
    for (int i = 0; i < 60; i++) {
        char tmp[4];
        snprintf(tmp, 4, "%02d", i);
        strcat(min_opts, tmp);
        if (i < 59) strcat(min_opts, "\n");
    }
    lv_roller_set_options(roller_min, min_opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(roller_min, 3);
    lv_obj_set_pos(roller_min, 185, y_pos + 28);
    lv_obj_set_width(roller_min, 70);
    lv_obj_set_style_bg_color(roller_min, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_text_color(roller_min, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_border_color(roller_min, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_text_font(roller_min, &lv_font_montserrat_18, 0);
    lv_obj_set_style_bg_color(roller_min, lv_color_hex(0x00A651), LV_PART_SELECTED);
    lv_roller_set_selected(roller_min, widget_statusbar_get_min(), LV_ANIM_OFF);

    /* Set button */
    lv_obj_t *btn_set = lv_btn_create(parent);
    lv_obj_set_size(btn_set, 80, 36);
    lv_obj_set_pos(btn_set, 280, y_pos + 50);
    lv_obj_set_style_bg_color(btn_set, lv_color_hex(0x00A651), 0);
    lv_obj_set_style_radius(btn_set, 6, 0);
    lv_obj_set_style_shadow_width(btn_set, 0, 0);
    lv_obj_add_event_cb(btn_set, time_set_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(btn_set);
    lv_label_set_text(sl, "Set");
    lv_obj_set_style_text_color(sl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_16, 0);
    lv_obj_center(sl);

    y_pos += 130;

    /* ── Web Portal ── */
    lv_obj_t *lbl_portal = lv_label_create(parent);
    lv_label_set_text(lbl_portal, LV_SYMBOL_WIFI " Web Portal");
    lv_obj_set_style_text_color(lbl_portal, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(lbl_portal, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(lbl_portal, 30, y_pos);

    lbl_portal_state = lv_label_create(parent);
    lv_label_set_text(lbl_portal_state, "OFFLINE");
    lv_obj_set_style_text_color(lbl_portal_state, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(lbl_portal_state, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(lbl_portal_state, 50, y_pos + 38);

    lbl_portal_meta = lv_label_create(parent);
    lv_obj_set_width(lbl_portal_meta, 230);
    lv_label_set_long_mode(lbl_portal_meta, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(lbl_portal_meta, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(lbl_portal_meta, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_portal_meta, 50, y_pos + 66);

    portal_refresh_ui();
    lv_timer_create(portal_status_timer_cb, 1500, NULL);

    /* ── Bottom row: Start Portal (left) + About (right) ── */

    btn_portal = lv_btn_create(parent);
    lv_obj_set_size(btn_portal, 128, 40);
    lv_obj_set_pos(btn_portal, 30, 378);
    lv_obj_set_style_bg_color(btn_portal, lv_color_hex(0x1E2740), 0);
    lv_obj_set_style_radius(btn_portal, 8, 0);
    lv_obj_set_style_shadow_width(btn_portal, 0, 0);
    lv_obj_add_event_cb(btn_portal, portal_toggle_cb, LV_EVENT_CLICKED, NULL);

    lbl_portal_btn = lv_label_create(btn_portal);
    lv_label_set_text(lbl_portal_btn, "Start Portal");
    lv_obj_set_style_text_color(lbl_portal_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_portal_btn, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_portal_btn);

    /* About */
    lv_obj_t *lbl_about = lv_label_create(parent);
    lv_label_set_text(lbl_about, LV_SYMBOL_LIST " About");
    lv_obj_set_style_text_color(lbl_about, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(lbl_about, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_about, 260, 362);

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char info[128];
    snprintf(info, sizeof(info),
        "NI 50th Badge v2.0\n"
        "Board: ESP32-S3 LCD 4\"\n"
        "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    lv_obj_t *about_lbl = lv_label_create(parent);
    lv_label_set_text(about_lbl, info);
    lv_obj_set_style_text_color(about_lbl, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(about_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(about_lbl, 260, 380);

    ESP_LOGI(TAG, "Config page created");
    return parent;
}
