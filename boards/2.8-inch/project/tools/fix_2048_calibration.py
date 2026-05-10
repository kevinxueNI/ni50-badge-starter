#!/usr/bin/env python3
"""Rewrite 2048 calibration and tilt logic in Badge_UI.c"""
from pathlib import Path

filepath = Path(__file__).resolve().parents[1] / 'main' / 'LVGL_UI' / 'Badge_UI.c'

with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================
# 1. Replace g2048 struct fields (calibration part)
# ============================================================
old_struct = """    lv_obj_t *btn_reset;
    lv_obj_t *btn_calibrate;
    lv_obj_t *lbl_cal_status;
    lv_timer_t *tilt_timer;
    uint32_t score;
    uint32_t last_move_ms;
    float cal_x;           /* \u6821\u51c6\u504f\u79fb */
    float cal_y;
    bool game_over;
    bool animating;
    bool calibrating;
} g2048;"""

new_struct = """    lv_obj_t *btn_reset;
    lv_obj_t *btn_calibrate;
    lv_timer_t *tilt_timer;
    uint32_t score;
    uint32_t last_move_ms;
    /* \u6821\u51c6\u72b6\u6001\u673a: 0=\u7a7a\u95f2 1=\u7b49\u5f85\u7ad6\u76f4 2=\u7b49\u5f85\u5de6 3=\u7b49\u5f85\u53f3 4=\u7b49\u5f85\u524d */
    int cal_step;
    /* \u6821\u51c6\u91c7\u6837\u70b9 (x,y,z) */
    float cal_n[3];   /* neutral/\u7ad6\u76f4 = DOWN \u65b9\u5411 */
    float cal_l[3];   /* left */
    float cal_r[3];   /* right */
    float cal_u[3];   /* forward = UP \u65b9\u5411 */
    /* \u6d3e\u751f\u65b9\u5411\u5411\u91cf (\u5f52\u4e00\u5316) */
    float lr_dir[3];  /* \u6307\u5411\u53f3 */
    float up_dir[3];  /* \u6307\u5411\u524d/UP */
    bool calibrated;
    bool game_over;
    bool animating;
} g2048;"""

assert old_struct in content, "Struct not found!"
content = content.replace(old_struct, new_struct, 1)

# ============================================================
# 2. Replace calibration callbacks + tilt timer
# ============================================================
# Find the section from "/* \u2500\u2500 \u6821\u51c6\u56de\u8c03 \u2500\u2500 */" to end of g2048_tilt_timer_cb
cal_start = content.find('/* \u2500\u2500 \u6821\u51c6\u56de\u8c03 \u2500\u2500 */')
assert cal_start != -1, "Cal section start not found!"

# Find the closing brace of g2048_tilt_timer_cb (ends with "}\n\nstatic void build_page_2048")
build_page_marker = '\nstatic void build_page_2048'
build_page_idx = content.find(build_page_marker, cal_start)
assert build_page_idx != -1, "build_page_2048 not found!"

# The section to replace goes from cal_start to build_page_idx
old_section = content[cal_start:build_page_idx]

