#pragma once

#include "lvgl.h"
#include "LVGL_Driver.h"
#include "PCF85063.h"
#include "BAT_Driver.h"

/* 屏幕尺寸 */
#define SCREEN_W  240
#define SCREEN_H  320

/* 颜色定义 */
#define COLOR_BG_DARK      lv_color_hex(0x1C233B)   /* 深蓝背景 */
#define COLOR_ACCENT_YELLOW lv_color_hex(0xF1C40F)   /* NI 经典黄 */
#define COLOR_NI_GREEN     lv_color_hex(0x00A651)    /* NI 经典绿 */
#define COLOR_TEXT_WHITE   lv_color_hex(0xFFFFFF)
#define COLOR_LINE_DIM     lv_color_hex(0x2A3258)    /* 几何线条 */

/**
 * @brief 初始化电子胸卡 UI（三页 tileview）
 */
void Badge_UI_Init(void);
