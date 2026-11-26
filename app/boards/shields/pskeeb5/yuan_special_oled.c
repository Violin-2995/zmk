/* SPDX-License-Identifier: MIT
 * PSKeeb5 left OLED, V3.0-alpha.4. Existing status widgets remain intact.
 * All LVGL access is from the existing display queue or LVGL timer.
 */
#include <zephyr/kernel.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zmk/yuan_special.h>
#include "yuan_title_bitmap.h"

#define YUAN_RX_TIMEOUT_MS 3400
static lv_obj_t *overlay, *id_label, *name_label, *eng_label, *shackle, *lock_body;
static lv_timer_t *refresh_timer;
static lv_obj_t *name_bitmap;
static uint8_t title_pixels[YUAN_TITLE_W * YUAN_TITLE_H];
static char rendered_name[YUAN_NAME_LEN + 1];
static const lv_image_dsc_t title_image = {
    .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_A8,
               .w = YUAN_TITLE_W, .h = YUAN_TITLE_H, .stride = YUAN_TITLE_W},
    .data_size = sizeof(title_pixels), .data = title_pixels,
};
static lv_obj_t *screen_parent;
static void create_overlay(void);
static struct zmk_sync_layer_state latest, drawn;
static int64_t last_rx;
static bool have_rx, shown;
static lv_color_t fg, bg;

