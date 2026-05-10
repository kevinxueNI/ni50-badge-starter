/**
 * page_settings.c — Settings page (brightness + auto-sleep)
 *
 * Independent screen, not part of the main tileview.
 * No NVS persistence — resets to defaults on reboot.
 */

#include "page_settings.h"
#include "ST7789.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "Settings";

/* ── Settings state (volatile, resets on reboot) ── */
static uint8_t s_brightness = 70;           /* default brightness */
static uint16_t s_sleep_timeout_s = 0;      /* 0 = never */
static int64_t s_last_activity_us = 0;      /* last touch timestamp */
static bool s_screen_sleeping = false;

/* ── UI handles ── */
static lv_obj_t *settings_screen = NULL;
static lv_obj_t *slider_brightness = NULL;
static lv_obj_t *lbl_brightness_val = NULL;
static lv_obj_t *btn_10s = NULL;
static lv_obj_t *btn_2min = NULL;
static lv_obj_t *btn_never = NULL;

/* Colors */
#define S_COLOR_BG       lv_color_hex(0x0D1117)
#define S_COLOR_PANEL    lv_color_hex(0x161B22)
#define S_COLOR_ACCENT   lv_color_hex(0x00A651)
#define S_COLOR_YELLOW   lv_color_hex(0xF1C40F)
#define S_COLOR_TEXT     lv_color_hex(0xC9D1D9)
#define S_COLOR_MUTED    lv_color_hex(0x8B949E)
#define S_COLOR_BTN_SEL  lv_color_hex(0x00A651)
#define S_COLOR_BTN_NORM lv_color_hex(0x1F2428)
#define S_COLOR_BORDER   lv_color_hex(0x30363D)

/* ── Forward declarations ── */
static void update_timeout_buttons(void);
static void return_to_main(void);

/* ── Callbacks ── */
static void brightness_slider_cb(lv_event_t *e)
{
    int val = lv_slider_get_value(lv_event_get_target(e));
    s_brightness = (uint8_t)val;
    LCD_Backlight = s_brightness;
    Set_Backlight(s_brightness);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", val);
    lv_label_set_text(lbl_brightness_val, buf);
}

static void btn_10s_cb(lv_event_t *e)
{
    (void)e;
    s_sleep_timeout_s = SLEEP_TIMEOUT_10S;
    update_timeout_buttons();
}

static void btn_2min_cb(lv_event_t *e)
{
    (void)e;
    s_sleep_timeout_s = SLEEP_TIMEOUT_2MIN;
    update_timeout_buttons();
}

static void btn_never_cb(lv_event_t *e)
{
    (void)e;
    s_sleep_timeout_s = SLEEP_TIMEOUT_NEVER;
    update_timeout_buttons();
}

static void btn_save_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "Settings saved: brightness=%d, sleep=%ds", s_brightness, s_sleep_timeout_s);
    return_to_main();
}

static void btn_back_cb(lv_event_t *e)
{
    (void)e;
    return_to_main();
}

static void return_to_main(void)
{
    /* Switch back to the main screen (the main tileview screen) */
    extern lv_obj_t *badge_main_screen;
    if (badge_main_screen) {
        lv_scr_load_anim(badge_main_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, true);
    }
    settings_screen = NULL;
}

/* ── Timeout button visual update ── */
static void style_timeout_btn(lv_obj_t *btn, bool selected)
{
    if (selected) {
        lv_obj_set_style_bg_color(btn, S_COLOR_BTN_SEL, 0);
        lv_obj_set_style_border_color(btn, S_COLOR_BTN_SEL, 0);
    } else {
        lv_obj_set_style_bg_color(btn, S_COLOR_BTN_NORM, 0);
        lv_obj_set_style_border_color(btn, S_COLOR_BORDER, 0);
    }
}

static void update_timeout_buttons(void)
{
    style_timeout_btn(btn_10s, s_sleep_timeout_s == SLEEP_TIMEOUT_10S);
    style_timeout_btn(btn_2min, s_sleep_timeout_s == SLEEP_TIMEOUT_2MIN);
    style_timeout_btn(btn_never, s_sleep_timeout_s == SLEEP_TIMEOUT_NEVER);
}

