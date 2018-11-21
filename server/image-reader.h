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

#ifndef IMAGEREADER_H
#define IMAGEREADER_H

#include <stdlib.h>

enum ImageType {
    RAWBMP = 0,         // raw bmp
    P6 = 1,             // ppm file (+ footer)
    PNG = 2             // png file (+ footer)
};

struct ImageMetaInfo {
    int width;
    int height;
    int range;      // Range of gray-levels. We only handle 255 correctly(=1byte)
    ImageType type;

    // FlaschenTaschen extensions
    int offset_x;   // display image at this x-offset
    int offset_y;   // .. y-offset
    int layer;      // stacked layer
};

const char *ReadImageData(const char *in_buffer, size_t buf_len,
              struct ImageMetaInfo *info_out);

void parseOffsets(const char *start, const char *end,
                         struct ImageMetaInfo *info);

const char *skipWhitespace(const char *buffer, const char *end,
                                  struct ImageMetaInfo *info);

int readNextNumber(const char **start, const char *end,
                          struct ImageMetaInfo *info);

#endif // IMAGEREADER_H
