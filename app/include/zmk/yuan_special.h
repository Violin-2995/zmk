/* SPDX-License-Identifier: MIT */
#pragma once
#include <zephyr/sys/util.h>
#include <zmk/yuan_wire.h>
#include <zmk/events/sync_layer_state.h>

#define YUAN_RIGHT_ENABLED (IS_ENABLED(CONFIG_SHIELD_PSKEEB5_RIGHT) && \
    IS_ENABLED(CONFIG_ZMK_BLE) && IS_ENABLED(CONFIG_ZMK_SYNC_LAYER))
#define YUAN_LEFT_ENABLED (IS_ENABLED(CONFIG_SHIELD_PSKEEB5_LEFT) && \
    IS_ENABLED(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM) && \
    IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_LAYER_STATUS))

#if YUAN_RIGHT_ENABLED
struct zmk_sync_layer_state yuan_build_sync(uint8_t index);
void yuan_forget_layer(uint8_t id);
void yuan_forget_all(void);
#endif

#if YUAN_LEFT_ENABLED
#include <lvgl.h>
void yuan_special_oled_init(lv_obj_t *parent);
void yuan_special_oled_receive(const struct zmk_sync_layer_state *state);
#endif
