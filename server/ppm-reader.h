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

// Reader for the extended PPM format we are using for FlaschenTaschen: it not
// only has width/height but also offset information in x,y and z (=layer)
// direction.

#include <stdlib.h>

struct ImageMetaInfo {
    int width;
    int height;
    int range;      // Range of gray-levels. We only handle 255 correctly(=1byte)

    // FlaschenTaschen extensions
    int offset_x;   // display image at this x-offset
    int offset_y;   // .. y-offset
    int layer;      // stacked layer
};

// Given an input buffer + size with a PPM file, extract the image
// meta information and return it in "info_out".
// Return pointer to raw image data with uint8 [r,g,b] tuples.
// Returns the original pointer if PPM meta data could not be read; this means
// that we are compatible as well with 'raw' images.
// Image info is left untouched when image does not contain any
// information.
// This also extracts the FlaschenTaschen extension to the PPM image format
// that contains the offset.
const char *ReadImageData(const char *in_buffer, size_t buf_len,
			  struct ImageMetaInfo *info_out);
