#pragma once
#include "lvgl.h"

#define CARD_IMAGE_COUNT 12

extern const lv_img_dsc_t img_card_cdaq_9174_edit;
extern const lv_img_dsc_t img_card_diadem;
extern const lv_img_dsc_t img_card_flexlogger;
extern const lv_img_dsc_t img_card_labview;
extern const lv_img_dsc_t img_card_ni_9215_edit;
extern const lv_img_dsc_t img_card_pxie_1084_edit;
extern const lv_img_dsc_t img_card_pxie_5842_edit;
extern const lv_img_dsc_t img_card_pxie_8862_edit;
extern const lv_img_dsc_t img_card_systemlink;
extern const lv_img_dsc_t img_card_teststand;
extern const lv_img_dsc_t img_card_usb_6003_edit;
extern const lv_img_dsc_t img_card_veristand;

static const lv_img_dsc_t *card_image_pool[CARD_IMAGE_COUNT] = {
    &img_card_cdaq_9174_edit,
    &img_card_diadem,
    &img_card_flexlogger,
    &img_card_labview,
    &img_card_ni_9215_edit,
    &img_card_pxie_1084_edit,
    &img_card_pxie_5842_edit,
    &img_card_pxie_8862_edit,
    &img_card_systemlink,
    &img_card_teststand,
    &img_card_usb_6003_edit,
    &img_card_veristand,
};
