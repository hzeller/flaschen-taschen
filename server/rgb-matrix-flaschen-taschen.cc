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

#include "led-matrix.h"
#include "gpio.h"
#include <stdio.h>
#include <stdlib.h>

// We can simulate the spacing between crates by having an extra dark pixel
// in-between. It is not quite perfect, but the actual spacing on the real
// FlaschenTaschen crates is more like 3/4 of a bottle, but probably more
// 'honest' than ignoring the spacing.
#define CRATE_SPACING 0

static rgb_matrix::GPIO gpio_s;

RGBMatrixFlaschenTaschen::RGBMatrixFlaschenTaschen(rgb_matrix::RGBMatrix *matrix,
                                                   int offset_x, int offset_y,
                                                   int width, int height)
    : matrix_(matrix),
      off_x_(offset_x), off_y_(offset_y), width_(width), height_(height) {
    if (matrix_ == NULL) {
        fprintf(stderr, "Couldn't initialize RGB matrix.\n");
        exit(1);
    }
}

RGBMatrixFlaschenTaschen::~RGBMatrixFlaschenTaschen() {
    delete matrix_;
}

void RGBMatrixFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
#if CRATE_SPACING
    // Simulate spacing of crates. One extra pixel every 5 pixels.
    x += x / 5;
    y += y / 5;
#endif
    matrix_->SetPixel(x + off_x_, y + off_y_, col.r, col.g, col.b);
}

void RGBMatrixFlaschenTaschen::PostDaemonInit() {
    matrix_->StartRefresh();  // Starts thread.
}
