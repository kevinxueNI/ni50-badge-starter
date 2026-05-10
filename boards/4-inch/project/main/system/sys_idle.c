#include "sys_idle.h"

#include "bsp/esp32_s3_touch_lcd_4.h"
#include "esp_log.h"
#include "lvgl.h"

#define SYS_IDLE_DEFAULT_TIMEOUT_MS 120000U
#define SYS_IDLE_FADE_MS          320U
#define SYS_IDLE_POLL_MS          120U
#define SYS_IDLE_DEFAULT_BRIGHTNESS 80U

typedef enum {
    SYS_IDLE_STATE_ACTIVE = 0,
    SYS_IDLE_STATE_FADING,
    SYS_IDLE_STATE_SLEEPING,
} sys_idle_state_t;

static const char *TAG = "sys_idle";

static struct {
    lv_obj_t *wake_overlay;
    lv_timer_t *poll_timer;
    uint32_t fade_start_ms;
    uint32_t timeout_ms;
    uint8_t user_brightness;
    sys_idle_state_t state;
    bool initialized;
} s_idle;

static void sys_idle_apply_brightness(uint8_t percent)
{
    bsp_display_brightness_set(percent);
}

static void sys_idle_wake(void)
{
    sys_idle_apply_brightness(s_idle.user_brightness);
    lv_obj_add_flag(s_idle.wake_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_opa(s_idle.wake_overlay, LV_OPA_TRANSP, 0);
    s_idle.state = SYS_IDLE_STATE_ACTIVE;
    lv_disp_trig_activity(NULL);
    ESP_LOGI(TAG, "Wake display, brightness=%u%%", (unsigned)s_idle.user_brightness);
}

static void wake_overlay_cb(lv_event_t *e)
{
    (void)e;

    if (s_idle.state != SYS_IDLE_STATE_ACTIVE) {
        sys_idle_wake();
    }
}

static void sys_idle_sleep(void)
{
    lv_obj_set_style_bg_opa(s_idle.wake_overlay, LV_OPA_COVER, 0);
    sys_idle_apply_brightness(0);
    s_idle.state = SYS_IDLE_STATE_SLEEPING;
    ESP_LOGI(TAG, "Display idle sleep entered");
}

static void sys_idle_poll_cb(lv_timer_t *timer)
{
    uint32_t inactive_ms;
    uint32_t fade_elapsed_ms;
    uint32_t fade_opa;

    (void)timer;

    if (!s_idle.initialized || !s_idle.wake_overlay) {
        return;
    }

    inactive_ms = lv_disp_get_inactive_time(NULL);

    switch (s_idle.state) {
    case SYS_IDLE_STATE_ACTIVE:
        if (s_idle.timeout_ms > 0 && inactive_ms >= s_idle.timeout_ms) {
            s_idle.state = SYS_IDLE_STATE_FADING;
            s_idle.fade_start_ms = lv_tick_get();
            lv_obj_clear_flag(s_idle.wake_overlay, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_opa(s_idle.wake_overlay, LV_OPA_TRANSP, 0);
            lv_obj_move_foreground(s_idle.wake_overlay);
            ESP_LOGI(TAG, "Display idle fade started");
        }
        break;
    case SYS_IDLE_STATE_FADING:
        if (inactive_ms < s_idle.timeout_ms) {
            sys_idle_wake();
            break;
        }

        fade_elapsed_ms = lv_tick_elaps(s_idle.fade_start_ms);
        if (fade_elapsed_ms >= SYS_IDLE_FADE_MS) {
            sys_idle_sleep();
            break;
        }

        fade_opa = (fade_elapsed_ms * LV_OPA_COVER) / SYS_IDLE_FADE_MS;
        lv_obj_set_style_bg_opa(s_idle.wake_overlay, (lv_opa_t)fade_opa, 0);
        break;
    case SYS_IDLE_STATE_SLEEPING:
        if (s_idle.timeout_ms > 0 && inactive_ms < s_idle.timeout_ms) {
            sys_idle_wake();
        }
        break;
    }
}

void sys_idle_init(void)
{
    if (s_idle.initialized) {
        return;
    }

    s_idle.user_brightness = SYS_IDLE_DEFAULT_BRIGHTNESS;
    s_idle.timeout_ms = SYS_IDLE_DEFAULT_TIMEOUT_MS;
    s_idle.state = SYS_IDLE_STATE_ACTIVE;

    s_idle.wake_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_idle.wake_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(s_idle.wake_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_idle.wake_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_idle.wake_overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_idle.wake_overlay, 0, 0);
    lv_obj_set_style_radius(s_idle.wake_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_idle.wake_overlay, 0, 0);
    lv_obj_set_style_shadow_width(s_idle.wake_overlay, 0, 0);
    lv_obj_clear_flag(s_idle.wake_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_idle.wake_overlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_idle.wake_overlay, wake_overlay_cb, LV_EVENT_PRESSED, NULL);

    s_idle.poll_timer = lv_timer_create(sys_idle_poll_cb, SYS_IDLE_POLL_MS, NULL);
    s_idle.initialized = true;

    sys_idle_apply_brightness(s_idle.user_brightness);
    lv_disp_trig_activity(NULL);
    ESP_LOGI(TAG, "Idle manager ready (timeout=%ums, brightness=%u%%)",
             (unsigned)s_idle.timeout_ms, (unsigned)s_idle.user_brightness);
}

void sys_idle_set_user_brightness(uint8_t percent)
{
    if (percent > 100U) {
        percent = 100U;
    }

    s_idle.user_brightness = percent;
    if (s_idle.state != SYS_IDLE_STATE_SLEEPING) {
        sys_idle_apply_brightness(percent);
    }
}

uint8_t sys_idle_get_user_brightness(void)
{
    return s_idle.user_brightness;
}

void sys_idle_set_timeout(uint32_t timeout_ms)
{
    s_idle.timeout_ms = timeout_ms;

    /* If switching to "never" while fading/sleeping, wake immediately */
    if (timeout_ms == 0 && s_idle.state != SYS_IDLE_STATE_ACTIVE) {
        sys_idle_wake();
    }

    ESP_LOGI(TAG, "Screen-off timeout set to %ums", (unsigned)timeout_ms);
}

uint32_t sys_idle_get_timeout(void)
{
    return s_idle.timeout_ms;
}

void sys_idle_mark_activity(void)
{
    if (!s_idle.initialized) {
        return;
    }

    if (s_idle.state != SYS_IDLE_STATE_ACTIVE) {
        sys_idle_wake();
        return;
    }

    lv_disp_trig_activity(NULL);
}

bool sys_idle_is_sleeping(void)
{
    return s_idle.state == SYS_IDLE_STATE_SLEEPING;
}