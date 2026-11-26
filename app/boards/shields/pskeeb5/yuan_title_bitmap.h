/* SPDX-License-Identifier: MIT
 * Portable 1-bit glyph to A8 OLED title renderer. Left shield only.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <zmk/yuan_wire.h>
#include "yuan_title_glyphs.inc"

#define YUAN_TITLE_W 124u
#define YUAN_TITLE_H 16u

static inline size_t yuan_glyph_find(uint32_t cp) {
    size_t lo=0, hi=YUAN_GLYPH_COUNT;
    while(lo<hi) {
        size_t mid=lo+(hi-lo)/2;
        if(yuan_glyph_code[mid]<cp) lo=mid+1;
        else hi=mid;
    }
    return lo<YUAN_GLYPH_COUNT && yuan_glyph_code[lo]==cp ? lo : YUAN_GLYPH_COUNT;
}
static inline uint8_t yuan_glyph_width(uint32_t cp) {
    return cp<128u ? 8u : 16u;
}

/* Render the same bounded, well-formed name used by the wire protocol.
 * ASCII takes 8 pixels, other scalars 16. A 15-byte UTF-8 name can occupy
 * at most 120 pixels, so even mixed strings fit the 124-pixel title strip.
 * Unsupported scalars receive a visible square, never a broken UTF-8 byte.
 */
static inline size_t yuan_title_render(uint8_t pixels[YUAN_TITLE_W*YUAN_TITLE_H],
                                       const char *name) {
    char bounded[YUAN_NAME_LEN+1];
    yuan_name_copy(bounded,name);
    uint32_t cps[YUAN_NAME_LEN];
    size_t count=0,width=0,remaining=strlen(bounded);
    const char *p=bounded;
    while(remaining && count<YUAN_NAME_LEN) {
        uint32_t cp;
        size_t n=yuan_utf8_decode(p,remaining,&cp);
        if(!n) { cp='?'; n=1; }
        uint8_t advance=yuan_glyph_width(cp);
        if(width+advance>YUAN_TITLE_W) break;
        cps[count++]=cp;
        width+=advance;
        p+=n; remaining-=n;
    }
    memset(pixels,0,YUAN_TITLE_W*YUAN_TITLE_H);
    size_t x=(YUAN_TITLE_W-width)/2;
    for(size_t i=0;i<count;++i) {
        uint32_t cp=cps[i];
        uint8_t advance=yuan_glyph_width(cp);
        size_t index=yuan_glyph_find(cp);
        if(index==YUAN_GLYPH_COUNT)
            index=yuan_glyph_find(cp<128u ? '?' : 0x25A1u);
        if(index<YUAN_GLYPH_COUNT) {
            for(size_t y=0;y<YUAN_TITLE_H;++y)
                for(size_t j=0;j<advance;++j)
                    if(yuan_glyph_rows[index][y] & (UINT16_C(0x8000)>>j))
                        pixels[y*YUAN_TITLE_W+x+j]=255u;
        }
        x+=advance;
    }
    return width;
}
