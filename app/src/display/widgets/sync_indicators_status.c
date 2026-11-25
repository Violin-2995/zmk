/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/sync_indicators_status.h>
#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

#define ZMK_HID_INDICATOR_CAPS_LOCK BIT(1)

static struct zmk_hid_indicators_changed get_state(const zmk_event_t *eh) {
    struct zmk_hid_indicators_changed state = {.indicators = 0};
    if (eh == NULL) {
        return state;
    }
    state.indicators = as_zmk_hid_indicators_changed(eh)->indicators;
    return state;
}

static void set_status_symbol(lv_obj_t *label, struct zmk_hid_indicators_changed state) {
    lv_label_set_text(label, (state.indicators & ZMK_HID_INDICATOR_CAPS_LOCK) ? "ABC" : "abc");
}

static void sync_indicators_status_update_cb(struct zmk_hid_indicators_changed state) {
    struct zmk_widget_sync_indicators_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status_symbol(widget->obj, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_sync_indicators_status, struct zmk_hid_indicators_changed,
                            sync_indicators_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_sync_indicators_status, zmk_hid_indicators_changed);

int zmk_widget_sync_indicators_status_init(struct zmk_widget_sync_indicators_status *widget,
                                           lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);
    sys_slist_append(&widgets, &widget->node);
    widget_sync_indicators_status_init();
    return 0;
}

lv_obj_t *zmk_widget_sync_indicators_status_obj(struct zmk_widget_sync_indicators_status *widget) {
    return widget->obj;
}
