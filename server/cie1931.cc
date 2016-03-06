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

#include <math.h>
#include <stdint.h>
#include <stdio.h>

static const int kBitPlanes = 16;

// Do CIE1931 luminance correction and scale to maximum expected output bits.
static int luminance_cie1931_internal(uint8_t c) {
    const float out_factor = ((1 << kBitPlanes) - 1);
    const float v = 100.0 * c / 255.0;
    return out_factor * ((v <= 8) ? v / 902.3 : pow((v + 16) / 116.0, 3));
}

static int *CreateCIE1931LookupTable() {
  int *result = new int[256];
  for (int i = 0; i < 256; ++i) {
      result[i] = luminance_cie1931_internal(i);
  }
  return result;
}

int luminance_cie1931(uint8_t output_bits, uint8_t color) {
    static const int *const luminance_lookup = CreateCIE1931LookupTable();
    return luminance_lookup[color] >> (kBitPlanes - output_bits);
}
