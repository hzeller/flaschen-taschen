// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Basic game framework
// Leonardo Romor <leonardo.romor@gmail.com>
// Henner Zeller <h.zeller@acm.org>
//
#include <vector>
#include "udp-flaschen-taschen.h"

#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

struct GameInput {
    float x_pos;  // Range -1 .. +1
    float y_pos;  // Range -1 .. +1
    // buttons...
};
typedef std::vector<GameInput> InputList;

// An InputController needs to implement this interface.
// When RunGame is called UpdateInputList is automatically called and the controller
// should be able to update the InputList (tbd, for now we accept a vector
// of 2 players each of them with an x,y (-1,1) "output")
class InputController {
public:
    ~InputController() {}
    virtual void UpdateInputList(InputList *inputs_list, uint64_t timeout_us) = 0;
};

// A game needs to implement this interface. It is called regularly with
// at least the frame-rate or whenever there is new input.
class Game {
public:

    virtual ~Game() {}

    // The canvas this game draws on.
    virtual void SetCanvas(UDPFlaschenTaschen *canvas) = 0;

    // Call UpdateFrame() with game time (micro-seconds since beginning)
    // and current state of input. The UpdateFrame() method is called whenever
    // the input changes, or at least with the refresh rate (like 60/second).
    // Only when within this method, the canvas should be updated.
    virtual void UpdateFrame(int64_t game_time_us, const InputList &inputs_list) = 0;

};

// Run game: Read inputs and call the UpdateFrame() regularly.
void RunGame(Game *game, int frame_rate, int remote_port);

#endif  // GAME_ENGINE_H
