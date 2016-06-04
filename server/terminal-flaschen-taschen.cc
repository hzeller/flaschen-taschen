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

extern "C" {
#include "pixelterm.h"
}

#define SCREEN_CLEAR    "\033c"
#define SCREEN_CURSOR_UP_FORMAT "\033[%dA"  // Move cursor up given lines.
#define CURSOR_OFF      "\033[?25l"
#define CURSOR_ON       "\033[?25h"

TerminalFlaschenTaschen::TerminalFlaschenTaschen(int fd, int width, int height)
    : terminal_fd_(fd), width_(width), height_(height), is_first_(true),
      buffer_(new Color[width*height]), last_time_(-1) {
}

TerminalFlaschenTaschen::~TerminalFlaschenTaschen() {
    if (!is_first_) {
        dprintf(terminal_fd_, SCREEN_CLEAR);
        dprintf(terminal_fd_, CURSOR_ON);
    }
}

void TerminalFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_)
        return;
    buffer_[y*width_ + x] = col;
}

void TerminalFlaschenTaschen::Send() {
    if (is_first_) {
        is_first_ = false;
        dprintf(terminal_fd_, SCREEN_CLEAR);
        dprintf(terminal_fd_, CURSOR_OFF);
    }

    char * blob = termify_pixels(reinterpret_cast<const uint32_t*>(buffer_), width_, height_);
    dprintf(terminal_fd_, "%s", blob);
    free(blob);

    struct timeval tp;
    gettimeofday(&tp, NULL);
    const int64_t time_now_us = tp.tv_sec * 1000000 + tp.tv_usec;
    const int64_t duration = time_now_us - last_time_;
    if (last_time_ > 0 && duration > 500 && duration < 10000000) {
        const float fps = 1e6 / duration;
        dprintf(terminal_fd_, "\n%7.1f fps\n", fps);
    } else {
        dprintf(terminal_fd_, "\n");
    }
    dprintf(terminal_fd_, SCREEN_CURSOR_UP_FORMAT, (height_+1)/2 + 1);
    last_time_ = time_now_us;
}
