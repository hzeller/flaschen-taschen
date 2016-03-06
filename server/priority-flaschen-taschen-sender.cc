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

#include "priority-flaschen-taschen-sender.h"

#include <pthread.h>

#include "ft-thread.h"

// We need to do the display update in a high priority thread that
// shall not be interrupted as the LED strips trigger with a 50usec pause.
class PriorityFlaschenTaschenSender::DisplayUpdater : public ft::Thread {
public:
    DisplayUpdater(FlaschenTaschen *display) : display_(display),
                                               state_(UPDATER_IDLE) {
        pthread_cond_init(&send_state_change_, NULL);
    }

    virtual void Run() {
        for (;;) {
            ft::MutexLock l(&mutex_);
            while (state_ == UPDATER_IDLE) {
                mutex_.WaitOn(&send_state_change_);
            }
            if (state_ == UPDATER_EXIT)
                break;
            display_->Send();
            state_ = UPDATER_IDLE;
            pthread_cond_signal(&send_state_change_);
        }
    }

    void SendInThread() {
        ft::MutexLock l(&mutex_);
        state_ = UPDATER_SEND;
        pthread_cond_signal(&send_state_change_);
        while (state_ != UPDATER_IDLE) {
            mutex_.WaitOn(&send_state_change_);
        }
    }

    void TriggerExit() {
        ft::MutexLock l(&mutex_);
        state_ = UPDATER_EXIT;
        pthread_cond_signal(&send_state_change_);
    }

private:
    FlaschenTaschen *const display_;
    enum State {
        UPDATER_IDLE,
        UPDATER_SEND,
        UPDATER_EXIT,
    } state_;
    pthread_cond_t send_state_change_;
    ft::Mutex mutex_;
};

PriorityFlaschenTaschenSender
::PriorityFlaschenTaschenSender(FlaschenTaschen *delegatee)
    : delegatee_(delegatee),
      width_(delegatee->width()), height_(delegatee->height()),
      display_updater_(new DisplayUpdater(delegatee_)) {
    // Realtime-priority, tie to CPU 3
    display_updater_->Start(sched_get_priority_max(SCHED_FIFO), 1 << 3);
}

PriorityFlaschenTaschenSender::~PriorityFlaschenTaschenSender() {
    display_updater_->TriggerExit();
    display_updater_->WaitStopped();
}

void PriorityFlaschenTaschenSender::Send() {
    display_updater_->SendInThread();
}
