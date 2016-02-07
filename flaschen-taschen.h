// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#ifndef FLASCHEN_TASCHEN_H_
#define FLASCHEN_TASCHEN_H_

#include <stdint.h>
#include "ws2811.h"

struct Color {
    Color() {}
    Color(int rr, int gg, int bb) : r(rr), g(gg), b(bb){}

    uint8_t r;
    uint8_t g;
    uint8_t b;
};

// We use multiple hardware implementations for FlaschenTaschen. This
// is the general interface.
class FlaschenTaschen {
public:
    virtual ~FlaschenTaschen(){}

    virtual int width() const = 0;
    virtual int height() const = 0;

    virtual void SetPixel(int x, int y, const Color &col) = 0;
    virtual void Send() = 0;
};

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

#endif // FLASCHEN_TASCHEN_H_
