
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include "udp-flaschen-taschen.h"
#include "bdf-font.h"
#include "game-engine.h"

// Size of the display. 9 x 7 crates.
#define DEFAULT_DISPLAY_WIDTH  (9 * 5)
#define DEFAULT_DISPLAY_HEIGHT (7 * 5)

#define PLAYER_ROWS 5
#define PLAYER_COLUMN 1

#define BALL_ROWS 1
#define BALL_COLUMN 1
#define BALL_SPEED 20

#define FRAME_RATE 60
#define MAXBOUNCEANGLE (M_PI * 2 / 12.0)

#define INITIAL_GAME_WAIT (4 * FRAME_RATE)  // First start
#define NEW_GAME_WAIT     (1 * FRAME_RATE)  // Time when a new game starts
#define HELP_DISPLAY_TIME (2 * FRAME_RATE)

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
          const int width, const int height)
        : repr_(representation), width_(width), height_(height) {}

    float pos[2];
    float speed[2];

    void print_on_buffer(FlaschenTaschen * frame_buffer);
    bool IsOverMe(float x, float y);

private:
    const char **repr_;
    const int width_;
    const int height_;

    Actor(); // no default constructor.
};

class PuffAnimation {
public:
    PuffAnimation(const Color &color, int steps, float radius)
        : steps_(steps), radius_(radius),
          color_(color), countdown_(0) {}

    void SetCanvas(FlaschenTaschen *display) { display_ = display; }
    void StartNew(int x, int y) {
        x_ = x;
        y_ = y;
        countdown_ = steps_;
    }

    void Animate() {
        if (countdown_ <= 0) return;
        const float r = (countdown_ - steps_) * radius_ / steps_;
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
    Pong(const int width, const int height,
             const ft::Font &font, const Color &bg);
    ~Pong() {}

    void SetCanvas(UDPFlaschenTaschen *canvas) {
        frame_buffer_ = canvas;
        edge_animation_.SetCanvas(canvas);
        new_ball_animation_.SetCanvas(canvas);
    }

    void UpdateFrame(int64_t game_time_us, const InputList &inputs_list);
private:
    // Evaluate t + 1 of the game, takes care of collisions with the players
    // and set the score in case of the ball goes in the border limit of the game.
    void sim(const uint64_t dt);

    // Simply project to pixels the pong world.
    void next_frame();

    // Reset the ball in 0 with random velocity
    void reset_ball();

private:
    const int width_;
    const int height_;
    const ft::Font &font_;
    const Color background_;

    int start_countdown_;   // Just started a game.
    int help_countdown_;    // Time displaying help

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

    //p1_.pos[0] = 2;
    p1_.pos[1] = (inputs_list[0].y_pos + 1) * (height_ - PLAYER_ROWS) / 2;

    //p2_.pos[0] = width_ - 3;
    p2_.pos[1] = (inputs_list[1].y_pos + 1) * (height_ - PLAYER_ROWS) / 2;

    sim(game_time_us);
    next_frame();
}

void Actor::print_on_buffer(FlaschenTaschen *frame_buffer) {
    for (int row = 0; row < height_; ++row) {
        const char *line = repr_[row];
        for (int x = 0; line[x]; ++x) {
            if (line[x] != ' ') {
                frame_buffer->SetPixel(x + pos[0], row + pos[1],  Color(255,255,255));
            }
        }
    }
}

bool Actor::IsOverMe(float x, float y) {
    return ((x - pos[0]) >= 0 && (x - pos[0]) < width_ &&
            (y - pos[1]) >= 0 && (y - pos[1]) < height_);
}

Pong::Pong(const int width, const int height,
                   const ft::Font &font, const Color &bg)
    : width_(width), height_(height), font_(font), background_(bg),
      start_countdown_(INITIAL_GAME_WAIT), help_countdown_(start_countdown_),
      edge_animation_(Color(255, 100, 0), 15, 5.0),
      new_ball_animation_(Color(0, 50, 0), 5, 3.0),
      ball_(ball, BALL_COLUMN, BALL_ROWS),
      p1_(player, PLAYER_COLUMN, PLAYER_ROWS),
      p2_(player, PLAYER_COLUMN, PLAYER_ROWS) {

    bzero(score_, sizeof(score_));

    reset_ball();  // center ball.

    p1_.pos[0] = 2;
    p1_.pos[1] = (height_ - PLAYER_ROWS) / 2;

    p2_.pos[0] = width_ - 3;
    p2_.pos[1] = (height_ - PLAYER_ROWS) / 2;
}

void Pong::reset_ball() {
    ball_.pos[0] = width_/2;
    ball_.pos[1] = height_/2-1;
    new_ball_animation_.StartNew(ball_.pos[0], ball_.pos[1]);

    // Generate random angle in the range of 1/4 tau
    const float theta = 2 * M_PI / 4 * (rand() % 100 - 50) / 100.0;
    const int direction = rand() % 2 == 0 ? -1 : 1;
    ball_.speed[0] = direction * BALL_SPEED * cos(theta);
    ball_.speed[1] = direction * BALL_SPEED * sin(theta);
}

void Pong::sim(const uint64_t dt) {
    float angle;
    float new_pos[2];
    if (help_countdown_ > 0)
        --help_countdown_;

    if (start_countdown_ > 0) {
        --start_countdown_;
        return;
    }

    new_pos[0] = ball_.pos[0] + dt * ball_.speed[0] / 1e6;
    new_pos[1] = ball_.pos[1] + dt * ball_.speed[1] / 1e6;

    // Check if bouncin on a player cursor, (careful, for low fps
    // QUANTUM TUNNELING may arise.
    // TODO: we should check if we are going through a paddle from one frame to the other)
    if (p1_.IsOverMe(new_pos[0], new_pos[1])) {
        // Evaluate the reflection angle that will be larger as we bounce farther from the paddle center
        angle = (p1_.pos[1] + PLAYER_ROWS/2 - new_pos[1]) * MAXBOUNCEANGLE / (PLAYER_ROWS / 2);
        ball_.speed[0] = BALL_SPEED * cos(angle);
        ball_.speed[1] = BALL_SPEED * -1 * sin(angle);
    } else if (p2_.IsOverMe(new_pos[0], new_pos[1])) {
        angle = (p2_.pos[1] + PLAYER_ROWS/2 - new_pos[1]) * MAXBOUNCEANGLE / (PLAYER_ROWS / 2);
        ball_.speed[0] = BALL_SPEED * -1 * cos(angle);
        ball_.speed[1] = BALL_SPEED * -1 * sin(angle);
    }

    // Wall
    if ( new_pos[1] < 0 || new_pos[1] > height_ ) ball_.speed[1] *= -1;

    // Evaluate the new ball delta
    ball_.pos[0] += dt * ball_.speed[0] / 1e6;
    ball_.pos[1] += dt * ball_.speed[1] / 1e6;

    // Score
    if (ball_.pos[0] < 0) {
        edge_animation_.StartNew(0, ball_.pos[1]);
        score_[1] += 1;
        ball_.pos[0] = width_ / 2;
        ball_.pos[1] = height_ / 2;
        reset_ball();
        start_countdown_ = NEW_GAME_WAIT;
    } else if (ball_.pos[0] > width_ - 1) {
        edge_animation_.StartNew(width_ - 1, ball_.pos[1]);
        score_[0] += 1;
        ball_.pos[0] = width_ / 2;
        ball_.pos[1] = height_ / 2;
        reset_ball();
        start_countdown_ = NEW_GAME_WAIT;
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

    if (help_countdown_) {
        const int kFadeDuration = 60;
        const float fade_fraction = (help_countdown_ < kFadeDuration
                               ? 1.0 * help_countdown_ / kFadeDuration
                               : 1.0);
        const Color help_col(fade_fraction * 255, fade_fraction * 255, 0);
        ft::DrawText(frame_buffer_, font_, 0, 5 - 1, help_col, NULL, "W");
        ft::DrawText(frame_buffer_, font_, 0, height_ - 1, help_col, NULL, "D");

        ft::DrawText(frame_buffer_, font_, width_-5, 5 - 1, help_col, NULL, "I");
        ft::DrawText(frame_buffer_, font_, width_-5, height_ - 1, help_col, NULL, "J");
    }

    edge_animation_.Animate();
    new_ball_animation_.Animate();

    // Print the actors
    ball_.print_on_buffer(frame_buffer_);
    p1_.print_on_buffer(frame_buffer_);
    p2_.print_on_buffer(frame_buffer_);

    // Push the frame
    frame_buffer_->Send();
}

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-g <width>x<height>[+<off_x>+<off_y>[+<layer>]] : Output geometry. Default 45x35\n"
            "\t-l <layer>      : Layer 0..15. Default 0 (note if also given in -g, then last counts)\n"
            "\t-h <host>       : Flaschen-Taschen display hostname.\n"
            "\t-r <port>       : remote operation: listen on port\n"
            //"\t-k              : Use the local keyboard without udp clients\n" ?
            "\t-b<RRGGBB>      : Background color as hex (default: 000000)\n"
            );

