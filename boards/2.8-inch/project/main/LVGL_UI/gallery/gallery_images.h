#pragma once
#include "lvgl.h"

/* NI Logo images (200x120 RGB565) */
#define NILOGO_IMG_W  200
#define NILOGO_IMG_H  120
#define NILOGO_COUNT  7

extern const lv_img_dsc_t img_nilogo_1;
extern const lv_img_dsc_t img_nilogo_2;
extern const lv_img_dsc_t img_nilogo_3;
extern const lv_img_dsc_t img_nilogo_4;
extern const lv_img_dsc_t img_nilogo_5;
extern const lv_img_dsc_t img_nilogo_6;
extern const lv_img_dsc_t img_nilogo_7;

/* LabVIEW 1.0 images (200x134 RGB565) */
#define LV1_IMG_W  200
#define LV1_IMG_H  134
#define LV1_COUNT  3

extern const lv_img_dsc_t img_lv1_block_diagram;
extern const lv_img_dsc_t img_lv1_splash;
extern const lv_img_dsc_t img_lv1_news;

/* Image pool arrays for gallery code */
static const lv_img_dsc_t * const gallery_nilogo_pool[] = {
    &img_nilogo_1,
    &img_nilogo_2,
    &img_nilogo_3,
    &img_nilogo_4,
    &img_nilogo_5,
    &img_nilogo_6,
    &img_nilogo_7,
};

static const lv_img_dsc_t * const gallery_lv1_pool[] = {
    &img_lv1_block_diagram,
    &img_lv1_splash,
    &img_lv1_news,
};
