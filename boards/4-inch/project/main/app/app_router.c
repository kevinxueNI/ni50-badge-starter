/**
 * app_router.c - minimal hybrid router for tab-first navigation
 */
#include "ui/ui.h"
#include "esp_log.h"

static const char *TAG = "app_router";

static lv_obj_t *s_main_screen = NULL;
static lv_obj_t *s_tileview = NULL;
static lv_obj_t *s_home_btn = NULL;
static lv_obj_t *s_current_immersive = NULL;

void app_router_register_main(lv_obj_t *main_screen, lv_obj_t *tileview, lv_obj_t *home_btn)
{
    s_main_screen = main_screen;
    s_tileview = tileview;
    s_home_btn = home_btn;

    if (s_home_btn) {
        lv_obj_add_flag(s_home_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

bool app_router_is_immersive_active(void)
{
    return s_main_screen && lv_scr_act() != s_main_screen;
}

void app_router_set_home_visible(bool visible)
{
    if (!s_home_btn) {
        return;
    }

    if (visible) {
        lv_obj_clear_flag(s_home_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_home_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

void app_router_go_tab(page_id_t page, lv_anim_enable_t anim_en)
{
    if (!s_main_screen || !s_tileview) {
        return;
    }

    if (lv_scr_act() != s_main_screen) {
        lv_scr_load(s_main_screen);
    }

    s_current_immersive = NULL;
    app_router_set_home_visible(false);
    lv_obj_set_tile_id(s_tileview, (uint32_t)page, 0, anim_en);
    ESP_LOGI(TAG, "Go tab: %d", (int)page);
}

void app_router_show_main(lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool auto_del)
{
    if (!s_main_screen) {
        return;
    }

    s_current_immersive = NULL;
    app_router_set_home_visible(false);
    lv_obj_set_tile_id(s_tileview, PAGE_CARD, 0, LV_ANIM_OFF);
    lv_scr_load_anim(s_main_screen, anim_type, time, delay, auto_del);
    ESP_LOGI(TAG, "Show main screen");
}

void app_router_enter_immersive(lv_obj_t *screen, lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool auto_del)
{
    if (!screen) {
        return;
    }

    s_current_immersive = screen;
    app_router_set_home_visible(true);
    lv_scr_load_anim(screen, anim_type, time, delay, auto_del);
    ESP_LOGI(TAG, "Enter immersive screen");
}

void app_router_go_home(void)
{
    if (!s_main_screen) {
        return;
    }

    lv_obj_t *active = lv_scr_act();

    /* Already home — nothing to do */
    if (active == s_main_screen && !s_current_immersive) {
        return;
    }

    lv_obj_t *to_delete = NULL;
    if (s_current_immersive && s_current_immersive != s_main_screen) {
        to_delete = s_current_immersive;
    }

    s_current_immersive = NULL;
    app_router_set_home_visible(false);

    if (active != s_main_screen) {
        /* Use immediate load — no animation avoids dual-screen render WDT */
        lv_scr_load(s_main_screen);
    }

    if (to_delete) {
        lv_obj_del_async(to_delete);
    }

    if (s_tileview) {
        lv_obj_set_tile_id(s_tileview, PAGE_CARD, 0, LV_ANIM_OFF);
    }

    ESP_LOGI(TAG, "Go home");
}