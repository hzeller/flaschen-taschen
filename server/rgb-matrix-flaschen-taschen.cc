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

static rgb_matrix::GPIO gpio_s;

RGBMatrixFlaschenTaschen::RGBMatrixFlaschenTaschen(
    rgb_matrix::RGBMatrix *matrix, int width, int height) : matrix_(matrix) {
    if (matrix_ == NULL) {
        fprintf(stderr, "Couldn't initialize RGB matrix.\n");
        exit(1);
    }
    width_ = (width > 0) ? width : matrix_->width();
    height_ = (height > 0) ? height : matrix_->height();
    fprintf(stderr, "Running with %dx%d resolution\n", width_, height_);
}

RGBMatrixFlaschenTaschen::~RGBMatrixFlaschenTaschen() {
    delete matrix_;
}

void RGBMatrixFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    matrix_->SetPixel(x, y, col.r, col.g, col.b);
}

void RGBMatrixFlaschenTaschen::PostDaemonInit() {
    matrix_->StartRefresh();  // Starts thread.
}
