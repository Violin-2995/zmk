/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/output_status.h>
#include <zmk/display/widgets/peripheral_status.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/wpm_status.h>
#include <zmk/display/widgets/cpi_status.h>
#include <zmk/display/widgets/sync_indicators_status.h>
#include <zmk/display/widgets/sync_layer_status.h>
#include <zmk/display/widgets/sync_output_status.h>
#include <zmk/display/widgets/sync_battery_status.h>
#include <zmk/display/status_screen.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BATTERY_STATUS)
static struct zmk_widget_battery_status battery_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
static struct zmk_widget_output_status output_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_PERIPHERAL_STATUS)
static struct zmk_widget_peripheral_status peripheral_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
static struct zmk_widget_layer_status layer_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_WPM_STATUS)
static struct zmk_widget_wpm_status wpm_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_CPI_STATUS)
static struct zmk_widget_cpi_status cpi_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_INDICATORS_STATUS)
static struct zmk_widget_sync_indicators_status sync_indicators_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_LAYER_STATUS)
static struct zmk_widget_sync_layer_status sync_layer_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_OUTPUT_STATUS)
static struct zmk_widget_sync_output_status sync_output_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_BATTERY_STATUS)
static struct zmk_widget_sync_battery_status sync_battery_widget;
#endif

static lv_obj_t *screen;
static lv_obj_t *splash;

#if LV_USE_IMAGE
#include "yuan_oled_art.h"
static lv_timer_t *splash_animation_timer;
static uint8_t splash_step;

static void animate_splash(lv_timer_t *timer) {
    (void)timer;
    ++splash_step;
    if (splash_step == 1) {
        lv_image_set_src(splash, &yuan_blink);
    } else if (splash_step == 2) {
        lv_image_set_src(splash, &yuan_heart);
    } else if (splash_step == 4) {
        lv_image_set_src(splash, &yuan_open);
    }
}
#endif

/* Compact three-column layout on the split peripheral OLED. */
#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_BATTERY_STATUS)
#include "yuan_status_font.h"

static void status_cell(lv_obj_t *obj, lv_align_t align, int32_t x,
                        int32_t width, lv_text_align_t text_align) {
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_MODE_CLIP);
    lv_obj_set_size(obj, width, 16);
    lv_obj_set_style_text_align(obj, text_align, LV_PART_MAIN);
    lv_obj_align(obj, align, x, 0);
}

/* Replace only selected glyphs; use Montserrat 12 fallback for the rest. */
static void mixed_status_cell(lv_obj_t *obj, lv_align_t align, int32_t x,
                              int32_t width, lv_text_align_t text_align,
                              const lv_font_t *font) {
    status_cell(obj, align, x, width, text_align);
    lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN);
}
#endif

static void show_status_screen(lv_timer_t *timer) {
    (void)timer;
#if LV_USE_IMAGE
    if (splash_animation_timer != NULL) {
        lv_timer_delete(splash_animation_timer);
        splash_animation_timer = NULL;
    }
#endif
    if (splash != NULL) {
        lv_obj_del(splash);
        splash = NULL;
    }

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BATTERY_STATUS)
    zmk_widget_battery_status_init(&battery_status_widget, screen);
#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_BATTERY_STATUS)
    mixed_status_cell(zmk_widget_battery_status_obj(&battery_status_widget),
                LV_ALIGN_TOP_RIGHT, -1, 51, LV_TEXT_ALIGN_RIGHT, &yuan_battery_font);
#else
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_RIGHT, 0, 0);
#endif
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_PERIPHERAL_STATUS)
    zmk_widget_peripheral_status_init(&peripheral_status_widget, screen);
#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_BATTERY_STATUS)
    mixed_status_cell(zmk_widget_peripheral_status_obj(&peripheral_status_widget),
                     LV_ALIGN_TOP_LEFT, 1, 39, LV_TEXT_ALIGN_LEFT, &yuan_link_font);
