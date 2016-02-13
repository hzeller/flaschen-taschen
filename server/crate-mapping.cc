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

// This is how the mapping looks from the back of the crate. So wiring is
// always happening on the left of the crate.

//     ^-- Next crate on the top of this
//     |
int kCrateMapping[5][5] = {
    { 24, 23, 22, 21, 20 },
    {  3,  4, 11, 12, 19 },
    {  2,  5, 10, 13, 18 },
    {  1,  6,  9, 14, 17 },
    {  0,  7,  8, 15, 16 },
};
//-----^
//     |
//     +-- Previous crate enters here (bottom)
