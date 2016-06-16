// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Very simple graphics library to do simple things.
//
// Might be useful to consider using Cairo instead and just have an interface
// between that and the Canvas. Well, this is a quick set of things to get
// started (and nicely self-contained).
#ifndef RPI_GRAPHICS_H
#define RPI_GRAPHICS_H

#include "flaschen-taschen.h"

#include <map>
#include <stdint.h>

namespace ft {
// Font loading bdf files. If this ever becomes more types, just make virtual
// base class.
class Font {
public:
    // Initialize font, but it is only usable after LoadFont() has been called.
    Font();
    ~Font();

    bool LoadFont(const char *path);

    // Return height of font in pixels. Returns -1 if font has not been loaded.
    int height() const { return font_height_; }

    // Return baseline. Pixels from the topline to the baseline.
    int baseline() const { return base_line_; }

    // Return width of given character, or -1 if font is not loaded or character
    // does not exist.
    int CharacterWidth(uint32_t unicode_codepoint) const;

    // Draws the unicode character at position "x","y"
    // with "color" on "background_color" (background_color can be NULL for
    // transparency.
    // The "y" position is the baseline of the font.
    // If we don't have it in the font, draws the replacement character "�" if
    // available.
    // Returns how much we advance on the screen, which is the width of the
    // character or 0 if we didn't draw any chracter.
    int DrawGlyph(FlaschenTaschen *c, int x, int y,
                  const Color &color, const Color *background_color,
                  uint32_t unicode_codepoint) const;

    // Create a new font from this font, which represents an outline of the
    // original font.
    Font *CreateOutlineFont() const;

private:
    Font(const Font& x);  // No copy constructor. Use references or pointer instead.

    struct Glyph;
    typedef std::map<uint32_t, Glyph*> CodepointGlyphMap;

    const Glyph *FindGlyph(uint32_t codepoint) const;

    int font_height_;
    int base_line_;
    CodepointGlyphMap glyphs_;
};

// Draw text, encoded in UTF-8, with given "font" at "x","y" with "color".
// "background_color" can be NULL for transparency.
// The "y" position is the bottom of the font.
// "extra_spacing" allows for additional spacing between characters (can be
// negative)
// Returns how far we advance on the screen.
int DrawText(FlaschenTaschen *c, const Font &font, int x, int y,
             const Color &color, const Color *background_color,
             const char *utf8_text,
             int extra_spacing = 0);

// Draw text as above, but vertically, encoded in UTF-8, with given "font" at
// "x","y" with "color".
// "background_color" can be NULL for transparency.
// The "y" position is the baseline of the font.
// "extra_spacing" allows for additional spacing between characters (can be
// negative)
// Returns font height to advance up on the screen.
int VerticalDrawText(FlaschenTaschen *c, const Font &font, int x, int y,
                     const Color &color, const Color *background_color,
                     const char *utf8_text, int extra_spacing = 0);
}  // namespace ft

#endif  // RPI_GRAPHICS_H
