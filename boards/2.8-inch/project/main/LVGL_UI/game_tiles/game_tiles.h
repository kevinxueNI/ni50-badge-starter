#pragma once
#include "lvgl.h"

#define GAME_TILE_COUNT 22

extern const lv_img_dsc_t img_tile_pcie_6357;
extern const lv_img_dsc_t img_tile_pxie_4322;
extern const lv_img_dsc_t img_tile_ni_logo_dark;
extern const lv_img_dsc_t img_tile_pxi_4110;
extern const lv_img_dsc_t img_tile_pxie_chassis;
extern const lv_img_dsc_t img_tile_crio_9045;
extern const lv_img_dsc_t img_tile_pxi_2567;
extern const lv_img_dsc_t img_tile_usb_6343;
extern const lv_img_dsc_t img_tile_pxi_4461;
extern const lv_img_dsc_t img_tile_scope_5105;
extern const lv_img_dsc_t img_tile_cdaq_9171;
extern const lv_img_dsc_t img_tile_pxie_5622;
extern const lv_img_dsc_t img_tile_pxi_system;
extern const lv_img_dsc_t img_tile_compactrio;
extern const lv_img_dsc_t img_tile_ni_switch;
extern const lv_img_dsc_t img_tile_diadem;
extern const lv_img_dsc_t img_tile_flexlogger;
extern const lv_img_dsc_t img_tile_labview;
extern const lv_img_dsc_t img_tile_ni50_badge;
extern const lv_img_dsc_t img_tile_systemlink;
extern const lv_img_dsc_t img_tile_teststand;
extern const lv_img_dsc_t img_tile_veristand;

static const lv_img_dsc_t *game_tile_pool[GAME_TILE_COUNT] = {
    &img_tile_pcie_6357,
    &img_tile_pxie_4322,
    &img_tile_ni_logo_dark,
    &img_tile_pxi_4110,
    &img_tile_pxie_chassis,
    &img_tile_crio_9045,
    &img_tile_pxi_2567,
    &img_tile_usb_6343,
    &img_tile_pxi_4461,
    &img_tile_scope_5105,
    &img_tile_cdaq_9171,
    &img_tile_pxie_5622,
    &img_tile_pxi_system,
    &img_tile_compactrio,
    &img_tile_ni_switch,
    &img_tile_diadem,
    &img_tile_flexlogger,
    &img_tile_labview,
    &img_tile_ni50_badge,
    &img_tile_systemlink,
    &img_tile_teststand,
    &img_tile_veristand,
};
