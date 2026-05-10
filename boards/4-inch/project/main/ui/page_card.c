/**
 * page_card.c — 名片主页 (480×480)
 *
 * Layout: upper-left avatar + upper-right profile,
 *         lower-left WeChat QR + lower-right NI5040 logo
 * Breathing ring animation on avatar
 * Easter egg: long-press NI5040 logo → NI Logo History
 */
#include "ui.h"
#include "esp_log.h"
#include "esp_random.h"
#include "svc_profile.h"

static const char *TAG = "page_card";

/* Image assets */
extern const lv_img_dsc_t img_avatar_160;
extern const lv_img_dsc_t img_ni5040_logo;
extern const lv_img_dsc_t img_bg_pattern;
extern const lv_img_dsc_t img_wechat_qr;

/* Long-press entry: NI Logo History */
extern void logo_history_show(void);

/* 呼吸动画控件引用 */
static lv_obj_t *avatar_ring = NULL;
static lv_timer_t *s_easter_timer = NULL;
static lv_obj_t *s_glitch_overlay = NULL;
static lv_timer_t *s_glitch_timer = NULL;
#define GLITCH_BAR_COUNT 5
static lv_obj_t *s_glitch_bars[GLITCH_BAR_COUNT];

/* Forward declarations */
static void glitch_remove(void);

static void easter_timer_cancel(void)
{
    if (!s_easter_timer) {
        return;
    }

    lv_timer_del(s_easter_timer);
    s_easter_timer = NULL;
}

static void easter_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_easter_timer = NULL;
    glitch_remove();
    ESP_LOGI(TAG, "Logo history triggered");
    logo_history_show();
    /* Force synchronous render to flush frame buffer immediately */
    lv_refr_now(NULL);
}

/* ── 头像环呼吸动画 ── */
static void ring_breath_cb(void *var, int32_t v)
{
    lv_obj_set_style_bg_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

/* ── Glitch effect helpers ── */
static void glitch_remove(void)
{
    if (s_glitch_timer) { lv_timer_del(s_glitch_timer); s_glitch_timer = NULL; }
    if (s_glitch_overlay) { lv_obj_del(s_glitch_overlay); s_glitch_overlay = NULL; }
    for (int i = 0; i < GLITCH_BAR_COUNT; i++) s_glitch_bars[i] = NULL;
}

static void glitch_tick_cb(lv_timer_t *t)
{
    lv_obj_t *parent = (lv_obj_t *)t->user_data;
    if (!s_glitch_overlay) return;
    lv_coord_t pw = lv_obj_get_width(parent);
    lv_coord_t ph = lv_obj_get_height(parent);
    if (ph < 12) return;
    for (int i = 0; i < GLITCH_BAR_COUNT; i++) {
        if (!s_glitch_bars[i]) continue;
        int h = 2 + (esp_random() % 6);
        int y = esp_random() % (ph - h);
        uint32_t colors[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF, 0x00FFFF};
        lv_obj_set_size(s_glitch_bars[i], pw, h);
        lv_obj_set_pos(s_glitch_bars[i], 0, y);
        lv_obj_set_style_bg_color(s_glitch_bars[i], lv_color_hex(colors[esp_random() % 6]), 0);
        lv_obj_set_style_bg_opa(s_glitch_bars[i], LV_OPA_40 + (esp_random() % 40), 0);
    }
}

static void glitch_start(lv_obj_t *target)
{
    if (s_glitch_overlay) return;
    s_glitch_overlay = lv_obj_create(target);
    lv_obj_set_size(s_glitch_overlay, lv_obj_get_width(target), lv_obj_get_height(target));
    lv_obj_set_pos(s_glitch_overlay, 0, 0);
    lv_obj_set_style_bg_opa(s_glitch_overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_glitch_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_glitch_overlay, 0, 0);
    lv_obj_set_style_shadow_width(s_glitch_overlay, 0, 0);
    lv_obj_clear_flag(s_glitch_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s_glitch_overlay, LV_OBJ_FLAG_CLICKABLE);
    /* Pre-create fixed bars */
    for (int i = 0; i < GLITCH_BAR_COUNT; i++) {
        s_glitch_bars[i] = lv_obj_create(s_glitch_overlay);
        lv_obj_set_style_border_width(s_glitch_bars[i], 0, 0);
        lv_obj_set_style_shadow_width(s_glitch_bars[i], 0, 0);
        lv_obj_set_style_radius(s_glitch_bars[i], 0, 0);
        lv_obj_set_style_pad_all(s_glitch_bars[i], 0, 0);
        lv_obj_clear_flag(s_glitch_bars[i], LV_OBJ_FLAG_SCROLLABLE);
    }
    s_glitch_timer = lv_timer_create(glitch_tick_cb, 150, target);
}

/* ── 长按 NI5040 Logo → Easter Egg ── */
static void logo_press_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *logo_container = (lv_obj_t *)lv_event_get_user_data(e);

    switch (code) {
    case LV_EVENT_PRESSED:
        easter_timer_cancel();
        s_easter_timer = lv_timer_create(easter_timer_cb, 800, NULL);
        lv_timer_set_repeat_count(s_easter_timer, 1);
        glitch_start(logo_container);
        break;
    case LV_EVENT_RELEASED:
    case LV_EVENT_PRESS_LOST:
    case LV_EVENT_DELETE:
        easter_timer_cancel();
        glitch_remove();
        break;
    default:
        break;
    }
}

