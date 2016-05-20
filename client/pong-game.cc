// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Pong game.
// Leonardo Romor <leonardo.romor@gmail.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include "udp-flaschen-taschen.h"
#include "bdf-font.h"
#include "graphics.h"
#include "game-engine.h"

#define PLAYER_ROWS 5
#define PLAYER_COLUMN 1

#define BALL_ROWS 1
#define BALL_COLUMN 1
#define BALL_SPEED 20

#define MAXBOUNCEANGLE (M_PI * 2 / 12.0)
#define MOMENTUM_RATIO 0.9

#define FRAME_RATE 60
#define INITIAL_GAME_WAIT (4 * FRAME_RATE)  // First start
#define NEW_GAME_WAIT     (1 * FRAME_RATE)  // Time when a new game starts
#define NEW_MATCH_WAIT    (3 * FRAME_RATE)  // Fireworks time
#define POINTS_TO_WIN 21

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

class Actor {
public:
    Actor(const char *representation[],
          const int width, const int height, const Color& color)
        : repr_(representation), width_(width), height_(height), col_(color),
          skip_blur_(true) {
        bzero(pos, sizeof(pos));
        bzero(speed, sizeof(pos));
        bzero(last_pos, sizeof(last_pos));
    }

    float pos[2];
    float speed[2];

    void print_on_buffer(FlaschenTaschen * frame_buffer);
    bool IsOverMe(float x, float y);

    void SetMotionBlurColor(const Color &col) {
        motion_blur_color_ = col;
    }

    void SetSkipNextMotionBlur() { skip_blur_ = true; }

private:
    const char **repr_;
    const int width_;
    const int height_;
    const Color col_;
    Color motion_blur_color_;
    bool skip_blur_;

    float last_pos[2];

    Actor(); // no default constructor.
};

class PuffAnimation {
public:
    // Negative radius: reverse animation.
    PuffAnimation(const Color &color, int steps, float radius)
        : steps_(steps), radius_(radius),
          color_(color), countdown_(0) {}

    void SetCanvas(FlaschenTaschen *display) {
        display_ = display;
    }

    void StartNew(int x, int y) {
        x_ = x;
        y_ = y;
        countdown_ = steps_;
    }

    void Animate() {
        if (countdown_ <= 0) return;
        float r;
        if (radius_ < 0) {
            r = radius_ - (countdown_ - steps_) * -radius_ / steps_;
        } else {
            r = (countdown_ - steps_) * radius_ / steps_;
        }
        // Simplified circle. Could be more efficient.
        for (float a = 0; a < M_PI / 2; a += M_PI / 7) {
            display_->SetPixel(x_ + cos(a) * r, y_ + sin(a) * r, color_);
            display_->SetPixel(x_ - cos(a) * r, y_ + sin(a) * r, color_);
            display_->SetPixel(x_ - cos(a) * r, y_ - sin(a) * r, color_);
            display_->SetPixel(x_ + cos(a) * r, y_ - sin(a) * r, color_);
        }
        --countdown_;
    }

private:
    const int steps_;
    const float radius_;
    const Color color_;
    FlaschenTaschen *display_;

    int countdown_;
    int x_, y_;
};

class Pong : public Game {
public:
    Pong(const ft::Font &font);
    ~Pong() {}

    virtual void SetCanvas(UDPFlaschenTaschen *canvas,
                           const Color &background) {
        frame_buffer_ = canvas;
        background_ = background;
        width_ = frame_buffer_->width();
        height_ = frame_buffer_->height();

        edge_animation_.SetCanvas(canvas);
        new_ball_animation_.SetCanvas(canvas);
    }

    virtual void Start() {
        start_countdown_ = INITIAL_GAME_WAIT;
        bzero(score_, sizeof(score_));
        last_game_time_ = 0;
        reset_ball();  // center ball.
        p1_.pos[0] = 2;
        p1_.pos[1] = (height_ - PLAYER_ROWS) / 2;

        p2_.pos[0] = width_ - 3;
        p2_.pos[1] = (height_ - PLAYER_ROWS) / 2;
    }

