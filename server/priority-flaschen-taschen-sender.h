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

#ifndef PRIORITY_FLASCHEN_TASCHEN_SENDER_H_
#define PRIORITY_FLASCHEN_TASCHEN_SENDER_H_

#include "flaschen-taschen.h"

#include <vector>

// Regular delegate, but Send() operation is elevated and executed in
// a high priority thread.
//
// The Send() operation is delegated to the delegatee but executed in a
// high-priority thread to prevent timing glitches (pauses longer than 50µsec
// are considered 'end of LED strip').
class PriorityFlaschenTaschenSender : public FlaschenTaschen {
public:
    // Does _not_ take over ownership of delegatee.
    PriorityFlaschenTaschenSender(FlaschenTaschen *delegatee);
    ~PriorityFlaschenTaschenSender();

    virtual int width() const { return width_; }
    virtual int height() const { return height_; }

    virtual void SetPixel(int x, int y, const Color &col) {
        delegatee_->SetPixel(x, y, col);
    }
    virtual void Send();

private:
    class DisplayUpdater;
    FlaschenTaschen *const delegatee_;
    const int width_;   // cached.
    const int height_;

    DisplayUpdater *display_updater_;
};

#endif // PRIORITY_FLASCHEN_TASCHEN_SENDER_H_