    return 1;
}

int main(int argc, char *argv[]) {

    const char *hostname = NULL;   // Will use default if not set otherwise.
    int width = DEFAULT_DISPLAY_WIDTH;
    int height= DEFAULT_DISPLAY_HEIGHT;
    int off_x = 0;
    int off_y = 0;
    int off_z = 0;
    int remote_port = 4321;
    Color bg(0, 0, 0);

    if (argc < 2) {
        fprintf(stderr, "Mandatory argument(s) missing\n");
        return usage(argv[0]);
    }

    int opt;
    while ((opt = getopt(argc, argv, "g:l:h:p:b:")) != -1) {
        switch (opt) {
        case 'g':
            if (sscanf(optarg, "%dx%d%d%d%d", &width, &height, &off_x, &off_y, &off_z)
                < 2) {
                fprintf(stderr, "Invalid size spec '%s'", optarg);
                return usage(argv[0]);
            }
            break;
        case 'p':
            remote_port = atoi(optarg);
            if (remote_port < 1023 || remote_port > 65535) {
                fprintf(stderr, "Invalid port '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'h':
            hostname = strdup(optarg); // leaking. Ignore.
            break;
        case 'l':
            if (sscanf(optarg, "%d", &off_z) != 1 || off_z < 0 || off_z >= 16) {
                fprintf(stderr, "Invalid layer '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'b': {
            int r, g, b;
            if (sscanf(optarg, "%02x%02x%02x", &r, &g, &b) != 3) {
                fprintf(stderr, "Background color parse error\n");
                return usage(argv[0]);
            }
            bg.r = r; bg.g = g; bg.b = b;
            break;
        }
        default:
            return usage(argv[0]);
        }
    }

    ft::Font font;
    font.LoadFont("fonts/5x5.bdf");  // TODO: don't hardcode.

    srand(time(NULL));

    const int display_socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen display(display_socket, width, height);
    display.SetOffset(off_x, off_y, off_z);

    Pong pong(width, height, font, bg);
    pong.SetCanvas(&display);

    // Start the game with x fps
    RunGame(&pong, FRAME_RATE, remote_port);

    display.Clear();
    display.Send();

    fprintf(stderr, "Good bye.\n");

    return 0;
}
