/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>
 *
 * This file is taken from pixelterm by jaseg <pixelterm@jaseg.net>,
 * the most recent version may be found at https://github.com/jaseg/pixelterm
 */

#ifndef __XTERMCOLORS_H__
#define __XTERMCOLORS_H__

#include <stdint.h>

union rgba_color {
    struct __attribute__((__packed__)) {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    } c;
    uint32_t i;
};

struct yuv_color {
    float y;
    float u;
    float v;
};

int xterm_closest_color(uint32_t rgba_col);
extern struct yuv_color xtermcolors_yuv[240];

#endif//__XTERMCOLORS_H__
