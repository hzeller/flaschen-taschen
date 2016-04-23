// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Simple example how to write an animation. Prepares two UDPFlaschenTaschen
// with one frame of an invader animation each and shows them in sequence
// while modifying the position on the screen.
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./simple-animation localhost
//
// .. or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./simple-animation

#include "udp-flaschen-taschen.h"

#include <vector>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include <sys/time.h>
#include <sys/select.h>
#include <termios.h>
#include <string.h>
#include "pong-game.h"

// Size of the display.
#define DISPLAY_WIDTH 92
#define DISPLAY_HEIGHT 43

#define PLAYER_ROWS 5
#define PLAYER_COLUMN 1

#define BALL_ROWS 1
#define BALL_COLUMN 1

struct termios orig_termios;

void reset_terminal_mode() {
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode() {
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

static const char * player[PLAYER_ROWS] = {
        "#",
        "#",
        "#",
        "#",
        "#",
};

static const char * ball[PLAYER_ROWS] = {
        "#",
};

void Actor::print_on_buffer(UDPFlaschenTaschen * frame_buffer) {

    for (int row = 0; row < height_; ++row) {
        const char *line = repr_[row];
        for (int x = 0; line[x]; ++x) {
            if (line[x] != ' ') {
                frame_buffer->SetPixel(x + pos[0], row + pos[1],  Color(255,255,255));
            }
        }
    }
}

bool Actor::IsOnMe(float x, float y) {
    if( (x - pos[0]) >= 0 && (x - pos[0]) < width_ && (y - pos[1]) >= 0 && (y - pos[1]) < height_ ) {
        return true;
    }
    return false;
}

PongGame::PongGame(const int socket, const int width, const int height) : width_(width), height_(height) {
    frame_buffer_ = new UDPFlaschenTaschen(socket,
                                                width,
                                                height);

    ball_ = Actor(ball, BALL_COLUMN, BALL_ROWS);
    //Center the ball
    ball_.pos[0] = width_/2;
    ball_.pos[1] = height_/2-2;
    ball_.speed[0] = 10; // pixels per sec
    ball_.speed[1] = 0;

    p1_ = Actor(player, PLAYER_COLUMN, PLAYER_ROWS);
    p1_.pos[0] = width_*2/10;
    p1_.pos[1] = height_/2 - PLAYER_ROWS/2;
    p2_ = Actor(player, PLAYER_COLUMN, PLAYER_ROWS);
    p2_.pos[0] = width_*8/10;
    p2_.pos[1] = height_/2 - PLAYER_ROWS/2;

}

PongGame::~PongGame() {
    delete frame_buffer_;
}

void PongGame::start(const int fps) {

    assert( fps > 0 );
    Timer t = Timer();
    float dt = 0;
    char command;
    // File descriptor set
    fd_set rfds;
    struct timeval tv;
    int retval;

    set_conio_terminal_mode();

    while(1){

        t.clear();
        t.start();

        //Asume the calculations to be instant for the sleep
        sim(dt); // Sim the pong world and handle game logic
        next_frame(); // Clear the screen, set the pixels, and send them.

        FD_ZERO(&rfds);
        FD_SET(0, &rfds);

       /* Do not wait */
        tv.tv_sec = 0;
        tv.tv_usec = 1e6/(float)fps;

        retval = select(1, &rfds, NULL, NULL, &tv);

        if (retval == -1)
             std::cerr << "Error on select" << std::endl;
        else if (retval){
            read(0, &command, sizeof(char));
            switch (command) {
                case 's':
                    p1_.pos[1] +=1;
                    break;
                case 'w':
                    p1_.pos[1] +=-1;
                    break;
                case 'k':
                    p2_.pos[1] +=1;
                    break;
                case 'i':
                    p2_.pos[1] +=-1;
                    break;
                case 'q':
                    exit(0);
                default:
                    break;
            }
            command = 0;
        }

        dt = t.stop();
    }

}

void PongGame::sim(const float dt) {

    // Evaluate the new ball delta
    ball_.pos[0] += dt * ball_.speed[0] / 1e3;
    ball_.pos[1] += dt * ball_.speed[1] / 1e3;

    // Check if bouncin on a player cursor, (careful, for low fps
    // QUANTUM TUNELING may arise)
    if(p1_.IsOnMe(ball_.pos[0], ball_.pos[1]) || p2_.IsOnMe(ball_.pos[0], ball_.pos[1])) {
        // If is one the cursor reevaluate the new position with the inverted speed( quite bad programming )
        ball_.speed[0] *= -1;
        ball_.pos[0] += 2 * dt * ball_.speed[0] / 1e3;
    }

    // Wall
    if(ball_.pos[1] < 0 || ball_.pos[1] > height_ ) ball_.speed[1] *=-1;

    // Score
    if(ball_.pos[0] < 0 ) {
        score[0] += 1;
        ball_.pos[0] = width_/2;
        ball_.pos[1] = height_/2;
    } else if (ball_.pos[0] > width_){
        score[1] += 1;
        ball_.pos[0] = width_/2;
        ball_.pos[1] = height_/2;

    }
    // remember to listen to  https://www.youtube.com/watch?v=cNAdtkSjSps
}

void PongGame::next_frame() {
    frame_buffer_->Clear();
    ball_.print_on_buffer(frame_buffer_);
    p1_.print_on_buffer(frame_buffer_);
    p2_.print_on_buffer(frame_buffer_);
    //frame_buffer_->SetPixel(0,0, Color(255,255,255));
    frame_buffer_->Send();

}


int main(int argc, char *argv[]) {
    const char *hostname = NULL;   // Will use default if not set otherwise.
    if (argc > 1) {
        hostname = argv[1];        // Hostname can be supplied as first arg
    }

    // Open socket.
    const int socket = OpenFlaschenTaschenSocket(hostname);

    PongGame pong = PongGame(socket, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    //Start game
    pong.start(60);

}
