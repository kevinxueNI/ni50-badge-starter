/**
 * app_boot.c - NI-style boot intro screen for first-pass product experience
 */
#include "ui/ui.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "app_boot";

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *log_label;
    lv_obj_t *fade_overlay;
    lv_timer_t *line_timer;
    lv_timer_t *cursor_timer;
    lv_timer_t *finish_timer;
    lv_timer_t *transition_timer;
    size_t line_idx;
    bool cursor_on;
    char text_buf[2048];
} boot_state_t;

static boot_state_t s_boot;

static void boot_fade_opa_cb(void *obj, int32_t v)
{
    lv_obj_set_style_bg_opa((lv_obj_t *)obj, (lv_opa_t)v, 0);
}

static const char *s_boot_lines[] = {
    "NI PXI Chassis Firmware v19.76.20.26",
    "Copyright (C) 1976-2026 National Instruments",
    "",
    "> Initializing UI backplane .......... [ OK ]",
    "> Loading LabVIEW runtime ............ [ OK ]",
    "> Scanning PXI software modules ...... [ OK ]",
    "> Mounting badge profile ............. [ OK ]",
    "> Preparing NI timeline cache ........ [ OK ]",
    "> Calibrating touch plane ............ [ OK ]",
    "> Routing front panels ............... [ OK ]",
    "",
    "> System ready. Launching Badge UI...",
};

static void boot_refresh_text(void)
{
    char render_buf[2100];
    snprintf(render_buf, sizeof(render_buf), "%s%s", s_boot.text_buf, s_boot.cursor_on ? "_" : " ");
    lv_label_set_text(s_boot.log_label, render_buf);
}

static void boot_transition_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_boot.transition_timer) {
        lv_timer_del(s_boot.transition_timer);
        s_boot.transition_timer = NULL;
    }

    ui_set_statusbar_visible(true);
    app_router_show_main(LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    s_boot.screen = NULL;
    ESP_LOGI(TAG, "Boot intro finished");
}

static void boot_finish_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_boot.line_timer) {
        lv_timer_del(s_boot.line_timer);
        s_boot.line_timer = NULL;
    }
    if (s_boot.cursor_timer) {
        lv_timer_del(s_boot.cursor_timer);
        s_boot.cursor_timer = NULL;
    }
    if (s_boot.finish_timer) {
        lv_timer_del(s_boot.finish_timer);
        s_boot.finish_timer = NULL;
    }

    /* Fade-out overlay on boot screen */
    s_boot.fade_overlay = lv_obj_create(s_boot.screen);
    lv_obj_set_size(s_boot.fade_overlay, 480, 480);
    lv_obj_set_pos(s_boot.fade_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_boot.fade_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_boot.fade_overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_boot.fade_overlay, 0, 0);
    lv_obj_set_style_radius(s_boot.fade_overlay, 0, 0);
    lv_obj_clear_flag(s_boot.fade_overlay, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(s_boot.fade_overlay);

    /* Animate opacity from 0 → 255 over 600ms */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_boot.fade_overlay);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, 600);
    lv_anim_set_exec_cb(&a, boot_fade_opa_cb);
    lv_anim_start(&a);

    /* After fade-out completes, switch to main */
    s_boot.transition_timer = lv_timer_create(boot_transition_cb, 650, NULL);
}

static void boot_cursor_cb(lv_timer_t *timer)
{
    (void)timer;
    s_boot.cursor_on = !s_boot.cursor_on;
    boot_refresh_text();
}

static void boot_line_cb(lv_timer_t *timer)
{
    (void)timer;

    if (s_boot.line_idx >= (sizeof(s_boot_lines) / sizeof(s_boot_lines[0]))) {
        if (!s_boot.finish_timer) {
            s_boot.finish_timer = lv_timer_create(boot_finish_cb, 2000, NULL);
        }
        return;
    }

    const char *line = s_boot_lines[s_boot.line_idx++];
    size_t current_len = strlen(s_boot.text_buf);
    size_t remaining = sizeof(s_boot.text_buf) - current_len - 1;

    if (remaining > 0) {
        strncat(s_boot.text_buf, line, remaining);
        remaining = sizeof(s_boot.text_buf) - strlen(s_boot.text_buf) - 1;
        if (remaining > 0) {
            strncat(s_boot.text_buf, "\n", remaining);
        }
    }

    boot_refresh_text();

    if (s_boot.line_idx >= (sizeof(s_boot_lines) / sizeof(s_boot_lines[0])) && !s_boot.finish_timer) {
        s_boot.finish_timer = lv_timer_create(boot_finish_cb, 2000, NULL);
    }
}

void app_boot_start(void)
{
    memset(&s_boot, 0, sizeof(s_boot));

    ui_set_statusbar_visible(false);

    s_boot.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_boot.screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_boot.screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_boot.screen, 0, 0);
    lv_obj_set_style_radius(s_boot.screen, 0, 0);
    lv_obj_set_style_pad_all(s_boot.screen, 0, 0);
    lv_obj_clear_flag(s_boot.screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header = lv_label_create(s_boot.screen);
    lv_label_set_text(header, "MAX BOOT SEQUENCE");
    lv_obj_set_style_text_color(header, COLOR_NI_GREEN, 0);
    lv_obj_set_style_text_font(header, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_letter_space(header, 1, 0);
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, 14, 14);

    lv_obj_t *log_box = lv_obj_create(s_boot.screen);
    lv_obj_set_size(log_box, SCREEN_W - 28, SCREEN_H - 52);
    lv_obj_align(log_box, LV_ALIGN_TOP_LEFT, 14, 44);
    lv_obj_set_style_bg_color(log_box, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(log_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(log_box, 0, 0);
    lv_obj_set_style_radius(log_box, 0, 0);
    lv_obj_set_style_pad_all(log_box, 0, 0);
    lv_obj_clear_flag(log_box, LV_OBJ_FLAG_SCROLLABLE);

    s_boot.log_label = lv_label_create(log_box);
    lv_obj_set_width(s_boot.log_label, SCREEN_W - 28);
    lv_label_set_long_mode(s_boot.log_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_boot.log_label, COLOR_NI_GREEN, 0);
    lv_obj_set_style_text_font(s_boot.log_label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_line_space(s_boot.log_label, 4, 0);
    lv_obj_align(s_boot.log_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_text(s_boot.log_label, "");

    s_boot.cursor_on = true;
    boot_refresh_text();

    s_boot.cursor_timer = lv_timer_create(boot_cursor_cb, 240, NULL);
    s_boot.line_timer = lv_timer_create(boot_line_cb, 140, NULL);

    lv_scr_load(s_boot.screen);
    ESP_LOGI(TAG, "Boot intro started");
}