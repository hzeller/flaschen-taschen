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
#include "rpi-dma.h"

#include <stddef.h>

// MultiSPI outputs multiple SPI streams in parallel on different GPIOs.
// The clock is on a single GPIO-pin. This way, we can transmit 25-ish
// SPI streams in parallel on a Pi with 40 IO pins.
// Current implementation assumes that all streams have the same amount
// of data.
// Also, there is no chip-select at this point (not needed for the LED strips).
class MultiSPI {
public:
    // Names of the pin-headers on the breakout board.
    enum {
        SPI_CLOCK = 27,

        SPI_P6  = 23,
        SPI_P7  = 17,
        SPI_P8  =  4,
        SPI_P9  = 14,  // This will be 18 with the new board.

        SPI_P10 =  5,
        SPI_P11 = 25,
        SPI_P12 = 24,
        SPI_P13 = 22,

        SPI_P14 = 16,
        SPI_P15 = 13,
        SPI_P16 =  6,
        SPI_P17 = 12,

        SPI_P18 = 21,
        SPI_P19 = 20,
        SPI_P20 = 26,
        SPI_P21 = 19,
    };

    // Create MultiSPI that outputs clock on the "clock_gpio" pin. The
    // "serial_bytes_per_stream" define the length of the transmission in
    // each stream. This creates the necessary buffers for streams, all
    // initialized to zero.
    explicit MultiSPI(int clock_gpio = SPI_CLOCK);
    ~MultiSPI();

    // Register a new data stream for the given GPIO. The SPI data is
    // sent with the common clock and this gpio pin.
    //
    // Note, each channel might receive more bytes because they share the
    // same clock with everyone and it depends on what is the longest requested
    // length.
    // Overlength transmission bytes are all zero.
    bool RegisterDataGPIO(int gpio, size_t serial_byte_size);

    // This needs to be called after all RegisterDataGPIO have been called.
    void FinishRegistration();

    // Set data byte for given gpio channel at given position in the
    // stream. "pos" needs to be in range [0 .. serial_bytes_per_stream)
    // Data is sent with next Send().
    void SetBufferedByte(int data_gpio, size_t pos, uint8_t data);

    // Send data for all streams. Wait for completion.
    void SendBuffers();
    
private:
    struct GPIOData;
    ft::GPIO gpio_;
    const int clock_gpio_;
    size_t size_;

    struct UncachedMemBlock alloced_;
    GPIOData *gpio_ops_;
    struct dma_cb* start_block_;
    struct dma_channel_header* dma_channel_;
};
#endif  // RPI_MULTI_SPI_H
