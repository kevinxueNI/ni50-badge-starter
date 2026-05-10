/**
 * page_gallery.c — LabVIEW landing page + first-round simulator
 */
#include "ui.h"
#include "esp_log.h"

static const char *TAG = "labview";

extern const lv_img_dsc_t img_lv_fp_broken;
extern const lv_img_dsc_t img_lv_fp_runnable;
extern const lv_img_dsc_t img_lv_bd_unwired;
extern const lv_img_dsc_t img_lv_bd_wired;
extern const lv_img_dsc_t img_lv_preview;

#define LAB_BG          lv_color_hex(0x0D1117)
#define LAB_PANEL       lv_color_hex(0x161B22)
#define LAB_PANEL_ALT   lv_color_hex(0x1B2230)
#define LAB_BORDER      lv_color_hex(0x30363D)
#define LAB_MUTED       lv_color_hex(0x8B949E)
#define LAB_GREEN       lv_color_hex(0x00A651)
#define LAB_RED         lv_color_hex(0xDA3633)
#define LAB_BLUE        lv_color_hex(0x58A6FF)
#define LAB_YELLOW      lv_color_hex(0xF1C40F)
#define LAB_VIEW_W      432
#define LAB_VIEW_H      286
#define WAVE_PT_COUNT   20

/* Sine lookup table: amplitude ±28 from center, 20 points = 2 full cycles */
static const int8_t sine_lut[WAVE_PT_COUNT] = {
    0, 16, 27, 28, 16, 0, -16, -27, -28, -16,
    0, 16, 27, 28, 16, 0, -16, -27, -28, -16
};

typedef enum {
    SIM_FRONT_BROKEN = 0,
    SIM_BLOCK_UNWIRED,
    SIM_BLOCK_WIRED,
    SIM_FRONT_RUNNABLE,
    SIM_RUNNING,
} sim_state_t;

static struct {
    lv_obj_t *screen;
    lv_obj_t *viewport;
    lv_obj_t *image;
    lv_obj_t *state_badge;
    lv_obj_t *state_label;
    lv_obj_t *instruction;
    lv_obj_t *toggle_btn;
    lv_obj_t *toggle_label;
    lv_obj_t *run_btn;
    lv_obj_t *run_label;
    lv_obj_t *source_hotspot;
    lv_obj_t *target_hotspot;
    lv_obj_t *wire_line;
    lv_obj_t *wave_box;
    lv_obj_t *wave_line;
    lv_obj_t *wave_dot;
    lv_point_t wave_pts[WAVE_PT_COUNT];
    lv_timer_t *wave_timer;
    lv_point_t wire_points[3];
    sim_state_t state;
    bool source_selected;
    uint8_t wave_phase;
} sim;

static void simulator_refresh(void);

static void wave_stop(void)
{
    if (sim.wave_timer) {
        lv_timer_del(sim.wave_timer);
        sim.wave_timer = NULL;
    }
}

static void wave_timer_cb(lv_timer_t *t)
{
    (void)t;
    sim.wave_phase = (sim.wave_phase + 1) % WAVE_PT_COUNT;
    lv_obj_set_pos(sim.wave_dot,
                   sim.wave_pts[sim.wave_phase].x - 5,
                   sim.wave_pts[sim.wave_phase].y - 5);
}

static void simulator_screen_del_cb(lv_event_t *e)
{
    (void)e;
    wave_stop();
    sim.screen = NULL;
}

static void simulator_close_cb(lv_event_t *e)
{
    (void)e;
    wave_stop();
    lv_obj_t *screen = sim.screen;
    sim.screen = NULL;
    app_router_go_tab(PAGE_LABVIEW, LV_ANIM_OFF);
    if (screen) {
        lv_obj_del_async(screen);
    }
}

static void simulator_toggle_cb(lv_event_t *e)
{
    (void)e;

    switch (sim.state) {
    case SIM_FRONT_BROKEN:
        sim.state = SIM_BLOCK_UNWIRED;
        break;
    case SIM_BLOCK_UNWIRED:
        sim.state = SIM_FRONT_BROKEN;
        sim.source_selected = false;
        break;
    case SIM_BLOCK_WIRED:
        sim.state = SIM_FRONT_RUNNABLE;
        break;
    case SIM_FRONT_RUNNABLE:
    case SIM_RUNNING:
        sim.state = SIM_BLOCK_WIRED;
        break;
    }

    simulator_refresh();
}

