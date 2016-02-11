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

MultiSPI::MultiSPI(int clock_gpio, size_t serial_bytes) 
    : clock_gpio_(clock_gpio), size_(serial_bytes), any_change_(true),
      gpio_data_(new uint32_t[serial_bytes * 8]) {
    bool success = gpio_.Init();
    assert(success);  // gpio couldn't be initialized
    success = RegisterDataGPIO(clock_gpio);
    assert(success);  // clock pin not valid
    bzero(gpio_data_, serial_bytes * 8);
}

MultiSPI::~MultiSPI() { delete [] gpio_data_; }

bool MultiSPI::RegisterDataGPIO(int gpio) {
    return gpio_.AddOutput(gpio);
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

