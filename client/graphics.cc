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

#include "graphics.h"
#include "utf8-internal.h"

#include <stdlib.h>
#include <functional>
#include <algorithm>

namespace ft {
void DrawCircle(FlaschenTaschen *c, int x0, int y0, int radius,
                const Color &color) {
    int x = radius, y = 0;
    int radiusError = 1 - x;

    while (y <= x) {
        c->SetPixel(x + x0, y + y0, color);
        c->SetPixel(y + x0, x + y0, color);
        c->SetPixel(-x + x0, y + y0, color);
        c->SetPixel(-y + x0, x + y0, color);
        c->SetPixel(-x + x0, -y + y0, color);
        c->SetPixel(-y + x0, -x + y0, color);
        c->SetPixel(x + x0, -y + y0, color);
        c->SetPixel(y + x0, -x + y0, color);
        y++;
        if (radiusError<0){
            radiusError += 2 * y + 1;
        } else {
            x--;
            radiusError+= 2 * (y - x + 1);
        }
    }
}

void DrawLine(FlaschenTaschen *c, int x0, int y0, int x1, int y1,
              const Color &color) {
    int dy = y1 - y0, dx = x1 - x0, gradient, x, y, shift = 0x10;

    if (abs(dx) > abs(dy)) {
        // x variation is bigger than y variation
        if (x1 < x0) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        gradient = (dy << shift) / dx ;

        for (x = x0 , y = 0x8000 + (y0 << shift); x <= x1; ++x, y += gradient) {
            c->SetPixel(x, y >> shift, color);
        }
    } else if (dy != 0) {
        // y variation is bigger than x variation
        if (y1 < y0) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        gradient = (dx << shift) / dy;
        for (y = y0 , x = 0x8000 + (x0 << shift); y <= y1; ++y, x += gradient) {
            c->SetPixel(x >> shift, y, color);
        }
    } else {
        c->SetPixel(x0, y0, color);
    }
}

}  // namespace ft
