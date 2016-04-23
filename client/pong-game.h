
#include "udp-flaschen-taschen.h"
#include <sys/time.h>


class Actor {
public:
    Actor() {}
    Actor(const char * representation[],
            const int width, const int height) : repr_(representation), width_(width), height_(height) {}

    void print_on_buffer(UDPFlaschenTaschen * frame_buffer);
    float pos[2];
    float speed[2];
    bool IsOnMe(float x, float y);
private:
    const char ** repr_;
    int width_,height_;
};


class PongGame {

public:
    PongGame(const int socket, const int width, const int height);
    ~PongGame();
    void start(const int fps);

private:
    // Evaluate t + 1 of the game, takes care of collisions with the players
    // and set the score in case of the ball goes in the border limit of the game.
    void sim(const float dt);

    // Simply project to pixels the pong world.
    void next_frame();


private:
    const int width_;
    const int height_;
    UDPFlaschenTaschen * frame_buffer_;
    Actor ball_;
    Actor p1_;
    Actor p2_;
    int score[2];
};

//Handles start/stop and returns the number of milliseconds passed if no c++11 :(
class Timer {
public:
    Timer() {}
    ~Timer() {}
    inline void start() {
        gettimeofday(&tp_, NULL);
        start_ = tp_.tv_sec * 1e6 + tp_.tv_usec;
    }

    inline float stop() {
        gettimeofday(&tp_, NULL);
        return (tp_.tv_sec * 1e6 + tp_.tv_usec - start_) / 1e3;
    }

    inline void clear() { start_ = 0; }

private:
    struct timeval tp_;
    unsigned long int start_;
};