    virtual void UpdateFrame(int64_t game_time_us, const InputList &inputs_list);

private:
    // Evaluate t + 1 of the game, takes care of collisions with the players
    // and set the score in case of the ball goes in the border limit of the game.
    void sim(const uint64_t dt, const InputList &inputs_list);

    // Simply project to pixels the pong world.
    void next_frame();

    // Reset the ball in 0 with random velocity
    void reset_ball();

private:
    int width_;
    int height_;
    const ft::Font &font_;
    Color background_;

    int64_t last_game_time_;
    int start_countdown_;   // Just started a game.

    UDPFlaschenTaschen * frame_buffer_;
    PuffAnimation edge_animation_;
    PuffAnimation new_ball_animation_;

    Actor ball_;
    Actor p1_;
    Actor p2_;
    int score_[2];

    Pong(); // no default constructor.
};

void Pong::UpdateFrame(int64_t game_time_us, const InputList &inputs_list) {
    sim(game_time_us - last_game_time_, inputs_list);
    next_frame();
    last_game_time_ = game_time_us;
}

void Actor::print_on_buffer(FlaschenTaschen *frame_buffer) {
    if (!motion_blur_color_.is_black() && !skip_blur_) {
        ft::DrawLine(frame_buffer, last_pos[0], last_pos[1],
                     pos[0], pos[1], motion_blur_color_);
    }
    for (int row = 0; row < height_; ++row) {
        const char *line = repr_[row];
        for (int x = 0; line[x]; ++x) {
            if (line[x] != ' ') {
                frame_buffer->SetPixel(x + pos[0], row + pos[1],  col_);
            }
        }
    }
    memcpy(last_pos, pos, sizeof(last_pos));
    skip_blur_ = false;  // reset
}

bool Actor::IsOverMe(float x, float y) {
    return ((x - pos[0]) >= 0 && (x - pos[0]) < width_ &&
            (y - pos[1]) >= 0 && (y - pos[1]) < height_);
}

Pong::Pong(const ft::Font &font)
    : width_(-1), height_(-1), font_(font),
      last_game_time_(0),
      start_countdown_(INITIAL_GAME_WAIT),
      edge_animation_(Color(255, 100, 0), 15, 5.0),
      new_ball_animation_(Color(100, 100, 0), 10, -5.0),
      ball_(ball, BALL_COLUMN, BALL_ROWS, Color(255, 255, 0)),
      p1_(player, PLAYER_COLUMN, PLAYER_ROWS, Color(255, 255, 255)),
      p2_(player, PLAYER_COLUMN, PLAYER_ROWS, Color(255, 255, 255)) {
    bzero(score_, sizeof(score_));
    ball_.SetMotionBlurColor(Color(120, 120, 0));
}

void Pong::reset_ball() {
    ball_.pos[0] = width_/2;
    ball_.pos[1] = height_/2-1;
    new_ball_animation_.StartNew(ball_.pos[0], ball_.pos[1]);
    ball_.SetSkipNextMotionBlur();

    // Generate random angle in the range of 1/4 tau
    const float theta = 2 * M_PI / 4 * (rand() % 100 - 50) / 100.0;
    const int direction = rand() % 2 == 0 ? -1 : 1;
    ball_.speed[0] = direction * BALL_SPEED * cos(theta);
    ball_.speed[1] = direction * BALL_SPEED * sin(theta);
}

