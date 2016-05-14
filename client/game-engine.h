// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Basic game framework
// Leonardo Romor <leonardo.romor@gmail.com>
// Henner Zeller <h.zeller@acm.org>
//
#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "udp-flaschen-taschen.h"
#include <vector>

struct GameInput {
    float x_pos;  // Range -1 .. +1
    float y_pos;  // Range -1 .. +1
    // buttons...
};

// A game needs to implement this interface. It is called regularly with
// at least the frame-rate or whenever there is new input.
class Game {
public:
    typedef std::vector<GameInput> InputList;

    virtual ~Game() {}

    // The canvas this game draws on.
    virtual void SetCanvas(UDPFlaschenTaschen *canvas) = 0;

    // Call UpdateFrame() with game time (micro-seconds since beginning)
    // and current state of input. The UpdateFrame() method is called whenever
    // the input changes, or at least with the refresh rate (like 60/second).
    // Only when within this method, the canvas should be updated.
    virtual void UpdateFrame(int64_t game_time_us,
                             const InputList &inputs_list) = 0;

};

// Run game: Read inputs and call the UpdateFrame() regularly with fresh
// input or whenever a new frame has to be drawn.
int RunGame(const int argc, char *argv[], Game *game);

#endif  // GAME_ENGINE_H
