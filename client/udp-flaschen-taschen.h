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

#ifndef UDP_FLASCHEN_TASCHEN_H
#define UDP_FLASCHEN_TASCHEN_H

#include <stdint.h>
#include <stddef.h>
#include "flaschen-taschen.h"

// Open a FlaschenTaschen Socket to the flaschen-taschen server.
int OpenFlaschenTaschenSocket(const char *host);

// A Framebuffer display interface that sends a frame via UDP. Makes things
// simple.
class UDPFlaschenTaschen : public FlaschenTaschen {
public:
    // Create a canvas that can be sent to a FlaschenTaschen server.
    // Socket can be -1, but then you have to use Send(int fd).
    UDPFlaschenTaschen(int socket, int width, int height);
    ~UDPFlaschenTaschen();

    // -- FlaschenTaschen interface
    virtual int width() const { return width_; }
    virtual int height() const { return height_; }

    virtual void SetPixel(int x, int y, const Color &col);
    // Send to file-descriptor given in constructor.
    virtual void Send() { Send(fd_); } 

    // -- additional features.
    // Get pixel at given position. Coordinates outside the range are
    // wrapped around.
    void Clear();
    const Color &GetPixel(int x, int y);
    void Send(int fd);     // Send to given file-descriptor.

private:
    const int fd_;
    const int width_;
    const int height_;
    const size_t buf_size_;
    Color *buffer_;
};

#endif  // UDP_FLASCHEN_TASCHEN_H
