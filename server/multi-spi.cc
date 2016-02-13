// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Copyright (C) 2013 Henner Zeller <h.zeller@acm.org>
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

#include "multi-spi.h"

#include <assert.h>
#include <strings.h>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>

MultiSPI::MultiSPI(int clock_gpio)
    : clock_gpio_(clock_gpio), size_(0), any_change_(true),
      gpio_data_(NULL) {
    bool success = gpio_.Init();
    assert(success);  // gpio couldn't be initialized
    success = gpio_.AddOutput(clock_gpio);
    assert(success);  // clock pin not valid
}

MultiSPI::~MultiSPI() {
    free(gpio_data_);
}

bool MultiSPI::RegisterDataGPIO(int gpio, size_t serial_byte_size) {
    UpdateBufferSize(serial_byte_size);
    return gpio_.AddOutput(gpio);
}

void MultiSPI::UpdateBufferSize(size_t requested_size) {
    if (requested_size == 0) return;
    // Each bit requires one word to allocate.
    const size_t memsize = sizeof(uint32_t) * requested_size * 8;
    if (gpio_data_ == NULL) {
        fprintf(stderr, "alloc %d \n", (int) memsize);
        gpio_data_ = (uint32_t*) malloc(memsize);
        bzero(gpio_data_, memsize);
        size_ = requested_size;
    } else if (requested_size > size_) {
        gpio_data_ = (uint32_t*) realloc(gpio_data_, memsize);
        const size_t oldsize = sizeof(uint32_t) * size_ * 8;
        bzero((uint8_t*)gpio_data_ + oldsize, memsize - oldsize);
        size_ = requested_size;
    }
}

void MultiSPI::SetBufferedByte(int data_gpio, size_t pos, uint8_t data) {
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

void MultiSPI::SendBuffers() {
    // Hack to work around that there might be multiple displays sending
    // this.
    if (!any_change_) return;
    const int kSlowdownGPIOFactor = 20;  // Janky connection currently.
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