/* ── Build Settings Screen ── */
void page_settings_show(void)
{
    if (settings_screen) return;  /* already showing */

    settings_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(settings_screen, S_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(settings_screen, LV_OPA_COVER, 0);

    /* Title */
    lv_obj_t *title = lv_label_create(settings_screen);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Settings");
    lv_obj_set_style_text_color(title, S_COLOR_YELLOW, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    /* ── Brightness Section ── */
    lv_obj_t *lbl_bright = lv_label_create(settings_screen);
    lv_label_set_text(lbl_bright, "Brightness");
    lv_obj_set_style_text_color(lbl_bright, S_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(lbl_bright, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_bright, LV_ALIGN_TOP_LEFT, 16, 50);

    lbl_brightness_val = lv_label_create(settings_screen);
    char bv[8];
    snprintf(bv, sizeof(bv), "%d%%", s_brightness);
    lv_label_set_text(lbl_brightness_val, bv);
    lv_obj_set_style_text_color(lbl_brightness_val, S_COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(lbl_brightness_val, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_brightness_val, LV_ALIGN_TOP_RIGHT, -16, 50);

    slider_brightness = lv_slider_create(settings_screen);
    lv_obj_set_size(slider_brightness, 208, 20);
    lv_obj_align(slider_brightness, LV_ALIGN_TOP_MID, 0, 74);
    lv_slider_set_range(slider_brightness, 5, Backlight_MAX);
    lv_slider_set_value(slider_brightness, s_brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_brightness, lv_color_hex(0x2D333B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_brightness, S_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_brightness, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider_brightness, 3, LV_PART_KNOB);
    lv_obj_add_event_cb(slider_brightness, brightness_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ── Auto-Sleep Section ── */
    lv_obj_t *lbl_sleep = lv_label_create(settings_screen);
    lv_label_set_text(lbl_sleep, "Auto Sleep");
    lv_obj_set_style_text_color(lbl_sleep, S_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(lbl_sleep, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_sleep, LV_ALIGN_TOP_LEFT, 16, 116);

    /* 3 timeout buttons in a row */
    int btn_w = 64;
    int btn_h = 32;
    int btn_y = 142;
    int gap = 8;
    int total_w = 3 * btn_w + 2 * gap;
    int start_x = (240 - total_w) / 2;

    btn_10s = lv_btn_create(settings_screen);
    lv_obj_set_size(btn_10s, btn_w, btn_h);
    lv_obj_set_pos(btn_10s, start_x, btn_y);
    lv_obj_set_style_radius(btn_10s, 6, 0);
    lv_obj_set_style_border_width(btn_10s, 1, 0);
    lv_obj_add_event_cb(btn_10s, btn_10s_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l1 = lv_label_create(btn_10s);
    lv_label_set_text(l1, "10s");
    lv_obj_set_style_text_font(l1, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l1, lv_color_white(), 0);
    lv_obj_center(l1);

    btn_2min = lv_btn_create(settings_screen);
    lv_obj_set_size(btn_2min, btn_w, btn_h);
    lv_obj_set_pos(btn_2min, start_x + btn_w + gap, btn_y);
    lv_obj_set_style_radius(btn_2min, 6, 0);
    lv_obj_set_style_border_width(btn_2min, 1, 0);
    lv_obj_add_event_cb(btn_2min, btn_2min_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l2 = lv_label_create(btn_2min);
    lv_label_set_text(l2, "2min");
    lv_obj_set_style_text_font(l2, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l2, lv_color_white(), 0);
    lv_obj_center(l2);

    btn_never = lv_btn_create(settings_screen);
    lv_obj_set_size(btn_never, btn_w, btn_h);
    lv_obj_set_pos(btn_never, start_x + 2 * (btn_w + gap), btn_y);
    lv_obj_set_style_radius(btn_never, 6, 0);
    lv_obj_set_style_border_width(btn_never, 1, 0);
    lv_obj_add_event_cb(btn_never, btn_never_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l3 = lv_label_create(btn_never);
    lv_label_set_text(l3, "Never");
    lv_obj_set_style_text_font(l3, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l3, lv_color_white(), 0);
    lv_obj_center(l3);

    update_timeout_buttons();

    /* ── Separator ── */
    lv_obj_t *sep = lv_obj_create(settings_screen);
    lv_obj_set_size(sep, 200, 1);
    lv_obj_align(sep, LV_ALIGN_TOP_MID, 0, 192);
    lv_obj_set_style_bg_color(sep, S_COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Action buttons ── */
    lv_obj_t *btn_save = lv_btn_create(settings_screen);
    lv_obj_set_size(btn_save, 100, 36);
    lv_obj_align(btn_save, LV_ALIGN_TOP_MID, 0, 210);
    lv_obj_set_style_bg_color(btn_save, S_COLOR_ACCENT, 0);
    lv_obj_set_style_radius(btn_save, 8, 0);
    lv_obj_add_event_cb(btn_save, btn_save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, LV_SYMBOL_OK " Save");
    lv_obj_set_style_text_font(lbl_save, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_save, lv_color_white(), 0);
    lv_obj_center(lbl_save);

    lv_obj_t *btn_back = lv_btn_create(settings_screen);
    lv_obj_set_size(btn_back, 100, 36);
    lv_obj_align(btn_back, LV_ALIGN_TOP_MID, 0, 260);
    lv_obj_set_style_bg_color(btn_back, S_COLOR_BTN_NORM, 0);
    lv_obj_set_style_border_color(btn_back, S_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(btn_back, 1, 0);
    lv_obj_set_style_radius(btn_back, 8, 0);
    lv_obj_add_event_cb(btn_back, btn_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_back, S_COLOR_TEXT, 0);
    lv_obj_center(lbl_back);

    /* Load settings screen with animation */
    lv_scr_load_anim(settings_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);

    /* Reset sleep timer on entry */
    settings_touch_activity();
}

/* ── Sleep/Wake Logic ── */
void settings_touch_activity(void)
{
    s_last_activity_us = esp_timer_get_time();
    if (s_screen_sleeping) {
        s_screen_sleeping = false;
        Set_Backlight(s_brightness);
        ESP_LOGI(TAG, "Screen wake");
    }
}

void settings_sleep_check(void)
{
    if (s_sleep_timeout_s == 0) return;  /* never sleep */
    if (s_screen_sleeping) return;       /* already sleeping */

    int64_t now = esp_timer_get_time();
    int64_t elapsed_us = now - s_last_activity_us;
    int64_t timeout_us = (int64_t)s_sleep_timeout_s * 1000000LL;

    if (elapsed_us >= timeout_us) {
        s_screen_sleeping = true;
        Set_Backlight(0);
        ESP_LOGI(TAG, "Screen sleep (timeout %ds)", s_sleep_timeout_s);
    }
}

uint8_t settings_get_brightness(void)
{
    return s_brightness;
}
