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

#include "multi-spi.h"

WS2801FlaschenTaschen::WS2801FlaschenTaschen(MultiSPI *multi_spi,
                                             int gpio, int crates)
    : spi_(multi_spi), gpio_pin_(gpio), height_(crates * 5) {
    multi_spi->RegisterDataGPIO(gpio, height_ * 5 * 3);
}

void WS2801FlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width() || y < 0 || y >= height())
        return;

    const int crate = y / 5;
    y %= 5;
    const int pos = 25 * crate + kCrateMapping[4-y][x];

    spi_->SetBufferedByte(gpio_pin_, 3 * pos + 0, col.r);
    spi_->SetBufferedByte(gpio_pin_, 3 * pos + 1, col.g);
    spi_->SetBufferedByte(gpio_pin_, 3 * pos + 2, col.b);
}

