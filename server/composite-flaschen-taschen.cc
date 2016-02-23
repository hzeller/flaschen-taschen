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

#include "composite-flaschen-taschen.h"

#include <assert.h>
#include <vector>
#include <unistd.h>
#include <strings.h>

namespace {
// A two-dimensional array, essentially.
template <class T> class TypedScreenBuffer {
public:
    TypedScreenBuffer(int width, int height)
        : width_(width), height_(height),
          screen_(new T [ width * height]) {
        bzero(screen_, width * height * sizeof(T));
    }

    ~TypedScreenBuffer() { delete [] screen_; }

    T &At(int x, int y) { return screen_[y * width_ + x]; }

private:
    const int width_;
    const int height_;
    T *const screen_;
};
}  // namespace

class CompositeFlaschenTaschen::ScreenBuffer : public TypedScreenBuffer<Color> {
public:
    ScreenBuffer(int w, int h) : TypedScreenBuffer(w, h){}
};

class CompositeFlaschenTaschen::ZBuffer : public TypedScreenBuffer<int> {
public:
    ZBuffer(int w, int h) : TypedScreenBuffer(w, h){}
};

CompositeFlaschenTaschen::CompositeFlaschenTaschen(FlaschenTaschen *delegatee,
                                                   int layers) 
    : delegatee_(delegatee),
      width_(delegatee->width()), height_(delegatee->height()),
      z_buffer_(new ZBuffer(width_, height_)) {
    assert(layers < 32);  // this might be pretty slow.
    for (int i = 0; i < layers; ++i) {
        screens_.push_back(new ScreenBuffer(delegatee->width(),
                                            delegatee->height()));
    }
}

CompositeFlaschenTaschen::~CompositeFlaschenTaschen() {
    for (size_t i = 0; i < screens_.size(); ++i) delete screens_[i];
    delete z_buffer_;
}

// TODO: garbage collect layers > 0 that have not been updated for more than
// 30 seconds or so to prevent permanently covering up stuff.
void CompositeFlaschenTaschen::SetPixelInLayer(int x, int y,
                                               int layer, const Color &col) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    if (layer < 0) layer = 0;
    if (layer >= (int)screens_.size()) layer = screens_.size() - 1;
    screens_[layer]->At(x, y) = col;
    
    if (layer >= z_buffer_->At(x, y)) {
        if (col.is_black()) {
            // transparent pixel. Find next below us that is not.
            for (/**/; layer > 0; --layer) {
                if (!screens_[layer]->At(x, y).is_black())
                    break;
            }
            delegatee_->SetPixel(x, y, screens_[layer]->At(x, y));
        } else {
            delegatee_->SetPixel(x, y, col);
        }
        z_buffer_->At(x, y) = layer;
    }
}

