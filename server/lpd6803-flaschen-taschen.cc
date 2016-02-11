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

// Pin 11 on Pi
#define MULTI_SPI_COMMON_CLOCK 17

// Shared singleton. First display initializes it.
static MultiSPI *multi_spi = NULL;

LPD6803FlaschenTaschen::LPD6803FlaschenTaschen(int gpio, int width, int height)
    : gpio_pin_(gpio), width_(width), height_(height),
      r_correct(1), g_correct(1), b_correct(1) {
    // 2 byts per pixel, 4 bytes start sequence, 4 bytes end-sequence
    const size_t bytes_needed = 4 + 2 * width * height + 4;
    // TODO(hzeller): Initialization should happen somewhere in main.
    if (multi_spi == NULL) {
        multi_spi = new MultiSPI(MULTI_SPI_COMMON_CLOCK, bytes_needed);
    } else {
        // The MultiSPI is initialized by first, but we need to have space for
        // longest strip. Just make sure that all are the same length for now.
        assert(multi_spi->serial_byte_size() == bytes_needed);
    }
    if (!multi_spi->RegisterDataGPIO(gpio_pin_)) {
        fprintf(stderr, "Couldn't register gpio %d\n", gpio);
        assert(false);
    }
    // Four zero bytes as start-bytes for lpd6803
    multi_spi->SetBufferedByte(gpio_pin_, 0, 0x00);
    multi_spi->SetBufferedByte(gpio_pin_, 1, 0x00);
    multi_spi->SetBufferedByte(gpio_pin_, 2, 0x00);
    multi_spi->SetBufferedByte(gpio_pin_, 3, 0x00);

    // Initialize all the pixel-bits (upper bit per 16bit value set)
    Color black(0,0,0);
    for (int x = 0; x < width_; x++) {
        for (int y = 0; y < height_; ++y) {
            SetPixel(x, y, black);
        }
    }
}

LPD6803FlaschenTaschen::~LPD6803FlaschenTaschen() {}
        
void LPD6803FlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width() || y < 0 || y >= height())
        return;

    // Our boxes are upside down somewhat currently. Correct :)
    x = width() - x - 1;
    y = height() - y - 1;

    // Zig-zag assignment of our strips, so every other column has the
    // y-offset reverse.
    int y_off = y % height_;
    int pos = (x * height_) + ((x % 2 == 0) ? y_off : (height_-1) - y_off);
    
    const int r = r_correct * col.r;
    const int g = g_correct * col.g;
    const int b = b_correct * col.b;
    uint16_t data = 0;
    data |= (1<<15);  // start bit
    data |= ((r >> 3) & 0x1F) << 10;
    data |= ((g >> 3) & 0x1F) <<  5;
    data |= ((b >> 3) & 0x1F) <<  0;

    multi_spi->SetBufferedByte(gpio_pin_, 2 * pos + 4 + 0, data >> 8);
    multi_spi->SetBufferedByte(gpio_pin_, 2 * pos + 4 + 1, data & 0xFF);
}

void LPD6803FlaschenTaschen::Send() {
    multi_spi->SendBuffers();
}
