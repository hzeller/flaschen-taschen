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

#include "flaschen-taschen.h"

#include <stdint.h>
#include <stddef.h>

// Open a FlaschenTaschen Socket to the flaschen-taschen server.
int OpenFlaschenTaschenSocket(const char *host);

// A Framebuffer display interface that sends a frame via UDP. Makes things
// simple.
class UDPFlaschenTaschen : public FlaschenTaschen {
public:
    // Create a canvas that can be sent to a FlaschenTaschen server.
    // Socket can be -1, but then you have to use the explicit Send(int fd).
    UDPFlaschenTaschen(int socket, int width, int height);
    ~UDPFlaschenTaschen();

    // -- FlaschenTaschen interface implementation
    virtual int width() const { return width_; }
    virtual int height() const { return height_; }

    virtual void SetPixel(int x, int y, const Color &col);

    // Send to file-descriptor given in constructor.
    virtual void Send() { Send(fd_); } 

    // -- Additional features.
    void Send(int fd);     // Send to given file-descriptor.
    void Clear();          // Clear screen (fill with black).

    // Set offset where this picture should be displayed on the remote
    // display.
    void SetOffset(int offset_x, int offset_y);

    // Get pixel color at given position. Coordinates outside the range
    // are wrapped around.
    const Color &GetPixel(int x, int y);

private:
    const int fd_;
    const int width_;
    const int height_;

    // Raw transmit buffer
    size_t buf_size_;
    char *buffer_;

    // pointers into the buffer.
    Color *pixel_buffer_start_;
    char *footer_start_;
};

#endif  // UDP_FLASCHEN_TASCHEN_H