static void start_ring_breath(lv_obj_t *ring)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ring);
    lv_anim_set_values(&a, LV_OPA_20, LV_OPA_COVER);
    lv_anim_set_time(&a, 2000);
    lv_anim_set_playback_time(&a, 2000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, ring_breath_cb);
    lv_anim_start(&a);
}

lv_obj_t *page_card_create(lv_obj_t *parent)
{
    badge_profile_t profile = {0};

    if (svc_profile_get(&profile) != ESP_OK) {
        ESP_LOGW(TAG, "Profile service unavailable, falling back to defaults");
        snprintf(profile.name, sizeof(profile.name), "%s", "Kevin XUE");
        snprintf(profile.dept, sizeof(profile.dept), "%s", "China Innovation Center");
    }

    /* Dark background fallback */
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Background image */
    lv_obj_t *bg_img = lv_img_create(parent);
    lv_img_set_src(bg_img, &img_bg_pattern);
    lv_obj_align(bg_img, LV_ALIGN_TOP_LEFT, 0, 0);

    /* ── Upper-left: Avatar with breathing ring ── */
    avatar_ring = lv_obj_create(parent);
    lv_obj_set_size(avatar_ring, 172, 172);
    lv_obj_set_pos(avatar_ring, 24, 36);
    lv_obj_set_style_radius(avatar_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(avatar_ring, COLOR_ACCENT_YELLOW, 0);
    lv_obj_set_style_bg_opa(avatar_ring, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(avatar_ring, 0, 0);
    lv_obj_set_style_pad_all(avatar_ring, 6, 0);
    lv_obj_set_style_shadow_width(avatar_ring, 0, 0);
    lv_obj_set_style_clip_corner(avatar_ring, true, 0);
    lv_obj_clear_flag(avatar_ring, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *avatar_img = lv_img_create(avatar_ring);
    lv_img_set_src(avatar_img, &img_avatar_160);
    lv_obj_center(avatar_img);

    start_ring_breath(avatar_ring);

    /* ── Upper-right: semi-transparent backdrop for profile text ── */
    lv_obj_t *profile_bg = lv_obj_create(parent);
    lv_obj_set_size(profile_bg, 252, 170);
    lv_obj_set_pos(profile_bg, 204, 37);
    lv_obj_set_style_bg_color(profile_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(profile_bg, LV_OPA_50, 0);
    lv_obj_set_style_border_width(profile_bg, 0, 0);
    lv_obj_set_style_radius(profile_bg, 12, 0);
    lv_obj_set_style_shadow_width(profile_bg, 0, 0);
    lv_obj_clear_flag(profile_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(profile_bg, LV_OBJ_FLAG_CLICKABLE);

    /* ── Upper-right: Profile text ── */
    lv_obj_t *name_lbl = lv_label_create(parent);
    lv_label_set_text(name_lbl, profile.name);
    lv_obj_set_style_text_color(name_lbl, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_letter_space(name_lbl, 2, 0);
    lv_obj_set_pos(name_lbl, 214, 40);

    lv_obj_t *title_lbl = lv_label_create(parent);
    lv_label_set_text(title_lbl, "Sr. Agile Engineering Mgr");
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(title_lbl, 214, 78);

    lv_obj_t *divider = lv_obj_create(parent);
    lv_obj_set_size(divider, 236, 2);
    lv_obj_set_pos(divider, 214, 104);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_set_style_shadow_width(divider, 0, 0);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *age_lbl = lv_label_create(parent);
    lv_label_set_text(age_lbl, "NI Age: 5 Years");
    lv_obj_set_style_text_color(age_lbl, COLOR_ACCENT_YELLOW, 0);
    lv_obj_set_style_text_font(age_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(age_lbl, 214, 116);

    lv_obj_t *dept_lbl = lv_label_create(parent);
    lv_label_set_text(dept_lbl, profile.dept);
    lv_obj_set_style_text_color(dept_lbl, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(dept_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(dept_lbl, 214, 144);

    lv_obj_t *email_lbl = lv_label_create(parent);
    lv_label_set_text(email_lbl, "kevin.xue@emerson.com");
    lv_obj_set_style_text_color(email_lbl, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(email_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(email_lbl, 214, 172);

    /* ── Section divider ── */
    lv_obj_t *h_div = lv_obj_create(parent);
    lv_obj_set_size(h_div, 420, 1);
    lv_obj_align(h_div, LV_ALIGN_TOP_MID, 0, 228);
    lv_obj_set_style_bg_color(h_div, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_bg_opa(h_div, LV_OPA_40, 0);
    lv_obj_set_style_border_width(h_div, 0, 0);
    lv_obj_set_style_shadow_width(h_div, 0, 0);
    lv_obj_clear_flag(h_div, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Lower-left: WeChat QR ── */
    lv_obj_t *qr_cap = lv_label_create(parent);
    lv_label_set_text(qr_cap, "WeChat");
    lv_obj_set_style_text_color(qr_cap, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(qr_cap, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(qr_cap, 82, 240);

    lv_obj_t *qr_img = lv_img_create(parent);
    lv_img_set_src(qr_img, &img_wechat_qr);
    lv_obj_set_pos(qr_img, 43, 262);

    /* ── Lower-right: NI5040 Logo in container (easter egg trigger) ── */
    lv_obj_t *logo_cont = lv_obj_create(parent);
    lv_obj_set_size(logo_cont, 248, 139);
    lv_obj_set_pos(logo_cont, 210, 262);
    lv_obj_set_style_bg_opa(logo_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(logo_cont, 0, 0);
    lv_obj_set_style_pad_all(logo_cont, 0, 0);
    lv_obj_set_style_radius(logo_cont, 0, 0);
    lv_obj_set_style_shadow_width(logo_cont, 0, 0);
    lv_obj_set_style_clip_corner(logo_cont, true, 0);
    lv_obj_clear_flag(logo_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(logo_cont, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(logo_cont, logo_press_cb, LV_EVENT_PRESSED, logo_cont);
    lv_obj_add_event_cb(logo_cont, logo_press_cb, LV_EVENT_RELEASED, logo_cont);
    lv_obj_add_event_cb(logo_cont, logo_press_cb, LV_EVENT_PRESS_LOST, logo_cont);

    lv_obj_t *logo_img = lv_img_create(logo_cont);
    lv_img_set_src(logo_img, &img_ni5040_logo);
    lv_obj_center(logo_img);

    ESP_LOGI(TAG, "Card page created (new layout + breathing anim)");
    return parent;
}