void Pong::sim(const uint64_t dt, const InputList &inputs_list) {
    float angle;
    float new_pos[2];

    if (start_countdown_ > 0) --start_countdown_;

    // New speed and position evaluation (PLAYERS)
    p1_.speed[1] = -p1_.pos[1];
    p1_.pos[1] = (inputs_list[0].y_pos + 1) * (height_ - PLAYER_ROWS) / 2;
    p1_.speed[1] += p1_.pos[1];
    p1_.speed[1] /= dt / 1e6;

    p2_.speed[1] = -p2_.pos[1];
    p2_.pos[1] = (inputs_list[1].y_pos + 1) * (height_ - PLAYER_ROWS) / 2;
    p2_.speed[1] += p2_.pos[1];
    p2_.speed[1] /= dt / 1e6;

    // Let's check if we are in a collision event
    new_pos[0] = ball_.pos[0] + dt * ball_.speed[0] / 1e6;
    new_pos[1] = ball_.pos[1] + dt * ball_.speed[1] / 1e6;

    // Check if bouncin on a player cursor, (careful, for low fps
    // QUANTUM TUNNELING may arise.
    // TODO: we should check if we are going through a paddle from one frame to the other)
    if (p1_.IsOverMe(new_pos[0], new_pos[1])) {
        // Evaluate the reflection angle that will be larger as we bounce farther from the paddle center
        angle = (p1_.pos[1] + PLAYER_ROWS/2 - new_pos[1]) * MAXBOUNCEANGLE / (PLAYER_ROWS / 2);
        ball_.speed[0] = BALL_SPEED * cos(angle);
        ball_.speed[1] = BALL_SPEED * -1 * sin(angle) + MOMENTUM_RATIO * p1_.speed[1];
    } else if (p2_.IsOverMe(new_pos[0], new_pos[1])) {
        angle = (p2_.pos[1] + PLAYER_ROWS/2 - new_pos[1]) * MAXBOUNCEANGLE / (PLAYER_ROWS / 2);
        ball_.speed[0] = BALL_SPEED * -1 * cos(angle);
        ball_.speed[1] = BALL_SPEED * -1 * sin(angle)  + MOMENTUM_RATIO * p2_.speed[1];
    }

    // Wall
    if ( new_pos[1] < 0 || new_pos[1] > height_ ) ball_.speed[1] *= -1;

    // Evaluate the new ball delta
    if (!start_countdown_) {
        ball_.pos[0] += dt * ball_.speed[0] / 1e6;
        ball_.pos[1] += dt * ball_.speed[1] / 1e6;
    }

    // Score
    if (ball_.pos[0] < 0) {
        edge_animation_.StartNew(0, ball_.pos[1]);
        score_[1] += 1;  // Opposing party gets the point.
        ball_.pos[0] = width_ / 2;
        ball_.pos[1] = height_ / 2;
        reset_ball();
        start_countdown_ = NEW_GAME_WAIT;
    } else if (ball_.pos[0] > width_ - 1) {
        edge_animation_.StartNew(width_ - 1, ball_.pos[1]);
        score_[0] += 1;  // Opposing party gets the point.
        ball_.pos[0] = width_ / 2;
        ball_.pos[1] = height_ / 2;
        reset_ball();
        start_countdown_ = NEW_GAME_WAIT;
    }

    if (score_[0] == POINTS_TO_WIN || score_[1] == POINTS_TO_WIN) {
        //start_countdown_ = NEW_MATCH_WAIT;
        // Fireworks?
        // ???
        // profit.
        // return;
    }
}

void Pong::next_frame() {
    // Clear the frame.
    frame_buffer_->Fill(background_);

    // Centerline
    const Color divider_color(50, 100, 50);
    for (int y = 0; y < height_/2; ++y) {
        frame_buffer_->SetPixel(width_ / 2, 2 * y, divider_color);
    }

    // We have crates that are 5 pixels. We want the number aligned to the left
    // and right of the center crate.
    const int center_start = width_ / (2 * 5) * 5;
    const Color score_color(0, 0, 255);

    char score_string[5];
    int text_len = snprintf(score_string, sizeof(score_string), "%d", score_[0]);
    ft::DrawText(frame_buffer_, font_,
                 center_start - 5*text_len, 4,  // Right aligned
                 score_color, NULL, score_string);

    text_len = snprintf(score_string, sizeof(score_string), "%d", score_[1]);
    ft::DrawText(frame_buffer_, font_,
                 center_start + 5, 4,
                 score_color, NULL, score_string);

    edge_animation_.Animate();
    new_ball_animation_.Animate();

    // Print the actors
    ball_.print_on_buffer(frame_buffer_);
    p1_.print_on_buffer(frame_buffer_);
    p2_.print_on_buffer(frame_buffer_);

    // Push the frame
    frame_buffer_->Send();
}

int main(int argc, char *argv[]) {
    ft::Font font;
    font.LoadFont("fonts/5x5.bdf");  // TODO: don't hardcode.

    srand(time(NULL));

    Pong pong(font);
    return RunGame(argc, argv, &pong);
}
