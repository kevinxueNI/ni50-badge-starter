#pragma once

#include "lvgl.h"

/**
 * page_calendar.h — NI Milestone Calendar page
 *
 * Dark-themed calendar showing current month with:
 * - Today highlighted in NI green
 * - Special NI milestone dates highlighted in yellow
 * - Bottom event card showing milestone info on tap
 */

/**
 * Build the calendar page content inside the given parent (tileview tile).
 */
void build_page_calendar(lv_obj_t *parent);
