#!/usr/bin/env python3
"""Fix calibration code in Badge_UI.c"""
import sys
from pathlib import Path

filepath = Path(__file__).resolve().parents[1] / 'main' / 'LVGL_UI' / 'Badge_UI.c'

with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

# Find the section to replace: from "static void g2048_calibrate_cb" through the tilt direction block
start_marker = '/* \u2500\u2500 \u6821\u51c6\u56de\u8c03 \u2500\u2500 */\nstatic void g2048_calibrate_cb'
end_marker = '    if (dir < 0) return;'

start_idx = content.find(start_marker)
if start_idx == -1:
    print("ERROR: Could not find start marker")
    sys.exit(1)

end_idx = content.find(end_marker, start_idx)
if end_idx == -1:
    print("ERROR: Could not find end marker")
    sys.exit(1)

# Include the end marker in what we replace
end_idx += len(end_marker)

old_section = content[start_idx:end_idx]
print(f"Found section at [{start_idx}:{end_idx}], length={len(old_section)}")
print("First 100 chars:", repr(old_section[:100]))

new_section = """/* \u2500\u2500 \u6821\u51c6\u56de\u8c03 \u2500\u2500 */
static void g2048_cal_restore_cb(lv_timer_t *t)
{
    lv_label_set_text(g2048.lbl_tilt_hint, LV_SYMBOL_LOOP " Tilt to play");
    g2048.calibrating = false;
    lv_timer_del(t);
}

static void g2048_calibrate_cb(lv_event_t *e)
{
    (void)e;
    g2048.cal_x = Accel.x;
    g2048.cal_y = Accel.y;
    g2048.calibrating = true;
    lv_label_set_text(g2048.lbl_tilt_hint, LV_SYMBOL_OK " Calibrated!");
    lv_timer_create(g2048_cal_restore_cb, 1500, NULL);
}

/* \u2500\u2500 \u503e\u659c\u63a7\u5236\u5b9a\u65f6\u5668 \u2500\u2500 */
static void g2048_tilt_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (current_page != 4) return;
    if (g2048.game_over || g2048.animating) return;
    if (g2048.calibrating) return;

    uint32_t now = lv_tick_get();
    if (now - g2048.last_move_ms < TILT_COOLDOWN_MS) return;

    float ax = Accel.x - g2048.cal_x;
    float ay = Accel.y - g2048.cal_y;
    float aax = (ax < 0) ? -ax : ax;
    float aay = (ay < 0) ? -ay : ay;

    int dir = -1;
    if (aax > aay && aax > TILT_THRESHOLD) {
        dir = (ax > 0) ? 1 : 0;
    } else if (aay > aax && aay > TILT_THRESHOLD) {
        dir = (ay > 0) ? 2 : 3;
    }

    if (dir < 0) return;"""

content = content[:start_idx] + new_section + content[end_idx:]

with open(filepath, 'w', encoding='utf-8', newline='\n') as f:
    f.write(content)

print("OK - calibration code replaced successfully")
