/*
 * YUAN OLED V2.1 -- selective thin glyphs.
 * SPDX-License-Identifier: MIT
 * Connection font: chain (local WIFI alias), Bluetooth only.
 * Battery font: battery levels, percentage and its digits only.
 * All other glyphs (especially lightning, check/cross, USB,
 * and Bluetooth profile numbers) use the original Montserrat 12.
 */
#ifndef YUAN_STATUS_FONT_H
#define YUAN_STATUS_FONT_H

#include <lvgl.h>

static const uint8_t yuan_link_bitmap[] = {
    0x00, 0x00, 0x00, 0x00, 0x07, 0x8f, 0x04, 0x00, 0x44, 0x00, 0x12, 0x3f,
    0x89, 0x00, 0x04, 0x80, 0x02, 0x20, 0x02, 0x0f, 0x1e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x46, 0x12, 0xc5, 0x11, 0x90, 0x70, 0x38, 0x32, 0x28,
    0xa4, 0xa3, 0x81, 0x00,
};
static const lv_font_fmt_txt_glyph_dsc_t yuan_link_glyphs[] = {
    {.bitmap_index=0, .adv_w=0, .box_w=0, .box_h=0, .ofs_x=0, .ofs_y=0},
    {.bitmap_index=0, .adv_w=288, .box_w=17, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+F1EB */
    {.bitmap_index=26, .adv_w=176, .box_w=9, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+F293 */
};
static const uint16_t yuan_link_offsets[] = { 0x0000, 0x00a8 };
static const lv_font_fmt_txt_cmap_t yuan_link_cmaps[] = {
    {
        .range_start = 0xf1eb,
        .range_length = 169,
        .glyph_id_start = 1,
        .unicode_list = yuan_link_offsets,
        .glyph_id_ofs_list = NULL,
        .list_length = 2,
        .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY,
    },
};
static const lv_font_fmt_txt_dsc_t yuan_link_dsc = {
    .glyph_bitmap = yuan_link_bitmap,
    .glyph_dsc = yuan_link_glyphs,
    .cmaps = yuan_link_cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = LV_FONT_FMT_TXT_PLAIN,
};
static const lv_font_t yuan_link_font = {
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = 15,
    .base_line = 3,
    .subpx = LV_FONT_SUBPX_NONE,
    .dsc = &yuan_link_dsc,
    .fallback = &lv_font_montserrat_12,
};

static const uint8_t yuan_battery_bitmap[] = {
    0x01, 0x87, 0x10, 0x20, 0x82, 0x08, 0x20, 0x47, 0x0c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x03, 0xa3, 0x18, 0xc6, 0x31,
    0x8b, 0x80, 0x00, 0x01, 0x18, 0x42, 0x10, 0x84, 0x23, 0x80, 0x00, 0x03,
    0xa2, 0x10, 0x88, 0x88, 0x87, 0xc0, 0x00, 0x07, 0x82, 0x10, 0xb8, 0x21,
    0x0f, 0x80, 0x00, 0x00, 0x8c, 0xa9, 0x4b, 0xe2, 0x10, 0x80, 0x00, 0x07,
    0xe1, 0x08, 0x78, 0x21, 0x8b, 0x80, 0x00, 0x01, 0x91, 0x08, 0x7a, 0x31,
    0x8b, 0x80, 0x00, 0x07, 0xc2, 0x21, 0x10, 0x88, 0x42, 0x00, 0x00, 0x03,
    0xa3, 0x18, 0xba, 0x31, 0x8b, 0x80, 0x00, 0x03, 0xa3, 0x18, 0xbc, 0x21,
    0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x20, 0x04, 0xaa, 0x9e,
    0xaa, 0x5a, 0xa9, 0x6a, 0xa7, 0x80, 0x13, 0xff, 0xc0, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0f, 0xff, 0x20, 0x04, 0xaa, 0x1e, 0xa8, 0x5a, 0xa1,
    0x6a, 0x87, 0x80, 0x13, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0f, 0xff, 0x20, 0x04, 0xa8, 0x1e, 0xa0, 0x5a, 0x81, 0x6a, 0x07, 0x80,
    0x13, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x20,
    0x04, 0xa0, 0x1e, 0x80, 0x5a, 0x01, 0x68, 0x07, 0x80, 0x13, 0xff, 0xc0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x20, 0x04, 0x80, 0x1e,
    0x00, 0x58, 0x01, 0x60, 0x07, 0x80, 0x13, 0xff, 0xc0, 0x00, 0x00, 0x00,
};
static const lv_font_fmt_txt_glyph_dsc_t yuan_battery_glyphs[] = {
    {.bitmap_index=0, .adv_w=0, .box_w=0, .box_h=0, .ofs_x=0, .ofs_y=0},
    {.bitmap_index=0, .adv_w=128, .box_w=7, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+0025 */
    {.bitmap_index=11, .adv_w=96, .box_w=5, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+002D */
    {.bitmap_index=19, .adv_w=96, .box_w=5, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+0030 */
    {.bitmap_index=27, .adv_w=96, .box_w=5, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+0031 */
    {.bitmap_index=35, .adv_w=96, .box_w=5, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+0032 */
    {.bitmap_index=43, .adv_w=96, .box_w=5, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+0033 */
    {.bitmap_index=51, .adv_w=96, .box_w=5, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+0034 */
    {.bitmap_index=59, .adv_w=96, .box_w=5, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+0035 */
    {.bitmap_index=67, .adv_w=96, .box_w=5, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+0036 */
    {.bitmap_index=75, .adv_w=96, .box_w=5, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+0037 */
    {.bitmap_index=83, .adv_w=96, .box_w=5, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+0038 */
    {.bitmap_index=91, .adv_w=96, .box_w=5, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+0039 */
    {.bitmap_index=99, .adv_w=336, .box_w=14, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+F240 */
    {.bitmap_index=120, .adv_w=336, .box_w=14, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+F241 */
    {.bitmap_index=141, .adv_w=336, .box_w=14, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+F242 */
    {.bitmap_index=162, .adv_w=336, .box_w=14, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+F243 */
    {.bitmap_index=183, .adv_w=336, .box_w=14, .box_h=12, .ofs_x=0, .ofs_y=-1}, /* U+F244 */
};
static const uint16_t yuan_battery_offsets[] = { 0x0000, 0x0008, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0xf21b, 0xf21c, 0xf21d, 0xf21e, 0xf21f };
static const lv_font_fmt_txt_cmap_t yuan_battery_cmaps[] = {
    {
        .range_start = 0x0025,
        .range_length = 61984,
        .glyph_id_start = 1,
        .unicode_list = yuan_battery_offsets,
        .glyph_id_ofs_list = NULL,
        .list_length = 17,
        .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY,
    },
};
static const lv_font_fmt_txt_dsc_t yuan_battery_dsc = {
    .glyph_bitmap = yuan_battery_bitmap,
    .glyph_dsc = yuan_battery_glyphs,
    .cmaps = yuan_battery_cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = LV_FONT_FMT_TXT_PLAIN,
};
static const lv_font_t yuan_battery_font = {
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = 15,
    .base_line = 3,
    .subpx = LV_FONT_SUBPX_NONE,
    .dsc = &yuan_battery_dsc,
    .fallback = &lv_font_montserrat_12,
};

#endif