new_section = """/* \u2500\u2500 \u6821\u51c6\u72b6\u6001\u673a \u2500\u2500 */
static const char *cal_prompts[] = {
    "",
    "Hold VERTICAL\\nthen tap screen",
    "Tilt LEFT\\nthen tap screen",
    "Tilt RIGHT\\nthen tap screen",
    "Tilt FORWARD\\nthen tap screen",
};

static void g2048_cal_compute(void)
{
    /* LR \u65b9\u5411: normalize(R - L) */
    float lr[3] = {
        g2048.cal_r[0] - g2048.cal_l[0],
        g2048.cal_r[1] - g2048.cal_l[1],
        g2048.cal_r[2] - g2048.cal_l[2]
    };
    float lr_mag = sqrtf(lr[0]*lr[0] + lr[1]*lr[1] + lr[2]*lr[2]);
    if (lr_mag < 0.01f) lr_mag = 1.0f;
    g2048.lr_dir[0] = lr[0] / lr_mag;
    g2048.lr_dir[1] = lr[1] / lr_mag;
    g2048.lr_dir[2] = lr[2] / lr_mag;

    /* UP \u65b9\u5411: normalize(U - N), \u4ece\u7ad6\u76f4\u6307\u5411\u524d\u503e = \u6e38\u620f UP */
    float ud[3] = {
        g2048.cal_u[0] - g2048.cal_n[0],
        g2048.cal_u[1] - g2048.cal_n[1],
        g2048.cal_u[2] - g2048.cal_n[2]
    };
    float ud_mag = sqrtf(ud[0]*ud[0] + ud[1]*ud[1] + ud[2]*ud[2]);
    if (ud_mag < 0.01f) ud_mag = 1.0f;
    g2048.up_dir[0] = ud[0] / ud_mag;
    g2048.up_dir[1] = ud[1] / ud_mag;
    g2048.up_dir[2] = ud[2] / ud_mag;

    g2048.calibrated = true;
    g2048.cal_step = 0;
    lv_label_set_text(g2048.lbl_tilt_hint, LV_SYMBOL_OK " Calibrated!");
}

/* \u6821\u51c6\u70b9\u51fb\u5c4f\u5e55\u786e\u8ba4\u56de\u8c03\uff08\u6ce8\u518c\u5728 tileview \u9875\u9762\u4e0a\uff09 */
static void g2048_cal_tap_cb(lv_event_t *e)
{
    (void)e;
    if (g2048.cal_step <= 0) return;

    float cur[3] = { Accel.x, Accel.y, Accel.z };

    switch (g2048.cal_step) {
    case 1: /* \u8bb0\u5f55\u7ad6\u76f4 (neutral=DOWN) */
        g2048.cal_n[0] = cur[0]; g2048.cal_n[1] = cur[1]; g2048.cal_n[2] = cur[2];
        g2048.cal_step = 2;
        lv_label_set_text(g2048.lbl_tilt_hint, cal_prompts[2]);
        break;
    case 2: /* \u8bb0\u5f55\u5de6\u503e */
        g2048.cal_l[0] = cur[0]; g2048.cal_l[1] = cur[1]; g2048.cal_l[2] = cur[2];
        g2048.cal_step = 3;
        lv_label_set_text(g2048.lbl_tilt_hint, cal_prompts[3]);
        break;
    case 3: /* \u8bb0\u5f55\u53f3\u503e */
        g2048.cal_r[0] = cur[0]; g2048.cal_r[1] = cur[1]; g2048.cal_r[2] = cur[2];
        g2048.cal_step = 4;
        lv_label_set_text(g2048.lbl_tilt_hint, cal_prompts[4]);
        break;
    case 4: /* \u8bb0\u5f55\u524d\u503e (UP) */
        g2048.cal_u[0] = cur[0]; g2048.cal_u[1] = cur[1]; g2048.cal_u[2] = cur[2];
        g2048_cal_compute();
        break;
    }
}

/* CAL \u6309\u94ae\u56de\u8c03\uff1a\u542f\u52a8\u6821\u51c6\u6d41\u7a0b */
static void g2048_calibrate_cb(lv_event_t *e)
{
    (void)e;
    g2048.cal_step = 1;
    g2048.calibrated = false;
    lv_label_set_text(g2048.lbl_tilt_hint, cal_prompts[1]);
}

/* \u2500\u2500 \u503e\u659c\u63a7\u5236\u5b9a\u65f6\u5668 \u2500\u2500 */
static void g2048_tilt_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (current_page != 4) return;
    if (g2048.game_over || g2048.animating) return;
    if (g2048.cal_step > 0) return;  /* \u6821\u51c6\u4e2d\u4e0d\u54cd\u5e94 */
    if (!g2048.calibrated) return;   /* \u672a\u6821\u51c6\u4e0d\u54cd\u5e94 */

    uint32_t now = lv_tick_get();
    if (now - g2048.last_move_ms < TILT_COOLDOWN_MS) return;

    /* \u8ba1\u7b97\u5f53\u524d\u52a0\u901f\u5ea6\u4e0e\u4e2d\u7acb\u4f4d\u7684\u5dee\u503c */
    float dx = Accel.x - g2048.cal_n[0];
    float dy = Accel.y - g2048.cal_n[1];
    float dz = Accel.z - g2048.cal_n[2];

    /* \u6295\u5f71\u5230 LR \u8f74 (\u6b63=\u53f3) */
    float lr_proj = dx * g2048.lr_dir[0] + dy * g2048.lr_dir[1] + dz * g2048.lr_dir[2];
    /* \u6295\u5f71\u5230 UP \u8f74 (\u6b63=\u524d/UP) */
    float up_proj = dx * g2048.up_dir[0] + dy * g2048.up_dir[1] + dz * g2048.up_dir[2];

    float abs_lr = (lr_proj < 0) ? -lr_proj : lr_proj;
    float abs_up = (up_proj < 0) ? -up_proj : up_proj;

    int dir = -1;
    if (abs_lr > abs_up && abs_lr > TILT_THRESHOLD) {
        dir = (lr_proj > 0) ? 1 : 0;   /* 1=right, 0=left */
    } else if (abs_up > abs_lr && abs_up > TILT_THRESHOLD) {
        dir = (up_proj > 0) ? 2 : 3;   /* 2=up, 3=down (but we handle down below) */
    } else {
        /* \u8fd1\u4e2d\u7acb\u4f4d\u7f6e (\u7ad6\u76f4) = \u81ea\u52a8 DOWN */
        dir = 3;
    }

    /* \u4fdd\u5b58\u79fb\u52a8\u524d\u72b6\u6001\u7528\u4e8e\u52a8\u753b */
    memcpy(g2048.prev_board, g2048.board, sizeof(g2048.board));

    if (g2048_move(dir)) {
        g2048.last_move_ms = now;
        g2048_spawn();
        if (!g2048_can_move()) g2048.game_over = true;
        g2048_animate_move(dir);
    }
}
"""

