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

class StackedFlaschenTaschen : public FlaschenTaschen {
public:
    StackedFlaschenTaschen(FlaschenTaschen *top, FlaschenTaschen *bottom)
        : top_(top), bottom_(bottom) {
    }

    int width() const { return top_->width(); }
    int height() const { return top_->height() + bottom_->height(); }

    void SetPixel(int x, int y, const Color &col) {
        if (y >= top_->height()) {
            bottom_->SetPixel(x, y - top_->height(), col);
        } else {
            top_->SetPixel(x, y, col);
        }
    }

    void Send() {
        top_->Send();
        bottom_->Send();
    }

private:
    FlaschenTaschen *const top_;
    FlaschenTaschen *const bottom_;
};

// Needs to be connected to GPIO 18 (pin 12 on RPi header)
class WS2811FlaschenTaschen : public FlaschenTaschen {
public:
    WS2811FlaschenTaschen(int width, int height);
    virtual ~WS2811FlaschenTaschen();

    int width() const { return width_; }
    int height() const { return height_; }

    void SetPixel(int x, int y, const Color &col);
    void Send();

private:
    const int width_;
    const int height_;
};

// CLK is GPIO 17 (RPI header pin 11). Rest of pin user chosen.
class LPD6803FlaschenTaschen : public FlaschenTaschen {
public:
    LPD6803FlaschenTaschen(int gpio, int width, int height);
    virtual ~LPD6803FlaschenTaschen();

    int width() const { return width_; }
    int height() const { return height_; }

    void SetPixel(int x, int y, const Color &col);
    void Send();

    void SetColorCorrect(float r, float g, float b) {
        r_correct = r; g_correct = g; b_correct = b;
    }
private:
    const int gpio_pin_;
    const int width_;
    const int height_;
    float r_correct, g_correct, b_correct;
};

#endif // LED_FLASCHEN_TASCHEN_H_
