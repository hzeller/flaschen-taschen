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

#include <stdint.h>
#include <assert.h>

#include "ws2811.h"

#define TARGET_FREQ   WS2811_TARGET_FREQ
#define GPIO_PIN      18
#define DMA_CHANNEL    5

static ws2811_t ledstrings = {0};

WS2811FlaschenTaschen::WS2811FlaschenTaschen(int crate_stack_height)
    : height_(5 * crate_stack_height) {
    const int channel = 0;
    ledstrings.freq = TARGET_FREQ;
    ledstrings.dmanum = DMA_CHANNEL;
    ledstrings.channel[channel].gpionum = GPIO_PIN;  // TODO: channel-config
    ledstrings.channel[channel].count = 5 * height_;
    ledstrings.channel[channel].brightness = 255;
    
    assert(ws2811_init(&ledstrings) == 0);
}

WS2811FlaschenTaschen::~WS2811FlaschenTaschen() {
    ws2811_fini(&ledstrings);
}
        
void WS2811FlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width() || y < 0 || y >= height())
        return;

    const int crate = y / 5;
    y = y % 5;
    const int pos = 25 * crate + kCrateMapping[4-y][x];

    // Our strip has swapped colors.
    const ws2811_led_t strip_color = col.g << 16 | col.r << 8 | col.b;
    ledstrings.channel[0].leds[pos] = strip_color;
}

void WS2811FlaschenTaschen::Send() {
    ws2811_render(&ledstrings);
}

