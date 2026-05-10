#pragma once

#include "lvgl.h"

/**
 * page_gallery.h — NI Gallery
 *
 * Tile page shows "NI Gallery" with an Enter button.
 * Clicking Enter opens an independent screen with two album choices:
 * - NI Logo (7 images with descriptions, swipeable)
 * - LabVIEW 1.0 (3 images, swipeable)
 */

/**
 * Build the gallery tile content (shown in main tileview).
 */
void build_page_gallery(lv_obj_t *parent);
