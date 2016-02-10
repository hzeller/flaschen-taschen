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

#ifndef FLASCHEN_TASCHEN_H_
#define FLASCHEN_TASCHEN_H_

#include <stdint.h>

struct Color {
    Color() {}
    Color(int rr, int gg, int bb) : r(rr), g(gg), b(bb){}

    uint8_t r;
    uint8_t g;
    uint8_t b;
} __attribute((packed));

// We have multiple implementations for FlaschenTaschen, using the
// same general interface on server and client. Essentially a canvas.
class FlaschenTaschen {
public:
    virtual ~FlaschenTaschen(){}

    virtual int width() const = 0;
    virtual int height() const = 0;

    virtual void SetPixel(int x, int y, const Color &col) = 0;
    virtual void Send() = 0;
};

#endif // FLASCHEN_TASCHEN_H_
