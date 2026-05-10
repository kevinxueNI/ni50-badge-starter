#pragma once

#include "lvgl.h"

/**
 * page_settings.h — Settings page (independent screen, not part of tileview)
 *
 * Features:
 * - Backlight brightness slider
 * - Auto-sleep timeout selection (10s, 2min, never)
 * - Save button (applies settings and returns)
 * - Back button (returns without saving)
 *
 * Sleep/Wake: After timeout with no touch, screen backlight is turned off.
 * Any touch wakes the screen. Settings are NOT persisted (reset on reboot).
 */

/* Auto-sleep timeout options */
typedef enum {
    SLEEP_TIMEOUT_10S   = 10,
    SLEEP_TIMEOUT_2MIN  = 120,
    SLEEP_TIMEOUT_NEVER = 0,
} sleep_timeout_t;

/**
 * Show the settings page (creates a new screen, loads it).
 * Call from anywhere — it will switch away from current screen.
 */
void page_settings_show(void);

/**
 * Call this periodically (e.g. every 1s in main timer) to check
 * if the sleep timeout has expired. If so, turns off backlight.
 */
void settings_sleep_check(void);

/**
 * Call on any touch event to reset the sleep timer and wake screen.
 */
void settings_touch_activity(void);

/**
 * Get current brightness setting (5-100).
 */
uint8_t settings_get_brightness(void);
