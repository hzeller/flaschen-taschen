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

#ifndef LED_FLASCHEN_TASCHEN_H_
#define LED_FLASCHEN_TASCHEN_H_

#include "flaschen-taschen.h"

#include <vector>
#include <string>

// Crate mapping. Strip position at kCrateMapping[4-y][x]
extern int kCrateMapping[5][5];
extern int luminance_cie1931(uint8_t output_bits, uint8_t gray_value);

class MultiSPI;

class ColumnAssembly : public FlaschenTaschen {
public:
    ColumnAssembly(MultiSPI *spi);
    ~ColumnAssembly();

    // Add column. Takes over ownership of column.
    // Columns have been added right to left, or, if standing
    // behind the display: leftmost column first.
    void AddColumn(FlaschenTaschen *taschen);

    int width() const { return width_; }
    int height() const { return height_; }

    void SetPixel(int x, int y, const Color &col);
    void Send();

private:
    MultiSPI *const spi_;
    std::vector<FlaschenTaschen*> columns_;
    int width_;
    int height_;
};

// SPI-based ws28 led driver. This represents one column. Unlike the
// final display, x-coordinates go right-to-left, and bottom to up.
// The final assembly will turn things around.
class WS2801FlaschenTaschen : public FlaschenTaschen {
public:
    WS2801FlaschenTaschen(MultiSPI *spi, int gpio, int crate_stack_height);

    int width() const { return 5; }
    int height() const { return height_; }

    void SetPixel(int x, int y, const Color &col);
    void Send() {}  // This happens in SPI sending.

private:
    MultiSPI *const spi_;
    const int gpio_pin_;
    const int height_;
};

// -- experimental other strips.

// Needs to be connected to GPIO 18 (pin 12 on RPi header)
class WS2811FlaschenTaschen : public FlaschenTaschen {
public:
    WS2811FlaschenTaschen(int crate_stack_height);
    virtual ~WS2811FlaschenTaschen();

    int width() const { return 5; }
    int height() const { return height_; }

    void SetPixel(int x, int y, const Color &col);
    void Send();

private:
    const int height_;
};

// CLK is GPIO 17 (RPI header pin 11). Rest of pin user chosen.
class LPD6803FlaschenTaschen : public FlaschenTaschen {
public:
    LPD6803FlaschenTaschen(MultiSPI *spi, int gpio, int crate_stack_height);
    virtual ~LPD6803FlaschenTaschen();

    int width() const { return 5; }
    int height() const { return height_; }

    void SetPixel(int x, int y, const Color &col);
    void Send() {}  // Done by ColumnAssembly

    void SetColorCorrect(float r, float g, float b) {
        r_correct = r; g_correct = g; b_correct = b;
    }
private:
    MultiSPI *const spi_;
    const int gpio_pin_;
    const int height_;
    float r_correct, g_correct, b_correct;
};

namespace rgb_matrix {
class RGBMatrix;
}

class RGBMatrixFlaschenTaschen : public FlaschenTaschen {
public:
    RGBMatrixFlaschenTaschen(int offset_x, int offset_y,
                             int width, int heigh);
    virtual ~RGBMatrixFlaschenTaschen();

    // Initialization that needs to be called after we have
    // become a daemon.
    void PostDaemonInit();

    int width() const { return width_; }
    int height() const { return height_; }

    void SetPixel(int x, int y, const Color &col);
    void Send() { }

private:
    const int off_x_;
    const int off_y_;
    const int width_;
    const int height_;

    rgb_matrix::RGBMatrix *matrix_;
};

class TerminalFlaschenTaschen : public FlaschenTaschen {
public:
    TerminalFlaschenTaschen(int width, int heigh);

    int width() const { return width_; }
    int height() const { return height_; }

    void SetPixel(int x, int y, const Color &col);
    void Send();

private:
    const int width_;
    const int height_;
    size_t initial_offset_;
    size_t pixel_offset_;
    bool is_first_;
    std::string buffer_;
};

#endif // LED_FLASCHEN_TASCHEN_H_
