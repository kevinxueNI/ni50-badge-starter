/**
 * ui.c — 主 UI 框架：Tileview + 页面调度 + 状态栏
 */
#include "ui.h"
#include "esp_log.h"

static const char *TAG = "ui";

lv_obj_t *g_tileview = NULL;
static lv_obj_t *tabbar = NULL;
static lv_obj_t *statusbar = NULL;

static void home_click_cb(lv_event_t *e)
{
    (void)e;
    app_router_go_home();
}

static void tab_click_cb(lv_event_t *e)
{
    page_id_t page = (page_id_t)(intptr_t)lv_event_get_user_data(e);
    app_router_go_tab(page, LV_ANIM_ON);
}

/* Tileview 页面切换回调 */
static void tileview_changed_cb(lv_event_t *e)
{
    lv_obj_t *tv = lv_event_get_target(e);
    lv_obj_t *active_tile = lv_tileview_get_tile_act(tv);
    lv_coord_t x = lv_obj_get_x(active_tile);
    int idx = x / SCREEN_W;
    sys_tabbar_set_active(tabbar, idx);
}

void ui_init(void)
{
    ESP_LOGI(TAG, "Initializing Badge UI (480x480, %d pages)...", PAGE_COUNT);

    lv_obj_t *main_screen = lv_obj_create(NULL);
    lv_obj_clear_flag(main_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* 屏幕深蓝底色 */
    lv_obj_set_style_bg_color(main_screen, COLOR_BG_DARKER, 0);
    lv_obj_set_style_bg_opa(main_screen, LV_OPA_COVER, 0);

    /* 创建水平 Tileview */
    g_tileview = lv_tileview_create(main_screen);
    lv_obj_set_size(g_tileview, SCREEN_W, SCREEN_H);
    lv_obj_align(g_tileview, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(g_tileview, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(g_tileview, LV_OBJ_FLAG_SCROLLABLE);

    /* 添加页面 tile */
    lv_obj_t *tile_card = lv_tileview_add_tile(g_tileview, 0, 0, LV_DIR_RIGHT);
    lv_obj_t *tile_milestones = lv_tileview_add_tile(g_tileview, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *tile_labview = lv_tileview_add_tile(g_tileview, 2, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *tile_calendar = lv_tileview_add_tile(g_tileview, 3, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *tile_config = lv_tileview_add_tile(g_tileview, 4, 0, LV_DIR_LEFT);

    /* 构建各页面内容 */
    page_card_create(tile_card);
    page_milestones_create(tile_milestones);
    page_labview_create(tile_labview);
    page_easter_egg_create(tile_calendar);
    page_config_create(tile_config);

    /* 底部 tab bar */
    tabbar = sys_tabbar_create(main_screen, tab_click_cb);
    sys_tabbar_set_active(tabbar, 0);

    /* 注册 tileview 切换事件 */
    lv_obj_add_event_cb(g_tileview, tileview_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 全局状态栏与 Home 按钮 */
    statusbar = widget_statusbar_create(lv_layer_sys());
    lv_obj_t *home_btn = widget_home_btn_create(lv_layer_top(), home_click_cb);
    app_router_register_main(main_screen, g_tileview, home_btn);
    ui_set_statusbar_visible(false);

    ESP_LOGI(TAG, "Badge UI ready. %d pages.", PAGE_COUNT);
}

void ui_set_statusbar_visible(bool visible)
{
    if (!statusbar) {
        return;
    }

    widget_statusbar_set_visible(visible);
}
