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

#include "flaschen-taschen.h"

#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <strings.h>

#include "ft-gpio.h"

// Pin 11 on Pi
#define MULTI_SPI_COMMON_CLOCK 17

namespace {
// MultiSPI essentially allows 1 clock line and a set of parallel
// data lines to send to a multitude of serial devices in parallel. This way
// we can have multiple shorter LPD6803 updated quickly.
//
// Multi-SPI is a shared resource.
class MultiSPI {
public:
    MultiSPI(int clock_gpio, size_t serial_bytes) 
        : clock_gpio_(clock_gpio), size_(serial_bytes), any_change_(true),
          gpio_data_(new uint32_t[serial_bytes * 8]) {
        bool success = gpio_.Init();
        assert(success);  // gpio couldn't be initialized
        success = RegisterDataGPIO(clock_gpio);
        assert(success);  // clock pin not valid
        bzero(gpio_data_, serial_bytes * 8);
    }

    ~MultiSPI() { delete [] gpio_data_; }

    size_t serial_byte_size() { return size_; }
    
    bool RegisterDataGPIO(int gpio) {
        return gpio_.AddOutput(gpio);
    }

    void SetSerialBits(int data_gpio, size_t pos, uint8_t data) {
        assert(pos < size_);
        uint32_t *buffer_pos = gpio_data_ + 8 * pos;
        for (uint8_t bit = 0x80; bit; bit >>= 1, buffer_pos++) {
            if (data & bit) {   // set
                *buffer_pos |= (1 << data_gpio);
            } else {            // reset
                *buffer_pos &= ~(1 << data_gpio);
            }
        }
        any_change_ = true;
    }

    void Send() {
        if (!any_change_) return;
        const int kSlowdownGPIOFactor = 5;
        uint32_t *end = gpio_data_ + 8 * size_;
        for (uint32_t *data = gpio_data_; data < end; ++data) {
            uint32_t d = *data;
            // Writing twice to slow down a little.
            for (int i = 0; i < kSlowdownGPIOFactor; ++i) gpio_.Write(d);
            d |= (1 << clock_gpio_);   // pos clock edge.
            for (int i = 0; i < kSlowdownGPIOFactor; ++i) gpio_.Write(d);
        }
        gpio_.Write(0);  // Reset clock.
        any_change_ = false;
    }
    
private:
    ft::GPIO gpio_;
    const int clock_gpio_;
    const size_t size_;
    bool any_change_;
    uint32_t *gpio_data_;
};
}

// Shared singleton. First display initializes it.
static MultiSPI *multi_spi = NULL;

LPD6803FlaschenTaschen::LPD6803FlaschenTaschen(int gpio, int width, int height)
    : gpio_pin_(gpio), width_(width), height_(height),
      r_correct(1), g_correct(1), b_correct(1) {
    // 2 byts per pixel, 4 bytes start sequence, 4 bytes end-sequence
    const size_t bytes_needed = 4 + 2 * width * height + 4;
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
    multi_spi->SetSerialBits(gpio_pin_, 0, 0x00);
    multi_spi->SetSerialBits(gpio_pin_, 1, 0x00);
    multi_spi->SetSerialBits(gpio_pin_, 2, 0x00);
    multi_spi->SetSerialBits(gpio_pin_, 3, 0x00);

    // Initialize all the start bits.
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

    multi_spi->SetSerialBits(gpio_pin_, 2 * pos + 4 + 0, data >> 8);
    multi_spi->SetSerialBits(gpio_pin_, 2 * pos + 4 + 1, data & 0xFF);
}

void LPD6803FlaschenTaschen::Send() {
    multi_spi->Send();
}
