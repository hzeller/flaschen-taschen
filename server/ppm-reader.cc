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

// Very simple ppm reader.

#include "ppm-reader.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// We also parse meta values in comments, so readNextNumber() and
// skipWhitespace() also take ImageMetaInfo to extract recursively.
static int readNextNumber(const char **start, const char *end,
                          struct ImageMetaInfo *info);
static const char *skipWhitespace(const char *buffer, const char *end,
                                  struct ImageMetaInfo *info);

static void parseOffsets(const char *start, const char *end,
                         struct ImageMetaInfo *info) {
    info->offset_x = readNextNumber(&start, end, NULL);
    if (start != NULL) {
        info->offset_y = readNextNumber(&start, end, NULL);
    }
    if (start != NULL) {
        info->layer = readNextNumber(&start, end, NULL);
    }
}

static void parseSpecialComment(const char *start, const char *end,
                                struct ImageMetaInfo *info) {
    if (info == NULL) return;
    if (end - start < 4) return;
    if (strncmp(start, "#FT:", 4) != 0) return;
    parseOffsets(start + 4, end, info);
}

static const char *skipWhitespace(const char *buffer, const char *end,
                                  struct ImageMetaInfo *info) {
    for (;;) {
        while (buffer < end && isspace(*buffer))
            ++buffer;
        if (buffer >= end)
            return NULL;
        if (*buffer == '#') {
            const char *start = buffer;
            while (buffer < end && *buffer != '\n') // read to end of line.
                ++buffer;
            parseSpecialComment(start, buffer, info);
            continue;  // Back to whitespace eating.
        }
        return buffer;
    }
}

// Read next number. Start reading at *start; modifies the *start pointer
// to point to the character just after the decimal number or NULL if reading
// was not successful.
static int readNextNumber(const char **start, const char *end,
                          struct ImageMetaInfo *info) {
    const char *start_number = skipWhitespace(*start, end, info);
    if (start_number == NULL) { *start = NULL; return 0; }
    char *end_number = NULL;
    int result = strtol(start_number, &end_number, 10);
    if (end_number == start_number) { *start = NULL; return 0; }
    *start = end_number;
    return result;
}

const char *ReadImageData(const char *in_buffer, size_t buf_len,
                          struct ImageMetaInfo *info) {
    if (in_buffer[0] != 'P' || in_buffer[1] != '6' ||
        (!isspace(in_buffer[2]) && in_buffer[2] != '#')) {
        return in_buffer;  // raw image. No P6 magic header.
    }
    const char *const end = in_buffer + buf_len;
    const char *parse_buffer = in_buffer + 2;
    const int width = readNextNumber(&parse_buffer, end, info);
    if (parse_buffer == NULL) return in_buffer;
    const int height = readNextNumber(&parse_buffer, end, info);
    if (parse_buffer == NULL) return in_buffer;
    const int range = readNextNumber(&parse_buffer, end, info);
    if (parse_buffer == NULL) return in_buffer;
    if (!isspace(*parse_buffer++)) return in_buffer;   // last char before data
    // Now make sure that the rest of the buffer still makes sense
    const size_t expected_image_data = width * height * 3;
    const size_t actual_data = end - parse_buffer;
    if (actual_data < expected_image_data)
        return in_buffer;   // Uh, not enough data.
    if (actual_data > expected_image_data) {
        // Our extension: at the end of the binary data, we provide an optional
        // offset. We can't store it in the header, as it is fixed in number
        // of fields. But nobody cares what is at the end of the buffer.
        // Note, this also can be supplied in the specially formatted comment
        // in the header.
        parseOffsets(parse_buffer + expected_image_data, end, info);
    }
    info->width = width;
    info->height = height;
    info->range = range;
    return parse_buffer;
}