#else
    lv_obj_align(zmk_widget_peripheral_status_obj(&peripheral_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_set_style_text_font(zmk_widget_layer_status_obj(&layer_status_widget),
                               lv_theme_get_font_small(screen), LV_PART_MAIN);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_WPM_STATUS)
    zmk_widget_wpm_status_init(&wpm_status_widget, screen);
    lv_obj_align(zmk_widget_wpm_status_obj(&wpm_status_widget), LV_ALIGN_BOTTOM_RIGHT, 0, 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_CPI_STATUS)
    zmk_widget_cpi_status_init(&cpi_status_widget, screen);
    lv_obj_align(zmk_widget_cpi_status_obj(&cpi_status_widget), LV_ALIGN_TOP_MID, 0, 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_INDICATORS_STATUS)
    zmk_widget_sync_indicators_status_init(&sync_indicators_widget, screen);
#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_BATTERY_STATUS)
    status_cell(zmk_widget_sync_indicators_status_obj(&sync_indicators_widget),
                LV_ALIGN_TOP_MID, -6, 35, LV_TEXT_ALIGN_CENTER);
#else
    lv_obj_align(zmk_widget_sync_indicators_status_obj(&sync_indicators_widget),
                 LV_ALIGN_TOP_MID, 15, 0);
#endif
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_OUTPUT_STATUS)
    zmk_widget_sync_output_status_init(&sync_output_widget, screen);
#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_BATTERY_STATUS)
    mixed_status_cell(zmk_widget_sync_output_status_obj(&sync_output_widget),
                LV_ALIGN_BOTTOM_LEFT, 1, 39, LV_TEXT_ALIGN_LEFT, &yuan_link_font);
#else
    lv_obj_align(zmk_widget_sync_output_status_obj(&sync_output_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);
#endif
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_LAYER_STATUS)
    zmk_widget_sync_layer_status_init(&sync_layer_widget, screen);
#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_BATTERY_STATUS)
    status_cell(zmk_widget_sync_layer_status_obj(&sync_layer_widget),
                LV_ALIGN_BOTTOM_MID, -6, 35, LV_TEXT_ALIGN_CENTER);
#else
    lv_obj_align(zmk_widget_sync_layer_status_obj(&sync_layer_widget), LV_ALIGN_BOTTOM_MID, 15, 0);
#endif
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_SYNC_BATTERY_STATUS)
    zmk_widget_sync_battery_status_init(&sync_battery_widget, screen);
    mixed_status_cell(zmk_widget_sync_battery_status_obj(&sync_battery_widget),
                LV_ALIGN_BOTTOM_RIGHT, -1, 51, LV_TEXT_ALIGN_RIGHT, &yuan_battery_font);
#endif
}

lv_obj_t *zmk_display_status_screen() {
    screen = lv_obj_create(NULL);
#if LV_USE_IMAGE
    splash_step = 0;
    splash = lv_image_create(screen);
    lv_image_set_src(splash, &yuan_open);
    lv_obj_set_style_radius(splash, 0, LV_PART_MAIN);
    lv_obj_set_style_image_recolor(splash, lv_obj_get_style_text_color(screen, LV_PART_MAIN),
                                  LV_PART_MAIN);
    lv_obj_set_style_image_recolor_opa(splash, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(splash);
    splash_animation_timer = lv_timer_create(animate_splash, 1000, NULL);
#else
    splash = lv_label_create(screen);
    lv_obj_set_style_text_font(splash, lv_theme_get_font_large(screen), LV_PART_MAIN);
    lv_label_set_text(splash, "YUAN");
    lv_obj_center(splash);
#endif

    /* Keep original transition time for the split status synchronization. */
    lv_timer_t *timer = lv_timer_create(show_status_screen, 5000, NULL);
    if (timer != NULL) {
        lv_timer_set_repeat_count(timer, 1);
    } else {
        show_status_screen(NULL);
    }
    return screen;
}
