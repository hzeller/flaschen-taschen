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

// Open a FlaschenTaschen Socket to the flaschen-taschen display
// hostname.
// If "host" is NULL, attempts to get the name from environment-variable
// FT_DISPLAY.
// If that is not set, uses the default display installation.
int OpenFlaschenTaschenSocket(const char *host);

// A Framebuffer display interface that sends a frame via UDP. Makes things
// simple.
class UDPFlaschenTaschen : public FlaschenTaschen {
public:
    // Create a canvas that can be sent to a FlaschenTaschen server.
    // Socket can be -1, but then you have to use the explicit Send(int fd).
    UDPFlaschenTaschen(int socket, int width, int height);
    UDPFlaschenTaschen(const UDPFlaschenTaschen& other);
    ~UDPFlaschenTaschen();

    // -- FlaschenTaschen interface implementation
    virtual int width() const { return width_; }
    virtual int height() const { return height_; }

    virtual void SetPixel(int x, int y, const Color &col);

    // Send to file-descriptor given in constructor.
    virtual void Send() { Send(fd_); }

    // -- Additional features.
    UDPFlaschenTaschen *Clone() const;  // Create new instance with same content.
    void Send(int fd) const;    // Send to given file-descriptor.
    void Clear();               // Clear screen (fill with black).
    void Fill(const Color &c);  // Fill screen with color.

    // Set offset where this picture should be displayed on the remote
    // display.
    //
    // Setting a z_offset other than zero allows to layer on top of lower
    // content. For layers larger than 0, black pixels are regarded transparent,
    // so if you want to show black, use some very dark gray instead.
    // This feature allows to implement sprites or overlay text easily.
    void SetOffset(int offset_x, int offset_y, int offset_z = 0);

    // Get pixel color at given position. Coordinates outside the range
    // are wrapped around.
    const Color &GetPixel(int x, int y) const;

private:
    const int fd_;
    const int width_;
    const int height_;
    Color *const pixel_buffer_;

    int off_x_;
    int off_y_;
    int off_z_;
};

#endif  // UDP_FLASCHEN_TASCHEN_H
