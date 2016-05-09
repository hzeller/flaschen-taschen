// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>

#include "bdf-font.h"
#include "utf8-internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// The little question-mark box "�" for unknown code.
static const uint32_t kUnicodeReplacementCodepoint = 0xFFFD;

// Bitmap for one row. This limits the number of available columns.
// Make wider if running into trouble.
typedef uint32_t rowbitmap_t;

namespace ft {
struct Font::Glyph {
    int width, height;
    int bbx_w, bbx_h;  // bounding box width/height.
    int y_offset;
    rowbitmap_t bitmap[0];  // contains 'height' elements. Allocated.
};

Font::Font() : font_height_(-1), base_line_(0) {}
Font::~Font() {
    for (CodepointGlyphMap::iterator it = glyphs_.begin();
         it != glyphs_.end(); ++it) {
        free(it->second);
    }
}

// TODO: that might not be working for all input files yet.
bool Font::LoadFont(const char *path) {
    if (!path || !*path) return false;
    FILE *f = fopen(path, "r");
    if (f == NULL)
        return false;
    uint32_t codepoint;
    char buffer[1024];
    int dummy;
    Glyph tmp;
    Glyph *current_glyph = NULL;
    int row = 0;
    int x_offset = 0;
    int bitmap_shift = 0;
    while (fgets(buffer, sizeof(buffer), f)) {
        if (sscanf(buffer, "FONTBOUNDINGBOX %d %d %d %d",
                   &dummy, &font_height_, &dummy, &base_line_) == 4) {
            base_line_ += font_height_;
            tmp.height = font_height_;
        }
        else if (sscanf(buffer, "ENCODING %ud", &codepoint) == 1) {
            // parsed.
        }
        else if (sscanf(buffer, "DWIDTH %d", &tmp.width) == 1) {
            // parsed.
        }
        else if (sscanf(buffer, "BBX %d %d %d %d", &tmp.bbx_w, &tmp.bbx_h,
                        &x_offset, &tmp.y_offset) == 4) {
            size_t alloc_size = sizeof(Glyph) + tmp.height * sizeof(rowbitmap_t);
            current_glyph = (Glyph*) calloc(1, alloc_size);
            *current_glyph = tmp;
            // We only get number of bytes large enough holding our width.
            // We want it always left-aligned.
            bitmap_shift =
                8 * (sizeof(rowbitmap_t) - ((tmp.bbx_w + 7) / 8)) + x_offset;
            row = -1;  // let's not start yet, wait for BITMAP
        }
        else if (strncmp(buffer, "BITMAP", strlen("BITMAP")) == 0) {
            row = 0;
        }
        else if (current_glyph && row >= 0 && row < current_glyph->height
                 && (sscanf(buffer, "%x", &current_glyph->bitmap[row]) == 1)) {
            current_glyph->bitmap[row] <<= bitmap_shift;
            row++;
        }
        else if (strncmp(buffer, "ENDCHAR", strlen("ENDCHAR")) == 0) {
            if (current_glyph && row == current_glyph->height) {
                Glyph *old_glyph = glyphs_[codepoint];
                if (old_glyph) {
                    fprintf(stderr, "Doubly defined code-point %d\n", codepoint);
                    free(old_glyph);
                }
                glyphs_[codepoint] = current_glyph;
                current_glyph = NULL;
            }
        }
    }
    fclose(f);
    return true;
}

const Font::Glyph *Font::FindGlyph(uint32_t unicode_codepoint) const {
    CodepointGlyphMap::const_iterator found = glyphs_.find(unicode_codepoint);
    if (found == glyphs_.end())
        return NULL;
    return found->second;
}

int Font::CharacterWidth(uint32_t unicode_codepoint) const {
    const Glyph *g = FindGlyph(unicode_codepoint);
    return g ? g->width : -1;
}

int Font::DrawGlyph(FlaschenTaschen *c, int x_pos, int y_pos,
                    const Color &color, const Color *bgcolor,
                    uint32_t unicode_codepoint) const {
    const Glyph *g = FindGlyph(unicode_codepoint);
    if (g == NULL) g = FindGlyph(kUnicodeReplacementCodepoint);
    if (g == NULL) return 0;
    if (x_pos > 0 - g->width && x_pos < c->width()) { // clip.
        y_pos = y_pos - g->height - g->y_offset;
        for (int y = 0; y < g->height; ++y) {
            const rowbitmap_t row = g->bitmap[y];
            rowbitmap_t x_mask = 0x80000000;
            for (int x = 0; x < g->width; ++x, x_mask >>= 1) {
                if (row & x_mask) {
                    c->SetPixel(x_pos + x, y_pos + y, color);
                } else if (bgcolor) {
                    c->SetPixel(x_pos + x, y_pos + y, *bgcolor);
                }
            }
        }
    }
    return g->width;
}

int DrawText(FlaschenTaschen *c, const Font &font,
             int x, int y, const Color &color, const Color *background_color,
             const char *utf8_text) {
    const int start_x = x;
    while (*utf8_text) {
        const uint32_t cp = utf8_next_codepoint(utf8_text);
        x += font.DrawGlyph(c, x, y, color, background_color, cp);
    }
    return x - start_x;
}

int VerticalDrawText(FlaschenTaschen *c, const Font &font,
             int x, int y, const Color &color, const Color *background_color,
             const char *utf8_text) {
    const int start_y = y;
    while (*utf8_text) {
        const uint32_t cp = utf8_next_codepoint(utf8_text);
        font.DrawGlyph(c, x, y, color, background_color, cp);
        y += font.height() ;
    }
    return y - start_y;
}

}  // namespace ft
