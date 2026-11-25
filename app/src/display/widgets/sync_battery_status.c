/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/sync_battery_status.h>
#include <zmk/event_manager.h>
#include <zmk/events/sync_battery_state.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void set_battery_symbol(lv_obj_t *label, struct zmk_sync_battery_state state) {
    char text[16];
    uint8_t level = state.level;
    const char *symbol;

    if (level > 95) {
        symbol = LV_SYMBOL_BATTERY_FULL;
    } else if (level > 65) {
        symbol = LV_SYMBOL_BATTERY_3;
    } else if (level > 35) {
        symbol = LV_SYMBOL_BATTERY_2;
    } else if (level > 5) {
        symbol = LV_SYMBOL_BATTERY_1;
    } else {
        symbol = LV_SYMBOL_BATTERY_EMPTY;
    }

    /* Remote charge state belongs to the central, not the local USB port. */
    if (state.is_charging) {
        symbol = LV_SYMBOL_CHARGE;
    }

    if (level <= 100) {
        snprintf(text, sizeof(text), "%s%u%%", symbol, (unsigned int)level);
    } else {
        snprintf(text, sizeof(text), "%s--%%", symbol);
    }
    lv_label_set_text(label, text);
}

void sync_battery_status_update_cb(struct zmk_sync_battery_state state) {
    struct zmk_widget_sync_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_symbol(widget->obj, state); }
}

static struct zmk_sync_battery_state sync_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_sync_battery_state *ev = as_zmk_sync_battery_state(eh);
    if (ev == NULL) {
        return (struct zmk_sync_battery_state){0};
    }
    return *ev;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_sync_battery_status, struct zmk_sync_battery_state,
                            sync_battery_status_update_cb, sync_battery_status_get_state)
ZMK_SUBSCRIPTION(widget_sync_battery_status, zmk_sync_battery_state);

int zmk_widget_sync_battery_status_init(struct zmk_widget_sync_battery_status *widget,
                                        lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);
    sys_slist_append(&widgets, &widget->node);
    widget_sync_battery_status_init();
    return 0;
}

lv_obj_t *zmk_widget_sync_battery_status_obj(struct zmk_widget_sync_battery_status *widget) {
    return widget->obj;
}
