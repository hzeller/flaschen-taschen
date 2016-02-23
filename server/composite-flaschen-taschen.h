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

#ifndef COMPOSITE_FLASCHEN_TASCHEN_H_
#define COMPOSITE_FLASCHEN_TASCHEN_H_

#include "flaschen-taschen.h"

#include <vector>

// A composite screen allows a layered screen: The lowest layer is the
// background, but layers can be stacked, higher number is on front.
// In layers above the background, 'black' is seen as transparent. That way,
// it is easy to do independent overlays (announcement texts above background),
// or sprites.
class CompositeFlaschenTaschen : public FlaschenTaschen {
public:
    CompositeFlaschenTaschen(FlaschenTaschen *delegatee, int layers);
    ~CompositeFlaschenTaschen();

    virtual int width() const { return width_; }
    virtual int height() const { return height_; }

    virtual void SetPixel(int x, int y, const Color &col) {
        SetPixelInLayer(x, y, 0, col);
    }
    virtual void Send() { delegatee_->Send(); }

    // -- Layering implementation
    void SetPixelInLayer(int x, int y, int layer, const Color &col);

private:
    class ScreenBuffer;
    class ZBuffer;

    FlaschenTaschen *const delegatee_;
    const int width_;
    const int height_;

    std::vector<ScreenBuffer*> screens_;
    ZBuffer *z_buffer_;
};

#endif // COMPOSITE_FLASCHEN_TASCHEN_H_