content = content[:cal_start] + new_section + content[build_page_idx:]

# ============================================================
# 3. Replace button layout in build_page_2048
#    Move CAL button to left of Reset; remove separate tilt hint row
# ============================================================
old_buttons = """    /* Reset \u6309\u94ae */
    g2048.btn_reset = lv_btn_create(parent);
    lv_obj_set_size(g2048.btn_reset, 60, 24);
    lv_obj_align(g2048.btn_reset, LV_ALIGN_TOP_RIGHT, -8, 28);
    lv_obj_set_style_bg_color(g2048.btn_reset, COLOR_NI_GREEN, 0);
    lv_obj_set_style_radius(g2048.btn_reset, 12, 0);
    lv_obj_add_event_cb(g2048.btn_reset, g2048_reset_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rlbl = lv_label_create(g2048.btn_reset);
    lv_label_set_text(rlbl, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(rlbl, &lv_font_montserrat_12, 0);
    lv_obj_center(rlbl);

    /* \u503e\u659c\u63d0\u793a + \u6821\u51c6\u6309\u94ae */
    g2048.lbl_tilt_hint = lv_label_create(parent);
    lv_label_set_text(g2048.lbl_tilt_hint, LV_SYMBOL_LOOP " Tilt to play");
    lv_obj_set_style_text_color(g2048.lbl_tilt_hint, lv_color_hex(0x7F8C8D), 0);
    lv_obj_set_style_text_font(g2048.lbl_tilt_hint, &lv_font_montserrat_10, 0);
    lv_obj_align(g2048.lbl_tilt_hint, LV_ALIGN_TOP_LEFT, 12, 55);

    /* Calibrate \u6309\u94ae */
    g2048.btn_calibrate = lv_btn_create(parent);
    lv_obj_set_size(g2048.btn_calibrate, 50, 20);
    lv_obj_align(g2048.btn_calibrate, LV_ALIGN_TOP_RIGHT, -8, 53);
    lv_obj_set_style_bg_color(g2048.btn_calibrate, lv_color_hex(0x2980B9), 0);
    lv_obj_set_style_radius(g2048.btn_calibrate, 10, 0);
    lv_obj_add_event_cb(g2048.btn_calibrate, g2048_calibrate_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cal_lbl = lv_label_create(g2048.btn_calibrate);
    lv_label_set_text(cal_lbl, "CAL");
    lv_obj_set_style_text_font(cal_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(cal_lbl);"""

