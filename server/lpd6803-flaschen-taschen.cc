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

LPD6803FlaschenTaschen::LPD6803FlaschenTaschen(MultiSPI *spi, int gpio,
                                               int crate_stack_height)
    : spi_(spi), gpio_pin_(gpio), height_(crate_stack_height * 5),
      r_correct(1), g_correct(1), b_correct(1) {
    // 2 byts per pixel, 4 bytes start sequence, 4 bytes end-sequence
    int width = 5;
    const size_t bytes_needed = 4 + 2 * width * height_ + 4;
    
    if (!spi_->RegisterDataGPIO(gpio_pin_, bytes_needed)) {
        fprintf(stderr, "Couldn't register gpio %d\n", gpio);
        assert(false);
    }

    // Four zero bytes as start-bytes for lpd6803
    spi_->SetBufferedByte(gpio_pin_, 0, 0x00);
    spi_->SetBufferedByte(gpio_pin_, 1, 0x00);
    spi_->SetBufferedByte(gpio_pin_, 2, 0x00);
    spi_->SetBufferedByte(gpio_pin_, 3, 0x00);

    // Initialize all the pixel-bits (upper bit per 16bit value set)
    Color black(0,0,0);
    for (int x = 0; x < 5; x++) {
        for (int y = 0; y < height_; ++y) {
            SetPixel(x, y, black);
        }
    }
}

LPD6803FlaschenTaschen::~LPD6803FlaschenTaschen() {}
        
void LPD6803FlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width() || y < 0 || y >= height())
        return;

    const int crate = y / 5;
    y = y % 5;
    int pos = 25 * crate + kCrateMapping[4-y][x];
    
    const int r = r_correct * col.r;
    const int g = g_correct * col.g;
    const int b = b_correct * col.b;
    uint16_t data = 0;
    data |= (1<<15);  // start bit
    data |= ((r >> 3) & 0x1F) << 10;
    data |= ((g >> 3) & 0x1F) <<  5;
    data |= ((b >> 3) & 0x1F) <<  0;

    spi_->SetBufferedByte(gpio_pin_, 2 * pos + 4 + 0, data >> 8);
    spi_->SetBufferedByte(gpio_pin_, 2 * pos + 4 + 1, data & 0xFF);
}
