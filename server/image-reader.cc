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

// We also parse meta values in comments, so readNextNumber() and
// skipWhitespace() also take ImageMetaInfo to extract recursively.

#include <string.h>
#include <ctype.h>

#include "image-reader.h"
#include "png-reader.h"
#include "ppm-reader.h"

const char *ReadImageData(const char *in_buffer, size_t buf_len,
                          struct ImageMetaInfo *info_out){
    if (CheckPMMSignature(in_buffer)){
        return ReadPPMImageData(in_buffer, buf_len, info_out);
    }
    else if (CheckPNGSignature(in_buffer)){
        return ReadPNGImageData(in_buffer, buf_len, info_out);
    }
    else{
        return in_buffer;
    }
}

// Read next number. Start reading at *start; modifies the *start pointer
// to point to the character just after the decimal number or NULL if reading
// was not successful.
int readNextNumber(const char **start, const char *end,
                          struct ImageMetaInfo *info) {
    const char *start_number = skipWhitespace(*start, end, info);
    if (start_number == NULL) { *start = NULL; return 0; }
    char *end_number = NULL;
    int result = strtol(start_number, &end_number, 10);
    if (end_number == start_number) { *start = NULL; return 0; }
    *start = end_number;
    return result;
}

void parseOffsets(const char *start, const char *end,
                         struct ImageMetaInfo *info) {
    info->offset_x = readNextNumber(&start, end, NULL);
    if (start != NULL) {
        info->offset_y = readNextNumber(&start, end, NULL);
    }
    if (start != NULL) {
        info->layer = readNextNumber(&start, end, NULL);
    }
}

void parseSpecialComment(const char *start, const char *end,
                                struct ImageMetaInfo *info) {
    if (info == NULL) return;
    if (end - start < 4) return;
    if (strncmp(start, "#FT:", 4) != 0) return;
    parseOffsets(start + 4, end, info);
}

const char *skipWhitespace(const char *buffer, const char *end,
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