static uint16_t text_width(const lv_font_t *font, const char *s) {
    uint16_t width = 0;
    for (size_t i = 0; s[i]; ++i)
        width += lv_font_get_glyph_width(font, (uint8_t)s[i], (uint8_t)s[i + 1]);
    return width;
}
static lv_obj_t *make_label(lv_obj_t *parent) {
    lv_obj_t *l = lv_label_create(parent);
    if (!l) return NULL;
    lv_obj_set_style_text_font(l, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, fg, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(l, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(l, 0, LV_PART_MAIN);
    lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_CLIP);
    return l;
}
static lv_obj_t *make_shape(lv_obj_t *parent) {
    lv_obj_t *o = lv_obj_create(parent);
    if (!o) return NULL;
    lv_obj_remove_style_all(o);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}
static void set_hidden(lv_obj_t *o, bool hidden) {
    if (hidden) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
}
static void render(void) {
    bool special = refresh_timer && have_rx && k_uptime_get() - last_rx <= YUAN_RX_TIMEOUT_MS &&
        latest.yuan_magic == YUAN_MAGIC && latest.yuan_id < YUAN_MAX_LAYERS &&
        (latest.yuan_flags & YUAN_SPECIAL);
    if (!special) {
        if (shown) set_hidden(overlay, true);
        shown = false;
        return;
    }
    if (!overlay) create_overlay();
    if (!overlay) return;
    if (shown && !memcmp(&latest, &drawn, sizeof(latest))) return;
    drawn = latest;
    char id[8], name[YUAN_NAME_LEN + 1];
    snprintf(id, sizeof(id), "L%02u", (unsigned)latest.yuan_id);
    yuan_name_copy(name, latest.yuan_name);
    if (!name[0]) snprintf(name, sizeof(name), "Layer %02u", (unsigned)latest.yuan_id);
    lv_label_set_text(id_label, id);
    bool eng = latest.yuan_flags & YUAN_ENGLISH;
    bool confirmed = eng && (latest.yuan_flags & YUAN_CONFIRMED);
    lv_label_set_text(eng_label, confirmed ? "ENG" : eng ? "ENG?" : "");
    set_hidden(shackle, !confirmed);
    set_hidden(lock_body, !confirmed);
    set_hidden(eng_label, !eng);
    if (confirmed) {
        int x = 126 - (int)text_width(&lv_font_montserrat_10, "ENG") - 10;
        lv_obj_set_pos(shackle, x + 1, 1);
        lv_obj_set_pos(lock_body, x, 5);
    }
    if (yuan_name_has_nonascii(name)) {
        if (!name_bitmap) {
            name_bitmap = lv_image_create(overlay);
            if (name_bitmap) {
                lv_image_set_src(name_bitmap, &title_image);
                lv_obj_set_style_image_recolor(name_bitmap, fg, LV_PART_MAIN);
                lv_obj_set_style_image_recolor_opa(name_bitmap, LV_OPA_COVER, LV_PART_MAIN);
                lv_obj_align(name_bitmap, LV_ALIGN_BOTTOM_MID, 0, -1);
            }
        }
        if (name_bitmap) {
            if (strcmp(rendered_name, name)) {
                yuan_title_render(title_pixels, name);
                memcpy(rendered_name, name, sizeof(rendered_name));
                lv_image_cache_drop(&title_image);
                lv_obj_invalidate(name_bitmap);
            }
            set_hidden(name_label, true);
            set_hidden(name_bitmap, false);
        } else {
            /* An allocation failure must not damage the ordinary screen. */
            lv_label_set_text(name_label, "Layer name?");
            lv_obj_set_style_text_font(name_label, &lv_font_montserrat_12, LV_PART_MAIN);
            lv_obj_align(name_label, LV_ALIGN_BOTTOM_MID, 0, 0);
            set_hidden(name_label, false);
        }
    } else {
        if (name_bitmap) set_hidden(name_bitmap, true);
        set_hidden(name_label, false);
        const lv_font_t *font = &lv_font_montserrat_12;
        const lv_font_t *choices[] = {&lv_font_montserrat_18, &lv_font_montserrat_16,
                                      &lv_font_montserrat_12};
        for (size_t i = 0; i < sizeof(choices)/sizeof(choices[0]); ++i) {
            if (choices[i]->line_height + lv_font_montserrat_10.line_height <= 32 && text_width(choices[i], name) <= 124) {
                font = choices[i]; break;
            }
        }
        if (text_width(font, name) > 124) {
            size_t n = strlen(name);
            while (n > 1 && text_width(font, name) > 124) {
                name[n - 2] = '~'; name[n - 1] = '\0'; --n;
            }
        }
        lv_obj_set_style_text_font(name_label, font, LV_PART_MAIN);
        lv_label_set_text(name_label, name);
        lv_obj_set_width(name_label, 124);
        lv_obj_set_height(name_label, LV_SIZE_CONTENT);
        lv_obj_align(name_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    set_hidden(overlay, false);
    shown = true;
}
static void timer_cb(lv_timer_t *timer) {
    (void)timer;
    render();
}
void yuan_special_oled_receive(const struct zmk_sync_layer_state *s) {
    latest = *s;
    latest.yuan_name[YUAN_NAME_LEN] = '\0';
    have_rx = true;
    last_rx = k_uptime_get();
    render();
}
static void create_overlay(void) {
    if (overlay || !screen_parent || !refresh_timer) return;
    lv_obj_t *parent = screen_parent;
    fg = lv_obj_get_style_text_color(parent, LV_PART_MAIN);
    bg = lv_obj_get_style_bg_color(parent, LV_PART_MAIN);
    overlay = make_shape(parent);
    if (!overlay) return;
    lv_obj_set_size(overlay, 128, 32);
    lv_obj_align(overlay, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(overlay, bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, LV_PART_MAIN);
    id_label = make_label(overlay);
    eng_label = make_label(overlay);
    name_label = make_label(overlay);
    shackle = make_shape(overlay);
    lock_body = make_shape(overlay);
    if (!id_label || !eng_label || !name_label || !shackle || !lock_body) {
        lv_obj_del(overlay);
        overlay = NULL;
        return;
    }
    lv_obj_align(id_label, LV_ALIGN_TOP_LEFT, 2, 0);
    lv_obj_set_width(eng_label, 43);
    lv_obj_set_style_text_align(eng_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_align(eng_label, LV_ALIGN_TOP_RIGHT, -2, 0);
    lv_obj_set_style_text_align(name_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_size(shackle, 6, 7);
    lv_obj_set_style_radius(shackle, 3, LV_PART_MAIN);
    lv_obj_set_style_border_width(shackle, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(shackle, fg, LV_PART_MAIN);
    lv_obj_set_size(lock_body, 8, 6);
    lv_obj_set_style_radius(lock_body, 1, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lock_body, fg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lock_body, LV_OPA_COVER, LV_PART_MAIN);
    set_hidden(overlay, true);
}
void yuan_special_oled_init(lv_obj_t *parent) {
    if (screen_parent) return;
    screen_parent = parent;
    refresh_timer = lv_timer_create(timer_cb, 200, NULL);
    /* If timer allocation fails, keep the ordinary screen. */
    if (refresh_timer) render();
}