new_buttons = """    /* CAL \u6309\u94ae (\u5de6\u4fa7) */
    g2048.btn_calibrate = lv_btn_create(parent);
    lv_obj_set_size(g2048.btn_calibrate, 50, 24);
    lv_obj_align(g2048.btn_calibrate, LV_ALIGN_TOP_RIGHT, -72, 28);
    lv_obj_set_style_bg_color(g2048.btn_calibrate, lv_color_hex(0x2980B9), 0);
    lv_obj_set_style_radius(g2048.btn_calibrate, 12, 0);
    lv_obj_add_event_cb(g2048.btn_calibrate, g2048_calibrate_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cal_lbl = lv_label_create(g2048.btn_calibrate);
    lv_label_set_text(cal_lbl, "CAL");
    lv_obj_set_style_text_font(cal_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(cal_lbl);

    /* Reset \u6309\u94ae (\u53f3\u4fa7) */
    g2048.btn_reset = lv_btn_create(parent);
    lv_obj_set_size(g2048.btn_reset, 50, 24);
    lv_obj_align(g2048.btn_reset, LV_ALIGN_TOP_RIGHT, -8, 28);
    lv_obj_set_style_bg_color(g2048.btn_reset, COLOR_NI_GREEN, 0);
    lv_obj_set_style_radius(g2048.btn_reset, 12, 0);
    lv_obj_add_event_cb(g2048.btn_reset, g2048_reset_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rlbl = lv_label_create(g2048.btn_reset);
    lv_label_set_text(rlbl, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(rlbl, &lv_font_montserrat_12, 0);
    lv_obj_center(rlbl);

    /* \u63d0\u793a\u6807\u7b7e\uff08\u6821\u51c6\u6d41\u7a0b + \u72b6\u6001\uff09 */
    g2048.lbl_tilt_hint = lv_label_create(parent);
    lv_label_set_text(g2048.lbl_tilt_hint, "Press CAL to start");
    lv_obj_set_style_text_color(g2048.lbl_tilt_hint, lv_color_hex(0x7F8C8D), 0);
    lv_obj_set_style_text_font(g2048.lbl_tilt_hint, &lv_font_montserrat_10, 0);
    lv_obj_align(g2048.lbl_tilt_hint, LV_ALIGN_TOP_MID, 0, 55);

    /* \u6ce8\u518c\u70b9\u51fb\u5c4f\u5e55\u4e8b\u4ef6\u7528\u4e8e\u6821\u51c6\u786e\u8ba4 */
    lv_obj_add_event_cb(parent, g2048_cal_tap_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE);"""

assert old_buttons in content, "Button section not found!"
content = content.replace(old_buttons, new_buttons, 1)

# ============================================================
# 4. Remove the old g2048_cal_restore_cb forward declaration if any leftover
# ============================================================
# Clean up: remove leftover "g2048_cal_restore_cb" references if present
# (Already handled by section replacement above)

# ============================================================
# 5. Add #include <math.h> if not already present (for sqrtf)
# ============================================================
if '#include <math.h>' not in content:
    # Add after the last #include
    last_include = content.rfind('#include', 0, content.find('\n\n', content.find('#include')))
    # Actually just add it near the top includes
    include_marker = '#include "QMI8658.h"'
    if include_marker in content:
        content = content.replace(include_marker, include_marker + '\n#include <math.h>', 1)
    else:
        # fallback: add after first #include
        first_include_end = content.find('\n', content.find('#include'))
        content = content[:first_include_end+1] + '#include <math.h>\n' + content[first_include_end+1:]

# ============================================================
# Write back
# ============================================================
with open(filepath, 'w', encoding='utf-8', newline='\n') as f:
    f.write(content)

print("All modifications applied successfully!")
print(f"File size: {len(content)} bytes")
