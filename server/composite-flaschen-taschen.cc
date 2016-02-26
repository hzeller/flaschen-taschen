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
#include <limits.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>

#include <vector>

#include "ft-thread.h"

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

class CompositeFlaschenTaschen::GarbageCollector : public ft::Thread {
public:
    GarbageCollector(CompositeFlaschenTaschen *owner, ft::Mutex *m,
                     Ticks max_age)
        : owner_(owner), lock_(m), max_age_(max_age),
          running_(true), ticks_(0) {}

    void Run() {
        for (;;) {
            {
                ft::MutexLock m(lock_);
                if (!running_) break;
                owner_->ClearLayersOlderThan(ticks_ - max_age_);
                owner_->SetTimeTicks(ticks_);
            }
            sleep(1);
            ++ticks_;  // one tick, roughly one second.
        }
    }

    void SetStopped() {
        ft::MutexLock m(lock_);
        running_ = false;
    }

private:
    CompositeFlaschenTaschen *const owner_;
    ft::Mutex *const lock_;
    const Ticks max_age_;
    bool running_;
    Ticks ticks_;
};

// We need to do the display update in a high priority thread that
// shall not be interrupted as the LED strips trigger with a 50usec pause.
class CompositeFlaschenTaschen::DisplayUpdater : public ft::Thread {
public:
    DisplayUpdater(FlaschenTaschen *display) : display_(display),
                                               state_(READY_TO_SEND) {
        pthread_cond_init(&send_state_change_, NULL);
    }

    virtual void Run() {
        for (;;) {
            ft::MutexLock l(&mutex_);
            while (state_ != SEND_QUEUED) {
                mutex_.WaitOn(&send_state_change_);
            }
            display_->Send();
            state_ = READY_TO_SEND;
            pthread_cond_signal(&send_state_change_);
        }
    }

    void TriggerSend() {
        ft::MutexLock l(&mutex_);
        while (state_ != READY_TO_SEND) {
            mutex_.WaitOn(&send_state_change_);
        }
        state_ = SEND_QUEUED;
        pthread_cond_signal(&send_state_change_);
        while (state_ != READY_TO_SEND) {
            mutex_.WaitOn(&send_state_change_);
        }
    }

private:
    FlaschenTaschen *const display_;
    enum State {
        SEND_QUEUED,
        READY_TO_SEND,
    } state_;
    pthread_cond_t send_state_change_;
    ft::Mutex mutex_;
};

CompositeFlaschenTaschen::CompositeFlaschenTaschen(FlaschenTaschen *delegatee,
                                                   int layers) 
    : delegatee_(delegatee),
      width_(delegatee->width()), height_(delegatee->height()),
      current_layer_(0),
      z_buffer_(new ZBuffer(width_, height_)),
      garbage_collect_(NULL),
      display_updater_(new DisplayUpdater(delegatee_)) {
    assert(layers < 32);  // otherwise could getting slow.
    for (int i = 0; i < layers; ++i) {
        screens_.push_back(new ScreenBuffer(delegatee->width(),
                                            delegatee->height()));
        last_layer_update_time_.push_back(INT_MAX);
    }

    display_updater_->Start(99, 1<<3);
}

CompositeFlaschenTaschen::~CompositeFlaschenTaschen() {
    if (garbage_collect_) {
        garbage_collect_->SetStopped();
        garbage_collect_->WaitStopped();
    }
    for (size_t i = 0; i < screens_.size(); ++i) delete screens_[i];
    delete z_buffer_;
}

void CompositeFlaschenTaschen::Send() {
    display_updater_->TriggerSend();
}

void CompositeFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    SetPixelAtLayer(x, y, current_layer_, col);
}

void CompositeFlaschenTaschen::SetPixelAtLayer(int x, int y, int layer,
                                               const Color &col) {
    screens_[layer]->At(x, y) = col;
    if (layer >= z_buffer_->At(x, y)) {
        if (col.is_black()) {
            // Transparent pixel. Find closest stacked below us that is not.
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

void CompositeFlaschenTaschen::SetLayer(int layer) {
    if (layer < 0) layer = 0;
    if (layer >= (int)screens_.size()) layer = screens_.size() - 1;
    current_layer_ = layer;
    last_layer_update_time_[current_layer_] = current_time_;
}

void CompositeFlaschenTaschen::StartLayerGarbageCollection(ft::Mutex *lock,
                                                           int timeout_seconds) {
    assert(garbage_collect_ == NULL);  // only start once.
    assert(lock != NULL);  // Must provide mutex.
    garbage_collect_ = new GarbageCollector(this, lock, timeout_seconds);
    garbage_collect_->Start();
}

void CompositeFlaschenTaschen::ClearLayersOlderThan(Ticks cutoff_time) {
    const Color black(0, 0, 0);
    bool any_change = false;
    // Only cleaning layers above zero (= background)
    for (size_t layer = 1; layer < last_layer_update_time_.size(); ++layer) {
        if (last_layer_update_time_[layer] > cutoff_time)
            continue;
        // Not very efficient, but good for now:
        for (int x = 0; x < width_; ++x) {
            for (int y = 0; y < height_; ++y) {
                SetPixelAtLayer(x, y, layer, black);
            }
        }
        last_layer_update_time_[layer] = INT_MAX;
        any_change = true;
    }
    if (any_change) Send();
}
