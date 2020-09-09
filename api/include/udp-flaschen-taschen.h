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
    //
    // The UDP packet size is the maximum amount of data that is sent
    // per Send() call. Usually, the maxiumum of 65507 is just fine except
    // in OSX, which has a limit of about 9000.
    //
    // The UDP size can be overwritten with the FT_UDP_SIZE environment
    // variable.
    UDPFlaschenTaschen(int socket, int width, int height,
                       size_t max_udp_size = 65507);
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

    // Set maximum UDP packet size to be used without IP/UDP header.
    // The maxium allowable size is 65507 (65535 minus UDP and IP header).
    // The minium depends on the width of the canvas and requires at least one
    // line to fit.
    //
    // Some operating systems (e.g. OSX) have smaller limits by default.
    // Returns boolean if setting the value was successful.
    bool SetMaxUDPPacketSize(size_t packet_size);

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

    size_t max_udp_size_;
};

#endif  // UDP_FLASCHEN_TASCHEN_H
