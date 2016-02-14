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

static rgb_matrix::GPIO gpio_s;

RGBMatrixFlaschenTaschen::RGBMatrixFlaschenTaschen(int offset_x, int offset_y,
                                                   int width, int height)
    : off_x_(offset_x), off_y_(offset_y), width_(width), height_(height),
      is_initialized_(false) {
    gpio_s.Init();
    matrix_ = new rgb_matrix::RGBMatrix(NULL, 32, 1, 2);
    matrix_->SetGPIO(&gpio_s, false);
}

RGBMatrixFlaschenTaschen::~RGBMatrixFlaschenTaschen() {
    delete matrix_;
}

void RGBMatrixFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (!is_initialized_) return;
    // Simulate spacing of crates (they need less)
#if 0
    x += x / 5;
    y += y / 5;
#endif
    //matrix_->SetPixel(x + off_x_, y + off_y_, col.r, col.g, col.b);
    matrix_->SetPixel(31 - y + off_y_, x + off_x_, col.r, col.g, col.b);
}

void RGBMatrixFlaschenTaschen::Send() {
    if (!is_initialized_) {
        matrix_->SetGPIO(&gpio_s, true);
        is_initialized_ = true;
        fprintf(stderr, "Initialized.\n");
    }
}
