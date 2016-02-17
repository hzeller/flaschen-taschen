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

#include "led-flaschen-taschen.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define SCREEN_CLEAR    "\033c"
#define SCREEN_PREFIX   "\033[48;2;0;0;0m"  // set black background
#define SCREEN_POSTFIX  "\033[0m\n"         // reset terminal settings
#define SCREEN_CURSOR_UP_FORMAT "\033[%dA"  // Move cursor up given lines.

// Idea is that we have all colors same width for fast in-place updates.
// so each pixel needs to be fixed width. Thus we do %03 (which luckily is
// not interpreted as octal by the terminal)

// Setting this Unicode character seems to confuse the terminal.
//#define PIXEL_FORMAT   "\033[38;2;%03d;%03d;%03dm●"   // Sent per pixel.

// This is a little dark.
//#define PIXEL_FORMAT   "\033[38;2;%03d;%03d;%03dm*"   // Sent per pixel.

// So, let's just send two spaces 
#define PIXEL_FORMAT   "\033[48;2;%03d;%03d;%03dm "   // Sent per pixel.

TerminalFlaschenTaschen::TerminalFlaschenTaschen(int width, int height)
    : width_(width), height_(height), is_first_(true) {
    buffer_.append(SCREEN_PREFIX);
    initial_offset_ = buffer_.size();
    char scratch[64];
    snprintf(scratch, sizeof(scratch), PIXEL_FORMAT, 0, 0, 0);  // black RGB
    pixel_offset_ = strlen(scratch) + 1;   // one extra space.
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            buffer_.append(scratch);
            buffer_.append(" ");          // Need to add space here.
        }
        buffer_.append("\n");
    }
    buffer_.append(SCREEN_POSTFIX);
    snprintf(scratch, sizeof(scratch), SCREEN_CURSOR_UP_FORMAT, height + 1);
    buffer_.append(scratch);
}

void TerminalFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    int pos = initial_offset_
        + (width_ * y + x) * pixel_offset_
        + y;  // <- one newline per y
    char *buf = const_cast<char*>(buffer_.data())  // Living on the edge
        + pos;
    if (*buf != '\033') {
        printf("Buffer is %x\n", *buf);
    }
    snprintf(buf, pixel_offset_, PIXEL_FORMAT, col.r, col.g, col.b);
    if (buf[pixel_offset_-1] != 0) {
        printf("Uhm, what ? %x\n", buf[pixel_offset_]);
    }
    buf[pixel_offset_-1] = ' ';   // overwrite \0-byte with space again.
}

void TerminalFlaschenTaschen::Send() {
    if (is_first_) {
        (void) write(STDOUT_FILENO, SCREEN_CLEAR, strlen(SCREEN_CLEAR));
        is_first_ = false;
    }
    (void)write(STDOUT_FILENO, buffer_.data(), buffer_.size());
}
