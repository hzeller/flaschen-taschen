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

namespace spixels {
class MultiSPI;
class LEDStrip;
}

// Column helps assembling the various columns of width 5 (the width of a crate)
// to one big display. Since all SPI based strips are necessary upated in
// parallel, that SPI send command is triggered within here.
class ColumnAssembly : public FlaschenTaschen {
public:
    ColumnAssembly(spixels::MultiSPI *spi);
    ~ColumnAssembly();

    // Add column. Takes over ownership of column.
    // Columns have been added right to left, or, if standing
    // behind the display: leftmost column first.
    void AddColumn(FlaschenTaschen *taschen);
    void PostDaemonInit();

    int width() const { return width_; }
    int height() const { return height_; }

    void SetPixel(int x, int y, const Color &col);
    void Send();

private:
    spixels::MultiSPI *const spi_;
    std::vector<FlaschenTaschen*> columns_;
    int width_;
    int height_;
};

// This represents one column. Unlike the final display,
// x-coordinates go right-to-left, and bottom to up.
// The final assembly will turn things around.
class CrateColumnFlaschenTaschen : public FlaschenTaschen {
public:
    // Given a simple LED strip, create a column of crates that behaves
    // the way we are snaking the strip.
    // Takes ownership of the LED strip.
    CrateColumnFlaschenTaschen(spixels::LEDStrip *strip);
    ~CrateColumnFlaschenTaschen();

    int width() const { return 5; }
    int height() const { return height_; }

    void SetPixel(int x, int y, const Color &col);
    void Send() {}  // This happens in SPI sending in ColumnAssembly

private:
    spixels::LEDStrip *strip_;
    const int height_;
};

// -- FlaschenTaschen implementation using rpi-rgb-led-matrix
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
    void Send() { /* update directly */ }

private:
    const int off_x_;
    const int off_y_;
    const int width_;
    const int height_;

    rgb_matrix::RGBMatrix *matrix_;
};

class TerminalFlaschenTaschen : public FlaschenTaschen {
public:
    TerminalFlaschenTaschen(int terminal_fd, int width, int heigh);
    virtual ~TerminalFlaschenTaschen();
    void PostDaemonInit() {}

    int width() const { return width_; }
    int height() const { return height_; }

    void SetPixel(int x, int y, const Color &col);
    void Send();

private:
    const int terminal_fd_;
    const int width_;
    const int height_;
    size_t initial_offset_;
    size_t pixel_offset_;
    size_t fps_offset_;
    bool is_first_;
    Color *buffer_;
    int64_t last_time_;
};

#endif // LED_FLASCHEN_TASCHEN_H_