static void simulator_run_cb(lv_event_t *e)
{
    (void)e;

    if (sim.state == SIM_FRONT_RUNNABLE || sim.state == SIM_RUNNING) {
        sim.state = SIM_RUNNING;
        simulator_refresh();
    }
}

static void simulator_source_cb(lv_event_t *e)
{
    (void)e;

    if (sim.state == SIM_BLOCK_UNWIRED) {
        sim.source_selected = true;
        simulator_refresh();
    }
}

static void simulator_target_cb(lv_event_t *e)
{
    (void)e;

    if (sim.state == SIM_BLOCK_UNWIRED && sim.source_selected) {
        sim.state = SIM_BLOCK_WIRED;
        sim.source_selected = false;
        simulator_refresh();
    }
}

static void hotspot_style(lv_obj_t *obj, lv_color_t color)
{
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_20, 0);
    lv_obj_set_style_border_color(obj, color, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_radius(obj, 8, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void simulator_refresh(void)
{
    const lv_img_dsc_t *image = &img_lv_fp_broken;
    lv_color_t state_color = LAB_YELLOW;
    const char *state_text = "Front Panel: Broken";
    const char *instruction = "Tap Show Diagram to inspect the broken dataflow path.";
    const char *toggle_text = "Show Diagram";
    bool show_toggle = true;
    bool show_run = false;
    bool show_source = false;
    bool show_target = false;
    bool show_wire = false;
    bool show_wave = false;

    switch (sim.state) {
    case SIM_FRONT_BROKEN:
        image = &img_lv_fp_broken;
        state_color = LAB_RED;
        state_text = "Front Panel: Broken";
        instruction = "Tap Show Diagram to inspect the broken dataflow path.";
        toggle_text = "Show Diagram";
        break;
    case SIM_BLOCK_UNWIRED:
        image = &img_lv_bd_unwired;
        state_color = LAB_BLUE;
        state_text = "Block Diagram: Unwired";
        instruction = sim.source_selected
                          ? "Source selected. Now tap the sink to complete the wire."
                          : "Tap the source, then tap the sink to complete the wiring.";
        toggle_text = "Front Panel";
        show_source = true;
        show_target = true;
        show_wire = sim.source_selected;
        break;
    case SIM_BLOCK_WIRED:
        image = &img_lv_bd_wired;
        state_color = LAB_GREEN;
        state_text = "Block Diagram: Wired";
        instruction = "Wiring complete. Return to the front panel to enable Run.";
        toggle_text = "Front Panel";
        show_wire = true;
        break;
    case SIM_FRONT_RUNNABLE:
        image = &img_lv_fp_runnable;
        state_color = LAB_GREEN;
        state_text = "Front Panel: Runnable";
        instruction = "The VI is wired. Tap Run to start the chart animation.";
        toggle_text = "Block Diagram";
        show_run = true;
        break;
    case SIM_RUNNING:
        image = &img_lv_fp_runnable;
        state_color = LAB_GREEN;
        state_text = "Front Panel: Running";
        instruction = "Waveform is active. Switch back to the block diagram or rerun as needed.";
        toggle_text = "Block Diagram";
        show_run = true;
        show_wave = true;
        sim.wave_phase = 0;
        break;
    }

    lv_img_set_src(sim.image, image);
    lv_obj_set_style_bg_color(sim.state_badge, state_color, 0);
    lv_label_set_text(sim.state_label, state_text);
    lv_label_set_text(sim.instruction, instruction);
    lv_label_set_text(sim.toggle_label, toggle_text);
    lv_label_set_text(sim.run_label, sim.state == SIM_RUNNING ? "Running" : "Run");

    if (show_toggle) {
        lv_obj_clear_flag(sim.toggle_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(sim.toggle_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (show_run) {
        lv_obj_clear_flag(sim.run_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(sim.run_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (show_source) {
        lv_obj_clear_flag(sim.source_hotspot, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(sim.source_hotspot, LV_OBJ_FLAG_HIDDEN);
    }
    if (show_target) {
        lv_obj_clear_flag(sim.target_hotspot, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(sim.target_hotspot, LV_OBJ_FLAG_HIDDEN);
    }
    if (show_wave) {
        lv_obj_clear_flag(sim.wave_box, LV_OBJ_FLAG_HIDDEN);
        /* Position dot at initial point and start animation timer */
        lv_obj_set_pos(sim.wave_dot,
                       sim.wave_pts[sim.wave_phase].x - 5,
                       sim.wave_pts[sim.wave_phase].y - 5);
        if (!sim.wave_timer) {
            sim.wave_timer = lv_timer_create(wave_timer_cb, 80, NULL);
        }
    } else {
        lv_obj_add_flag(sim.wave_box, LV_OBJ_FLAG_HIDDEN);
        wave_stop();
    }

    if (sim.source_selected && sim.state == SIM_BLOCK_UNWIRED) {
        sim.wire_points[0].x = 128;
        sim.wire_points[0].y = 118;
        sim.wire_points[1].x = 196;
        sim.wire_points[1].y = 136;
        sim.wire_points[2].x = 240;
        sim.wire_points[2].y = 146;
    } else {
        sim.wire_points[0].x = 128;
        sim.wire_points[0].y = 118;
        sim.wire_points[1].x = 198;
        sim.wire_points[1].y = 138;
        sim.wire_points[2].x = 286;
        sim.wire_points[2].y = 152;
    }
    lv_line_set_points(sim.wire_line, sim.wire_points, 3);

    if (show_wire) {
        lv_obj_clear_flag(sim.wire_line, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(sim.wire_line, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_set_style_border_color(sim.source_hotspot, sim.source_selected ? LAB_YELLOW : LAB_BLUE, 0);
    lv_obj_set_style_bg_color(sim.source_hotspot, sim.source_selected ? LAB_YELLOW : LAB_BLUE, 0);
}

static void simulator_show(void)
{
    wave_stop();
    if (sim.screen) {
        lv_obj_del_async(sim.screen);
        sim.screen = NULL;
    }

    sim.source_selected = false;
    sim.wave_phase = 0;
    sim.state = SIM_FRONT_BROKEN;

    sim.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(sim.screen, LAB_BG, 0);
    lv_obj_set_style_bg_opa(sim.screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(sim.screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(sim.screen, simulator_screen_del_cb, LV_EVENT_DELETE, NULL);

    lv_obj_t *badge = lv_obj_create(sim.screen);
    lv_obj_set_size(badge, 480, 38);
    lv_obj_set_pos(badge, 0, 0);
    lv_obj_set_style_bg_color(badge, LAB_YELLOW, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_80, 0);
    lv_obj_set_style_radius(badge, 0, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_shadow_width(badge, 0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *badge_text = lv_label_create(badge);
    lv_label_set_text(badge_text, "> LabVIEW SIMULATOR_");
    lv_obj_set_style_text_color(badge_text, lv_color_hex(0x111111), 0);
    lv_obj_set_style_text_font(badge_text, &lv_font_montserrat_18, 0);
    lv_obj_align(badge_text, LV_ALIGN_LEFT_MID, 12, 0);

    lv_obj_t *btn_close = lv_btn_create(sim.screen);
    lv_obj_set_size(btn_close, 64, 34);
    lv_obj_set_pos(btn_close, 16, 434);
    lv_obj_set_style_bg_color(btn_close, LAB_RED, 0);
    lv_obj_set_style_radius(btn_close, 8, 0);
    lv_obj_set_style_shadow_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, simulator_close_cb, LV_EVENT_CLICKED, sim.screen);

    lv_obj_t *btn_close_label = lv_label_create(btn_close);
    lv_label_set_text(btn_close_label, "STOP");
    lv_obj_set_style_text_color(btn_close_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(btn_close_label, &lv_font_montserrat_14, 0);
    lv_obj_center(btn_close_label);

    lv_obj_t *title = lv_label_create(sim.screen);
    lv_label_set_text(title, "Front Panel / Block Diagram");
    lv_obj_set_style_text_color(title, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 44);

    sim.state_badge = lv_obj_create(sim.screen);
    lv_obj_set_size(sim.state_badge, 176, 30);
    lv_obj_align(sim.state_badge, LV_ALIGN_TOP_LEFT, 24, 82);
    lv_obj_set_style_radius(sim.state_badge, 15, 0);
    lv_obj_set_style_border_width(sim.state_badge, 0, 0);
    lv_obj_set_style_shadow_width(sim.state_badge, 0, 0);
    lv_obj_clear_flag(sim.state_badge, LV_OBJ_FLAG_SCROLLABLE);

    sim.state_label = lv_label_create(sim.state_badge);
    lv_obj_set_style_text_color(sim.state_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(sim.state_label, &lv_font_montserrat_14, 0);
    lv_obj_center(sim.state_label);

    sim.instruction = lv_label_create(sim.screen);
    lv_obj_set_style_text_color(sim.instruction, LAB_MUTED, 0);
    lv_obj_set_style_text_font(sim.instruction, &lv_font_montserrat_14, 0);
    lv_obj_set_width(sim.instruction, 248);
    lv_label_set_long_mode(sim.instruction, LV_LABEL_LONG_WRAP);
    lv_obj_align(sim.instruction, LV_ALIGN_TOP_RIGHT, -24, 78);

    sim.viewport = lv_obj_create(sim.screen);
    lv_obj_set_size(sim.viewport, LAB_VIEW_W, LAB_VIEW_H);
    lv_obj_align(sim.viewport, LV_ALIGN_TOP_MID, 0, 126);
    lv_obj_set_style_bg_color(sim.viewport, LAB_PANEL, 0);
    lv_obj_set_style_bg_opa(sim.viewport, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(sim.viewport, LAB_BORDER, 0);
    lv_obj_set_style_border_width(sim.viewport, 1, 0);
    lv_obj_set_style_radius(sim.viewport, 16, 0);
    lv_obj_set_style_pad_all(sim.viewport, 0, 0);
    lv_obj_set_style_shadow_width(sim.viewport, 0, 0);
    lv_obj_clear_flag(sim.viewport, LV_OBJ_FLAG_SCROLLABLE);

    sim.image = lv_img_create(sim.viewport);
    lv_obj_align(sim.image, LV_ALIGN_CENTER, 0, 0);

    sim.toggle_btn = lv_btn_create(sim.viewport);
    lv_obj_set_size(sim.toggle_btn, 120, 34);
    lv_obj_align(sim.toggle_btn, LV_ALIGN_TOP_RIGHT, -12, 10);
    lv_obj_set_style_bg_color(sim.toggle_btn, LAB_BLUE, 0);
    lv_obj_set_style_radius(sim.toggle_btn, 8, 0);
    lv_obj_set_style_shadow_width(sim.toggle_btn, 0, 0);
    lv_obj_add_event_cb(sim.toggle_btn, simulator_toggle_cb, LV_EVENT_CLICKED, NULL);

    sim.toggle_label = lv_label_create(sim.toggle_btn);
    lv_obj_set_style_text_color(sim.toggle_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(sim.toggle_label, &lv_font_montserrat_14, 0);
    lv_obj_center(sim.toggle_label);

    sim.run_btn = lv_btn_create(sim.viewport);
    lv_obj_set_size(sim.run_btn, 92, 34);
    lv_obj_align(sim.run_btn, LV_ALIGN_BOTTOM_RIGHT, -12, -10);
    lv_obj_set_style_bg_color(sim.run_btn, LAB_GREEN, 0);
    lv_obj_set_style_radius(sim.run_btn, 8, 0);
    lv_obj_set_style_shadow_width(sim.run_btn, 0, 0);
    lv_obj_add_event_cb(sim.run_btn, simulator_run_cb, LV_EVENT_CLICKED, NULL);

    sim.run_label = lv_label_create(sim.run_btn);
    lv_obj_set_style_text_color(sim.run_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(sim.run_label, &lv_font_montserrat_14, 0);
    lv_obj_center(sim.run_label);

    sim.source_hotspot = lv_obj_create(sim.viewport);
    lv_obj_set_size(sim.source_hotspot, 42, 42);
    lv_obj_set_pos(sim.source_hotspot, 53, 106);
    lv_obj_set_style_bg_opa(sim.source_hotspot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sim.source_hotspot, 0, 0);
    lv_obj_set_style_shadow_width(sim.source_hotspot, 0, 0);
    lv_obj_clear_flag(sim.source_hotspot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(sim.source_hotspot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(sim.source_hotspot, simulator_source_cb, LV_EVENT_CLICKED, NULL);

    sim.target_hotspot = lv_obj_create(sim.viewport);
    lv_obj_set_size(sim.target_hotspot, 42, 42);
    lv_obj_set_pos(sim.target_hotspot, 160, 63);
    lv_obj_set_style_bg_opa(sim.target_hotspot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sim.target_hotspot, 0, 0);
    lv_obj_set_style_shadow_width(sim.target_hotspot, 0, 0);
    lv_obj_clear_flag(sim.target_hotspot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(sim.target_hotspot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(sim.target_hotspot, simulator_target_cb, LV_EVENT_CLICKED, NULL);

    sim.wire_line = lv_line_create(sim.viewport);
    lv_obj_set_style_line_opa(sim.wire_line, LV_OPA_TRANSP, 0);
    lv_obj_set_style_line_width(sim.wire_line, 4, 0);
    lv_obj_set_style_line_rounded(sim.wire_line, true, 0);

    sim.wave_box = lv_obj_create(sim.viewport);
    lv_obj_set_size(sim.wave_box, 180, 80);
    lv_obj_set_pos(sim.wave_box, 210, 50);
    lv_obj_set_style_bg_opa(sim.wave_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sim.wave_box, 0, 0);
    lv_obj_set_style_pad_all(sim.wave_box, 0, 0);
    lv_obj_set_style_shadow_width(sim.wave_box, 0, 0);
    lv_obj_clear_flag(sim.wave_box, LV_OBJ_FLAG_SCROLLABLE);

    /* Pre-compute sine wave points */
    for (int i = 0; i < WAVE_PT_COUNT; i++) {
        sim.wave_pts[i].x = 10 + i * (160 / (WAVE_PT_COUNT - 1));
        sim.wave_pts[i].y = 40 - sine_lut[i];
    }

    sim.wave_line = lv_line_create(sim.wave_box);
    lv_line_set_points(sim.wave_line, sim.wave_pts, WAVE_PT_COUNT);
    lv_obj_set_style_line_color(sim.wave_line, LAB_BLUE, 0);
    lv_obj_set_style_line_width(sim.wave_line, 2, 0);
    lv_obj_set_style_line_rounded(sim.wave_line, true, 0);

    sim.wave_dot = lv_obj_create(sim.wave_box);
    lv_obj_set_size(sim.wave_dot, 10, 10);
    lv_obj_set_style_radius(sim.wave_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(sim.wave_dot, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(sim.wave_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sim.wave_dot, 0, 0);
    lv_obj_set_style_shadow_width(sim.wave_dot, 8, 0);
    lv_obj_set_style_shadow_color(sim.wave_dot, LAB_BLUE, 0);
    lv_obj_clear_flag(sim.wave_dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(sim.wave_dot, LV_OBJ_FLAG_CLICKABLE);

    sim.wave_timer = NULL;

    simulator_refresh();
    app_router_enter_immersive(sim.screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void launch_simulator_cb(lv_event_t *e)
{
    (void)e;
    simulator_show();
    /* Force synchronous render to flush frame buffer immediately */
    lv_refr_now(NULL);
}

lv_obj_t *page_labview_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, LAB_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "LabVIEW");
    lv_obj_set_style_text_color(title, COLOR_ACCENT_YELLOW, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_letter_space(title, 2, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "Front Panel / Block Diagram simulator");
    lv_obj_set_style_text_color(sub, LAB_MUTED, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 38);

    lv_obj_t *preview = lv_obj_create(parent);
    lv_obj_set_size(preview, 420, 242);
    lv_obj_align(preview, LV_ALIGN_TOP_MID, 0, 62);
    lv_obj_set_style_bg_color(preview, LAB_PANEL, 0);
    lv_obj_set_style_bg_opa(preview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(preview, LAB_BORDER, 0);
    lv_obj_set_style_border_width(preview, 1, 0);
    lv_obj_set_style_radius(preview, 16, 0);
    lv_obj_set_style_pad_all(preview, 0, 0);
    lv_obj_set_style_shadow_width(preview, 0, 0);
    lv_obj_clear_flag(preview, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *preview_img = lv_img_create(preview);
    lv_img_set_src(preview_img, &img_lv_preview);
    lv_obj_align(preview_img, LV_ALIGN_CENTER, 0, -6);

    lv_obj_t *preview_hint = lv_label_create(preview);
    lv_label_set_text(preview_hint, "Broken FP -> Show Diagram -> Wire -> Run");
    lv_obj_set_style_text_color(preview_hint, LAB_MUTED, 0);
    lv_obj_set_style_text_font(preview_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(preview_hint, LV_ALIGN_BOTTOM_MID, 0, -12);

    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 188, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -52);
    lv_obj_set_style_bg_color(btn, COLOR_NI_GREEN, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, launch_simulator_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Launch Simulator");
    lv_obj_set_style_text_color(btn_lbl, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(btn_lbl);

    lv_obj_t *footer = lv_label_create(parent);
    lv_label_set_text(footer, "Wire a broken VI and watch it run.");
    lv_obj_set_style_text_color(footer, LAB_MUTED, 0);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_14, 0);
    lv_obj_set_width(footer, 420);
    lv_label_set_long_mode(footer, LV_LABEL_LONG_WRAP);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -2);

    ESP_LOGI(TAG, "LabVIEW landing page built");
    return parent;
}
