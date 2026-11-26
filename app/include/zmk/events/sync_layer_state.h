/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zephyr/sys/util.h>
#include <zmk/yuan_wire.h>

struct zmk_sync_layer_state {
    uint8_t index;
#if IS_ENABLED(CONFIG_SHIELD_PSKEEB5_LEFT) || IS_ENABLED(CONFIG_SHIELD_PSKEEB5_RIGHT)
    uint8_t yuan_magic;
    uint8_t yuan_id;
    uint8_t yuan_flags;
    char yuan_name[YUAN_NAME_LEN + 1];
#endif
};

ZMK_EVENT_DECLARE(zmk_sync_layer_state);