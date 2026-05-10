#pragma once

#include "lvgl.h"

/**
 * Build the QR code configuration page.
 * Shows QR code linking to the badge's config server.
 * Also starts the WiFi AP + HTTP server.
 */
void build_page_config(lv_obj_t *parent);
