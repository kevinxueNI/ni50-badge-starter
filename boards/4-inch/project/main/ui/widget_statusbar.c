/**
 * widget_statusbar.c — 顶部状态栏 (32px 高, 半透明黑底)
 * 显示：时间 | 电池图标 + 百分比
 * 时间源：svc_rtc 统一时间服务（优先硬件 RTC，失败则软件回退）
 */
#include "ui.h"
#include "svc_rtc.h"
#include "esp_log.h"
#include <stdio.h>

#define STATUSBAR_H  32

static lv_obj_t *statusbar = NULL;
static lv_obj_t *lbl_time    = NULL;
static lv_obj_t *lbl_battery = NULL;
static lv_obj_t *lbl_bat_pct = NULL;
static int battery_pct = 85;

static void statusbar_refresh_time_label(void)
{
    svc_rtc_time_t now = {0};
    char buf[8];

    if (svc_rtc_get_now(&now) != ESP_OK) {
        lv_label_set_text(lbl_time, "--:--");
        return;
    }

    snprintf(buf, sizeof(buf), "%02u:%02u", now.hour, now.minute);
    lv_label_set_text(lbl_time, buf);
}

void widget_statusbar_set_time(int hour, int min)
{
    svc_rtc_time_t now = {0};
    if (svc_rtc_get_now(&now) != ESP_OK) {
        return;
    }

    now.hour = (uint8_t)hour;
    now.minute = (uint8_t)min;
    now.second = 0;
    if (svc_rtc_set_now(&now) == ESP_OK) {
        statusbar_refresh_time_label();
    }
}

int widget_statusbar_get_hour(void)
{
    svc_rtc_time_t now = {0};
    return svc_rtc_get_now(&now) == ESP_OK ? now.hour : 0;
}

int widget_statusbar_get_min(void)
{
    svc_rtc_time_t now = {0};
    return svc_rtc_get_now(&now) == ESP_OK ? now.minute : 0;
}

void widget_statusbar_set_visible(bool visible)
{
    if (!statusbar) {
        return;
    }

    if (visible) {
        lv_obj_clear_flag(statusbar, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(statusbar, LV_OBJ_FLAG_HIDDEN);
    }
}

/* 定时刷新状态栏 (每秒) */
static void statusbar_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    statusbar_refresh_time_label();
}

void widget_statusbar_set_battery(int percent)
{
    battery_pct = percent;
    if (percent > 80)       lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_FULL);
    else if (percent > 60)  lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_3);
    else if (percent > 40)  lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_2);
    else if (percent > 20)  lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_1);
    else                    lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_EMPTY);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    lv_label_set_text(lbl_bat_pct, buf);

    /* 低电量红色警告 */
    if (percent <= 20) {
        lv_obj_set_style_text_color(lbl_battery, lv_color_hex(0xE74C3C), 0);
        lv_obj_set_style_text_color(lbl_bat_pct, lv_color_hex(0xE74C3C), 0);
    } else {
        lv_obj_set_style_text_color(lbl_battery, COLOR_NI_GREEN, 0);
        lv_obj_set_style_text_color(lbl_bat_pct, COLOR_TEXT_DIM, 0);
    }
}

lv_obj_t *widget_statusbar_create(lv_obj_t *parent)
{
    svc_rtc_init();

    statusbar = lv_obj_create(parent);
    lv_obj_set_size(statusbar, SCREEN_W, STATUSBAR_H);
    lv_obj_align(statusbar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(statusbar, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(statusbar, LV_OPA_60, 0);
    lv_obj_set_style_border_width(statusbar, 0, 0);
    lv_obj_set_style_radius(statusbar, 0, 0);
    lv_obj_set_style_pad_all(statusbar, 0, 0);
    lv_obj_set_style_shadow_width(statusbar, 0, 0);
    lv_obj_clear_flag(statusbar, LV_OBJ_FLAG_SCROLLABLE);

    /* 时间 (左侧) */
    lbl_time = lv_label_create(statusbar);
    lv_label_set_text(lbl_time, "--:--");
    lv_obj_set_style_text_color(lbl_time, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_time, LV_ALIGN_LEFT_MID, 12, 0);

    /* 电池百分比 (右侧数字) */
    lbl_bat_pct = lv_label_create(statusbar);
    lv_label_set_text(lbl_bat_pct, "85%");
    lv_obj_set_style_text_color(lbl_bat_pct, COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(lbl_bat_pct, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_bat_pct, LV_ALIGN_RIGHT_MID, -12, 0);

    /* 电池图标 (百分比左侧) */
    lbl_battery = lv_label_create(statusbar);
    lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(lbl_battery, COLOR_NI_GREEN, 0);
    lv_obj_set_style_text_font(lbl_battery, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_battery, LV_ALIGN_RIGHT_MID, -52, 0);

    /* 1 秒定时器 */
    lv_timer_create(statusbar_timer_cb, 1000, NULL);
    statusbar_refresh_time_label();
    widget_statusbar_set_battery(battery_pct);

    return statusbar;
}
