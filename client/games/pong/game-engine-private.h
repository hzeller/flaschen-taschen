// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Basic game framework
// Leonardo Romor <leonardo.romor@gmail.com>
// Henner Zeller <h.zeller@acm.org>
//
#ifndef GAME_ENGINE_PRIVATE_H
#define GAME_ENGINE_PRIVATE_H

#include <stdint.h>
#include <arpa/inet.h>

// The message, serialized over the wire.
struct ClientOutput {
    ClientOutput() : x_pos(0), y_pos(0), button_bits(0) {}

    int16_t x_pos;  // Range -32767 .. +32767
    int16_t y_pos;  // Range -32767 .. +32767

    union {
        unsigned char button_bits;
        struct {
            unsigned char but_exit : 1;
            unsigned char but_a : 1;
            unsigned char but_b : 1;
        } b;
    };
};

#endif  // GAME_ENGINE_PRIVATE_H
