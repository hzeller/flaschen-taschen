// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
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

#include "led-flaschen-taschen.h"

#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <strings.h>

#include "led-strip.h"

CrateColumnFlaschenTaschen::CrateColumnFlaschenTaschen(spixels::LEDStrip *strip)
    : strip_(strip), height_(strip->count() / 25) {
}

CrateColumnFlaschenTaschen::~CrateColumnFlaschenTaschen() {
    delete strip_;
}

void CrateColumnFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width() || y < 0 || y >= height())
        return;

    const int crate = y / 5;
    y %= 5;
    const int pos = 25 * crate + kCrateMapping[4-y][x];
    strip_->SetPixel(pos, col.r, col.g, col.b);
}

