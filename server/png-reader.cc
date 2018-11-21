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

#include <algorithm>
#include <libpng/png.h>
#include <stdio.h>
#include <string.h>
#include <istream>

#include "image-reader.h"
#include "png-reader.h"

#define PNGSIGSIZE 8

const bool CheckPNGSignature(const char* in_buffer) {

    //Allocate a buffer of 8 bytes, where we can put the file signature.
    png_byte pngsig[PNGSIGSIZE];
    std::copy(in_buffer, in_buffer+PNGSIGSIZE, pngsig);

    //Let LibPNG check the sig. If this function returns 0, png file detected.
    int is_png = png_sig_cmp(pngsig, 0, PNGSIGSIZE);
    return (is_png == 0);
}

const char *ReadPNGImageData(const char *in_buffer, size_t buf_len,
                             struct ImageMetaInfo *info_out) {
    png_image image;
    memset(&image, 0, (sizeof image));
    image.version = PNG_IMAGE_VERSION;

    if (png_image_begin_read_from_memory(&image, in_buffer, buf_len) == 0)
    {
        return in_buffer;
    }

    void* buffer = malloc(PNG_IMAGE_SIZE(image));
    if (buffer == NULL) {
        return in_buffer;
    }
    if (!png_image_finish_read(&image, NULL, buffer, 0, NULL)) {
        free(buffer);
        return in_buffer;
    }

    //find end of png file in stream
    const char* iter = in_buffer + buf_len;
    int count = 0;
    while (strncmp(iter, "IEND", 4) != 0){
        --iter;
        count++;
    }

    iter += PNGSIGSIZE;
    if (count>PNGSIGSIZE){
        // try to parse custom footer
        parseOffsets(iter, iter + count, info_out);
    }

    info_out->width=image.width;
    info_out->height=image.height;
    info_out->type=ImageType::PNG;

    return (char*)buffer;
}
