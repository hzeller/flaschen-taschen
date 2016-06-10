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

#ifndef __PIXELTERM_H__
#define __PIXELTERM_H__

ssize_t balloon(const uint32_t *img, size_t sx, size_t x, size_t y);
char *termify_pixels(const uint32_t *img_rgba, size_t sx, size_t sy);

#endif//__PIXELTERM_H__
