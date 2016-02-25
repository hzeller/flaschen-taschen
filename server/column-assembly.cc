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

#include <unistd.h>
#include "multi-spi.h"

ColumnAssembly::ColumnAssembly(MultiSPI *spi)
    : spi_(spi), width_(0), height_(0)  {}

ColumnAssembly::~ColumnAssembly() {
    for (size_t i = 0; i < columns_.size(); ++i)
        delete columns_[i];
}

void ColumnAssembly::AddColumn(FlaschenTaschen *taschen) {
    columns_.push_back(taschen);
    width_ += 5;
    height_ = std::max(height_, taschen->height());
}

void ColumnAssembly::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width() || y < 0 || y >= height())
        return;
    const int crate_from_left = x / 5;
    FlaschenTaschen *column = columns_[columns_.size() - crate_from_left - 1];

    // Our physical display has the (0,0) at the bottom right corner.
    // Flip it around.
    column->SetPixel(4 - x % 5, height() - y - 1, col);
}

void ColumnAssembly::Send() {
    for (size_t i = 0; i < columns_.size(); ++i) {
        columns_[i]->Send();
    }
    spi_->SendBuffers();
    usleep(1 * 1000);  // Some strips need some low 
}
