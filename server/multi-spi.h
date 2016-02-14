// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Copyright (C) 2016 Henner Zeller <h.zeller@acm.org>
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
#ifndef RPI_MULTI_SPI_H
#define RPI_MULTI_SPI_H

#include "ft-gpio.h"
#include <stddef.h>

// MultiSPI outputs multiple SPI streams in parallel on different GPIOs.
// The clock is on a single GPIO-pin. This way, we can transmit 25-ish
// SPI streams in parallel on a Pi with 40 IO pins.
// Current implementation assumes that all streams have the same amount
// of data.
// Also, there is no chip-select at this point (not needed for the LED strips).
class MultiSPI {
public:
    // Create MultiSPI that outputs clock on the "clock_gpio" pin. The
    // "serial_bytes_per_stream" define the length of the transmission in
    // each stream. This creates the necessary buffers for streams, all
    // initialized to zero.
    explicit MultiSPI(int clock_gpio);
    ~MultiSPI();

    // Bytes per stream.
    size_t serial_byte_size() const { return size_; }

    // Register a new data stream for the given GPIO. The SPI data is
    // sent with the common clock and this gpio pin.
    // Note, each channel might receive more bytes because they share the
    // same clock and it depends on varying length. The remaining bytes
    // are all zero though.
    bool RegisterDataGPIO(int gpio, size_t serial_byte_size);

    // Set data byte for given gpio channel at given position in the
    // stream. "pos" needs to be in range [0 .. serial_bytes_per_stream)
    // Data is sent with next Send().
    void SetBufferedByte(int data_gpio, size_t pos, uint8_t data);

    // Send data for all streams if there was any change.
    void SendBuffers();
    
private:
    void UpdateBufferSize(size_t s);

    ft::GPIO gpio_;
    const int clock_gpio_;
    size_t size_;
    uint32_t *gpio_data_;
};
#endif  // RPI_MULTI_SPI_H
