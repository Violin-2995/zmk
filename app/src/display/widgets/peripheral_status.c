/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/peripheral_status.h>
#include <zmk/event_manager.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

#if LV_USE_IMAGE
/* Original 16x12 chain-link art, A8 mask tinted to match the text. */
#define PX(b) ((b) & 0x80 ? 255 : 0), ((b) & 0x40 ? 255 : 0), \
              ((b) & 0x20 ? 255 : 0), ((b) & 0x10 ? 255 : 0), \
              ((b) & 0x08 ? 255 : 0), ((b) & 0x04 ? 255 : 0), \
              ((b) & 0x02 ? 255 : 0), ((b) & 0x01 ? 255 : 0)
static const uint8_t link_icon_pixels[] __attribute__((aligned(4))) = {
    PX(0x00), PX(0x1c),
    PX(0x00), PX(0x22),
    PX(0x00), PX(0x61),
    PX(0x07), PX(0xc1),
    PX(0x08), PX(0x81),
    PX(0x11), PX(0x82),
    PX(0x23), PX(0x84),
    PX(0x44), PX(0x88),
    PX(0x81), PX(0x70),
    PX(0x82), PX(0x00),
    PX(0x84), PX(0x00),
    PX(0x78), PX(0x00),
};
static const lv_image_dsc_t link_icon = {
    .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_A8,
               .w = 16, .h = 12, .stride = 16},
    .data_size = sizeof(link_icon_pixels), .data = link_icon_pixels,
};
#undef PX
#endif

struct peripheral_status_state {
    bool connected;
};

static struct peripheral_status_state get_state(const zmk_event_t *_eh) {
    return (struct peripheral_status_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void set_status_symbol(lv_obj_t *label, struct peripheral_status_state state) {
#if LV_USE_IMAGE
    const char *text = state.connected ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE;
#else
    const char *text = state.connected ? "LINK " LV_SYMBOL_OK : "LINK " LV_SYMBOL_CLOSE;
#endif
    LOG_DBG("connected? %s", state.connected ? "true" : "false");
    lv_label_set_text(label, text);
}

static void output_status_update_cb(struct peripheral_status_state state) {
    struct zmk_widget_peripheral_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status_symbol(widget->obj, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);

int zmk_widget_peripheral_status_init(struct zmk_widget_peripheral_status *widget,
                                      lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);
#if LV_USE_IMAGE
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_size(widget->obj, 32, 16);
    lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_label_set_long_mode(widget->obj, LV_LABEL_LONG_MODE_CLIP);

    lv_obj_t *icon = lv_image_create(widget->obj);
    lv_image_set_src(icon, &link_icon);
    lv_obj_set_style_radius(icon, 0, LV_PART_MAIN);
    lv_obj_set_style_image_recolor(icon,
        lv_obj_get_style_text_color(widget->obj, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_image_recolor_opa(icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);
#endif

    sys_slist_append(&widgets, &widget->node);
    widget_peripheral_status_init();
    return 0;
}

lv_obj_t *zmk_widget_peripheral_status_obj(struct zmk_widget_peripheral_status *widget) {
    return widget->obj;
}
