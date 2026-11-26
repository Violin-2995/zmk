/* SPDX-License-Identifier: MIT
 * YUAN V3 alpha.2: portable policy and wire constants. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define YUAN_MAX_LAYERS 32u
#define YUAN_NAME_LEN 12u
#define YUAN_MAGIC 0xA3u
#define YUAN_PROTOCOL 1u
#define YUAN_SPECIAL 0x01u
#define YUAN_ENGLISH 0x02u
#define YUAN_CONFIRMED 0x04u
#define YUAN_VALID_PREFS (YUAN_SPECIAL | YUAN_ENGLISH)
#define YUAN_STATUS_SIZE 20u
#define YUAN_CATALOG_RECORD 15u
#define YUAN_INVALID_ID 255u

struct yuan_policy {
    uint32_t available, active, special, english;
    uint8_t display_id;
};

static inline uint32_t yuan_bit(uint8_t id) {
    return id < YUAN_MAX_LAYERS ? (UINT32_C(1) << id) : 0;
}

/* order lists IDs from lowest to highest priority, with 255 for absent slots. */
static inline struct yuan_policy yuan_select(const uint8_t *order, size_t count,
                                             uint32_t active, const uint8_t *prefs) {
    struct yuan_policy p = {.display_id = YUAN_INVALID_ID};
    for (size_t i = 0; i < count; ++i) {
        uint8_t id = order[i];
        if (id >= YUAN_MAX_LAYERS) continue;
        uint32_t bit = yuan_bit(id);
        p.available |= bit;
        if (prefs[id] & YUAN_SPECIAL) p.special |= bit;
        if (prefs[id] & YUAN_ENGLISH) p.english |= bit;
        if ((active & bit) && (prefs[id] & YUAN_SPECIAL)) p.display_id = id;
    }
    p.active = active & p.available;
    return p;
}

/* ASCII-only OLED representation. Never cuts through a UTF-8 codepoint:
 * unsupported bytes become '?' and continuation bytes are skipped. */
static inline void yuan_name_copy(char out[YUAN_NAME_LEN + 1], const char *src) {
    size_t n = 0;
    memset(out, 0, YUAN_NAME_LEN + 1);
    if (!src) return;
    while (*src && n < YUAN_NAME_LEN) {
        unsigned char c = (unsigned char)*src++;
        if ((c & 0xC0u) == 0x80u) continue;
        out[n++] = (c >= 32 && c <= 126) ? (char)c : '?';
    }
}

static inline bool yuan_should_lock(struct yuan_policy p, bool selected,
                                    bool fresh, bool paused) {
    return selected && fresh && !paused && (p.active & p.english) != 0;
}

static inline uint8_t yuan_screen_flags(struct yuan_policy p, bool confirmed) {
    if (p.display_id == YUAN_INVALID_ID) return 0;
    uint8_t f = YUAN_SPECIAL;
    if (p.active & p.english) f |= YUAN_ENGLISH;
    if ((f & YUAN_ENGLISH) && confirmed) f |= YUAN_CONFIRMED;
    return f;
}
