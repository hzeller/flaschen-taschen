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
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define SCREEN_CLEAR    "\033c"
#define SCREEN_PREFIX   "\033[48;2;0;0;0m"  // set black background
#define SCREEN_POSTFIX  "\033[0m"           // reset terminal settings
#define SCREEN_CURSOR_UP_FORMAT "\033[%dA"  // Move cursor up given lines.
#define CURSOR_OFF      "\033[?25l"
#define CURSOR_ON       "\033[?25h"

// Idea is that we have all colors same width for fast in-place updates.
// so each pixel needs to be fixed width. Thus we do %03 (which luckily is
// not interpreted as octal by the terminal)

// Setting this Unicode character seems to confuse at least konsole. Also,
// it is pretty dark.
//#define PIXEL_FORMAT   "\033[38;2;%03d;%03d;%03dm●"   // Sent per pixel.

// This is a little dark.
//#define PIXEL_FORMAT   "\033[38;2;%03d;%03d;%03dm*"   // Sent per pixel.

// So, let's just send two spaces. First space here, rest separate below.
#define PIXEL_FORMAT   "\033[48;2;%03d;%03d;%03dm "   // Sent per pixel.

#define FPS_PLACEHOLDER "___________"

TerminalFlaschenTaschen::TerminalFlaschenTaschen(int fd, int width, int height)
    : terminal_fd_(fd), width_(width), height_(height), is_first_(true),
      last_time_(-1) {
    buffer_.append(SCREEN_PREFIX);
    initial_offset_ = buffer_.size();
    char scratch[64];
    snprintf(scratch, sizeof(scratch), PIXEL_FORMAT, 0, 0, 0); // black.
    pixel_offset_ = strlen(scratch) + 1;   // one extra space.
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            buffer_.append(scratch);
            buffer_.append(" ");          // Need to add space here.
        }
        buffer_.append("\n");
    }

    buffer_.append(SCREEN_POSTFIX);

    fps_offset_ = buffer_.size();
    buffer_.append(FPS_PLACEHOLDER "\n\n");

    snprintf(scratch, sizeof(scratch), SCREEN_CURSOR_UP_FORMAT, height + 2);
    buffer_.append(scratch);
}
TerminalFlaschenTaschen::~TerminalFlaschenTaschen() {
    if (!is_first_) {
        write(terminal_fd_, SCREEN_CLEAR, strlen(SCREEN_CLEAR));
        write(terminal_fd_, CURSOR_ON, strlen(CURSOR_ON));
    }
}

void TerminalFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    const int pos = initial_offset_
        + (width_ * y + x) * pixel_offset_
        + y;  // <- one newline per y
    char *buf = const_cast<char*>(buffer_.data()) + pos;  // Living on the edge
    snprintf(buf, pixel_offset_, PIXEL_FORMAT, col.r, col.g, col.b);
    buf[pixel_offset_-1] = ' ';   // overwrite \0-byte with space again.
}

void TerminalFlaschenTaschen::Send() {
    if (is_first_) {
        write(terminal_fd_, SCREEN_CLEAR, strlen(SCREEN_CLEAR));
        write(terminal_fd_, CURSOR_OFF, strlen(CURSOR_OFF));
        is_first_ = false;
    }

    char *fps_place = const_cast<char*>(buffer_.data()) + fps_offset_;
    struct timeval tp;
    gettimeofday(&tp, NULL);
    const int64_t time_now_us = tp.tv_sec * 1000000 + tp.tv_usec;
    const int64_t duration = time_now_us - last_time_;
    if (last_time_ > 0 && duration > 500 && duration < 10000000) {
        const float fps = 1e6 / duration;
        snprintf(fps_place, strlen(FPS_PLACEHOLDER)+1, "%7.1f fps", fps);
    } else {
        memcpy(fps_place, FPS_PLACEHOLDER, strlen(FPS_PLACEHOLDER));
    }
    last_time_ = time_now_us;

    write(terminal_fd_, buffer_.data(), buffer_.size());
}
