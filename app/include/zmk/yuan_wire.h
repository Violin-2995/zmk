/* SPDX-License-Identifier: MIT
 * YUAN V3 alpha.4: portable policy and wire constants. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define YUAN_MAX_LAYERS 32u
#define YUAN_NAME_LEN 15u
#define YUAN_MAGIC 0xA4u
#define YUAN_PROTOCOL 2u
#define YUAN_SPECIAL 0x01u
#define YUAN_ENGLISH 0x02u
#define YUAN_CONFIRMED 0x04u
#define YUAN_VALID_PREFS (YUAN_SPECIAL | YUAN_ENGLISH)
#define YUAN_STATUS_SIZE 20u
#define YUAN_CATALOG_RECORD 16u
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

/* Strict UTF-8 scalar decoder. Return 0 for invalid or incomplete sequences;
 * never read beyond the supplied buffer. */
static inline size_t yuan_utf8_decode(const char *src, size_t remaining, uint32_t *cp) {
    if (!remaining) return 0;
    const uint8_t *s = (const uint8_t *)src;
    uint8_t b = s[0];
    if (b < 0x80) { *cp = b; return 1; }
    size_t n;
    uint32_t v;
    if (b >= 0xC2 && b <= 0xDF) { n = 2; v = b & 0x1Fu; }
    else if (b >= 0xE0 && b <= 0xEF) { n = 3; v = b & 0x0Fu; }
    else if (b >= 0xF0 && b <= 0xF4) { n = 4; v = b & 0x07u; }
    else return 0;
    if (remaining < n) return 0;
    for (size_t i = 1; i < n; ++i) {
        if ((s[i] & 0xC0u) != 0x80u) return 0;
        v = (v << 6) | (s[i] & 0x3Fu);
    }
    if ((n == 2 && v < 0x80u) || (n == 3 && v < 0x800u) ||
        (n == 4 && v < 0x10000u) || v > 0x10FFFFu ||
        (v >= 0xD800u && v <= 0xDFFFu)) return 0;
    *cp = v;
    return n;
}

/* Keep well-formed UTF-8 in the bounded display name, independent of glyph
 * availability. Replace controls/invalid input and never split a scalar.
 * Source is a NUL-terminated ZMK layer name (or bounded received name).
 * A visible '~' marks truncation; 5 ordinary Han characters fit exactly. */
static inline void yuan_name_copy(char out[YUAN_NAME_LEN + 1], const char *src) {
    memset(out, 0, YUAN_NAME_LEN + 1);
    if (!src) return;
    size_t remaining = strlen(src), used = 0, last_start = 0;
    while (remaining) {
        uint32_t cp = 0;
        size_t consumed = yuan_utf8_decode(src, remaining, &cp);
        bool replace = !consumed || cp < 32u || (cp >= 127u && cp <= 159u);
        if (!consumed) consumed = 1;
        size_t width = replace ? 1 : consumed;
        if (used + width > YUAN_NAME_LEN) {
            if (used == YUAN_NAME_LEN) used = last_start;
            memset(out + used, 0, YUAN_NAME_LEN + 1 - used);
            out[used] = '~';
            return;
        }
        last_start = used;
        if (replace) out[used] = '?';
        else memcpy(out + used, src, width);
        used += width;
        remaining -= consumed;
        src += consumed;
    }
}

static inline bool yuan_name_has_nonascii(const char *name) {
    if (!name) return false;
    for (; *name; ++name) if ((uint8_t)*name >= 0x80u) return true;
    return false;
}

/* Protocol 2: stable ID is the record slot. Byte 0 is layer order (255 =
 * absent); bytes 1..15 are bounded UTF-8, zero-padded when shorter.
 * No NUL is required for an exactly 15-byte name. */
static inline void yuan_catalog_record_write(uint8_t record[YUAN_CATALOG_RECORD],
                                             uint8_t index, const char *name) {
    char bounded[YUAN_NAME_LEN + 1];
    yuan_name_copy(bounded, name);
    record[0] = index;
    memcpy(record + 1, bounded, YUAN_NAME_LEN);
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
