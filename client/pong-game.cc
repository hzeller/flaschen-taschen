// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Pong game.
// Leonardo Romor <leonardo.romor@gmail.com>
//
//  ./pong-game localhost
//
// .. or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./pong-game

#include "udp-flaschen-taschen.h"
#include "bdf-font.h"

#include <assert.h>
#include <iostream>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>

// Size of the display. 9 x 7 crates.
#define DISPLAY_WIDTH  (9 * 5)
#define DISPLAY_HEIGHT (7 * 5)

#define PLAYER_ROWS 5
#define PLAYER_COLUMN 1

#define BALL_ROWS 1
#define BALL_COLUMN 1
#define BALL_SPEED 15

#define FRAME_RATE 60
#define KEYBOARD_STEP 2

#define INITIAL_GAME_WAIT (4 * FRAME_RATE)  // First start
#define NEW_GAME_WAIT     (1 * FRAME_RATE)  // Time when a new game starts
#define HELP_DISPLAY_TIME (2 * FRAME_RATE)

#define MAX_BUFFER_LENGTH 2
#define SHRT_MAX 32768

struct termios orig_termios;
static void reset_terminal_mode() {
    tcsetattr(0, TCSANOW, &orig_termios);
}

static void set_conio_terminal_mode() {
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

class Timer {
public:
    Timer() : start_(0) {}

    void Start() {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        start_ = tp.tv_sec * 1000000 + tp.tv_usec;
    }

    float GetElapsedInMilliseconds() {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        return (tp.tv_sec * 1000000 + tp.tv_usec - start_) / 1e3;
    }

    inline void clear() { start_ = 0; }

private:
    int64_t start_;
};

class Actor {
public:
    Actor(const char *representation[],
          const int width, const int height)
        : repr_(representation), width_(width), height_(height) {}

    float pos[2];
    float speed[2];

    void print_on_buffer(UDPFlaschenTaschen * frame_buffer);
    bool IsOverMe(float x, float y);

private:
    const char **repr_;
    const int width_;
    const int height_;

    Actor(); // no default constructor.
};

class PuffAnimation {
public:
    PuffAnimation(const Color &color, int steps, float radius,
                  FlaschenTaschen *display)
        : steps_(steps), radius_(radius),
          color_(color), display_(display), countdown_(0) {}

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
    FlaschenTaschen *const display_;

    int countdown_;
    int x_, y_;
};

class PongGame {
public:
    PongGame(const int socket, const int width, const int height,
             const ft::Font &font);
    ~PongGame();

    void Start(int fps, int socket_fd = -1);

private:
    // Evaluate t + 1 of the game, takes care of collisions with the players
    // and set the score in case of the ball goes in the border limit of the game.
    void sim(const float dt);

    // Simply project to pixels the pong world.
    void next_frame();

    // Reset the ball in 0 with random velocity
    void reset_ball();

private:
    const int width_;
    const int height_;
    const ft::Font &font_;

    int start_countdown_;   // Just started a game.
    int help_countdown_;    // Time displaying help

    UDPFlaschenTaschen * frame_buffer_;
    PuffAnimation edge_animation_;
    PuffAnimation new_ball_animation_;

    Actor ball_;
    Actor p1_;
    Actor p2_;
    int score_[2];

    PongGame(); // no default constructor.
};

void Actor::print_on_buffer(UDPFlaschenTaschen *frame_buffer) {
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

PongGame::PongGame(const int socket, const int width, const int height,
                   const ft::Font &font)
    : width_(width), height_(height), font_(font),
      start_countdown_(INITIAL_GAME_WAIT), help_countdown_(start_countdown_),
      frame_buffer_(new UDPFlaschenTaschen(socket, width, height)),
      edge_animation_(Color(255, 100, 0), 15, 5.0, frame_buffer_),
      new_ball_animation_(Color(0, 50, 0), 5, 3.0, frame_buffer_),
      ball_(ball, BALL_COLUMN, BALL_ROWS),
      p1_(player, PLAYER_COLUMN, PLAYER_ROWS),
      p2_(player, PLAYER_COLUMN, PLAYER_ROWS) {

    bzero(score_, sizeof(score_));

    reset_ball();  // center ball.

    p1_.pos[0] = 2;
    p1_.pos[1] = (height_ - PLAYER_ROWS) / 2;

    p2_.pos[0] = width_ - 2;
    p2_.pos[1] = (height_ - PLAYER_ROWS) / 2;
}

PongGame::~PongGame() {
    delete frame_buffer_;
}

void PongGame::reset_ball() {
    ball_.pos[0] = width_/2;
    ball_.pos[1] = height_/2-1;
    new_ball_animation_.StartNew(ball_.pos[0], ball_.pos[1]);

    // Generate random angle in the range of 1/4 tau
    const float theta = 2 * M_PI / 4 * (rand() % 100 - 50) / 100.0;
    const int direction = rand() % 2 == 0 ? -1 : 1;
    ball_.speed[0] = direction * BALL_SPEED * cos(theta);
    ball_.speed[1] = direction * BALL_SPEED * sin(theta);
}

void PongGame::Start(int fps, int socket_fd) {
    assert(fps > 0);
    printf("Waiting...");
    Timer t;
    float dt = 0;
    char command;

    // Select stuff
    fd_set rfds;
    struct timeval tv;
    int retval, bytes_read;

    int fd = socket_fd; // Default to stdout

    // UDP Socket stuff
    uint16_t recieve_data;
    struct sockaddr_in6 client_address;
    unsigned int address_length;
    uint16_t p1port, p2port;
    unsigned char p1addr[16], p2addr[16];
    address_length = sizeof(struct sockaddr);

    if (fd > 0) {
        // Wait for players to register
        // Wait first player
        fprintf(stderr,"Waiting for first player...\n");
        bytes_read = recvfrom(fd,&recieve_data,MAX_BUFFER_LENGTH,0,(struct sockaddr *)&client_address, &address_length);
        p1port = ntohs(client_address.sin6_port);
        inet_ntop(AF_INET6,&client_address,(char *)&p1addr,INET6_ADDRSTRLEN);
        fprintf(stderr,"Waiting for second player...\n");
        // Wait until the second user send something
        for (;;) {
            bytes_read = recvfrom(fd,&recieve_data,MAX_BUFFER_LENGTH,0,(struct sockaddr *)&client_address, &address_length);
            p2port = ntohs(client_address.sin6_port);
            if (p2port != p1port) {
                inet_ntop(AF_INET6,&client_address,(char *)&p2addr,INET6_ADDRSTRLEN);
                break;
            }
        }
        fprintf(stderr,"Registered p1: %s:%u, p2: %s:%u", p1addr,p1port,p2addr,p2port);
    } else {
        // Keyboard mode
        fd = 0;
        set_conio_terminal_mode();
    }

    for (;;) {
        t.Start();

        // Assume the calculations to be instant for the sleep
        sim(dt);      // Sim the pong world and handle game logic
        next_frame(); // Clear the screen, set the pixels, and send them.

        // Handle keypress
        // TODO: make this a network event thing, so that people can play
        // over a network.
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 1e6 / (float) fps;
        retval = select(fd + 1, &rfds, NULL, NULL, &tv);

        if (retval < 0)
            std::cerr << "Error on select" << std::endl;
        else if (retval && fd == 0){
            read(0, &command, sizeof(char));
            switch (command) {
            case 'd': case 'D':
                p1_.pos[1] += KEYBOARD_STEP;
                break;
            case 'w': case 'W':
                p1_.pos[1] += -KEYBOARD_STEP;
                break;
            case 'j': case 'J':
                p2_.pos[1] += KEYBOARD_STEP;
                break;
            case 'i': case 'I':
                p2_.pos[1] += -KEYBOARD_STEP;
                break;
            case '?': case '/':
                help_countdown_ = HELP_DISPLAY_TIME;
                break;
            case 'q':
                return;
            }
        } else {
            bytes_read = recvfrom(fd,&recieve_data,MAX_BUFFER_LENGTH,0,(struct sockaddr *)&client_address, &address_length);
            if (bytes_read == 2) {
                if (ntohs(client_address.sin6_port) == p2port)
                    p2_.pos[1] = ((float)(short int)ntohs(recieve_data) / (SHRT_MAX) + 1) * (height_ - PLAYER_ROWS) / 2;
                else {
                    p1_.pos[1] = ((float)(short int)ntohs(recieve_data) / (SHRT_MAX) + 1) * (height_ - PLAYER_ROWS) / 2;
                }
            }
        }
        dt = t.GetElapsedInMilliseconds();
    }
}

void PongGame::sim(const float dt) {
    if (help_countdown_ > 0)
        --help_countdown_;

    if (start_countdown_ > 0) {
        --start_countdown_;
        return;
    }

    // Evaluate the new ball delta
    ball_.pos[0] += dt * ball_.speed[0] / 1e3;
    ball_.pos[1] += dt * ball_.speed[1] / 1e3;

    // Check if bouncin on a player cursor, (careful, for low fps
    // QUANTUM TUNNELING may arise)
    if( p1_.IsOverMe(ball_.pos[0], ball_.pos[1]) || p2_.IsOverMe(ball_.pos[0], ball_.pos[1])) {
        ball_.speed[0] *= -1;
        ball_.pos[0] += 2 * dt * ball_.speed[0] / 1e3;
    }

    // Wall
    if( ball_.pos[1] < 0 || ball_.pos[1] > height_ ) ball_.speed[1] *= -1;

    // Score
    if (ball_.pos[0] < 0) {
        edge_animation_.StartNew(0, ball_.pos[1]);
        score_[1] += 1;
        ball_.pos[0] = width_ / 2;
        ball_.pos[1] = height_ / 2;
        reset_ball();
        start_countdown_ = NEW_GAME_WAIT;
    } else if (ball_.pos[0] > width_){
        edge_animation_.StartNew(width_ - 1, ball_.pos[1]);
        score_[0] += 1;
        ball_.pos[0] = width_ / 2;
        ball_.pos[1] = height_ / 2;
        reset_ball();
        start_countdown_ = NEW_GAME_WAIT;
    }
}

void PongGame::next_frame() {
    // Clear the frame
    frame_buffer_->Clear();

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

int udp_server_init(int port) {
    int server_socket;

    if ((server_socket = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("IPv6 enabled ? While reating listen socket");
        return false;
    }
    int opt = 0;   // Unset IPv6-only, in case it is set. Best effort.
    setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));

    struct sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);
    if (bind(server_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        return false;
    }

    fprintf(stderr, "UDP-server: ready to listen on %d\n", port);
    return server_socket;
}

int main(int argc, char *argv[]) {
    const char *hostname = NULL;   // Will use default if not set otherwise.
    if ( argc > 1 ) {
        hostname = argv[1];        // Hostname can be supplied as first arg
    }

    ft::Font font;
    font.LoadFont("fonts/5x5.bdf");  // TODO: don't hardcode.

    srand(time(NULL));

    // Open socket.
    const int socket = OpenFlaschenTaschenSocket(hostname);
    PongGame pong(socket, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                  font);

    fprintf(stderr, "Game keys:\n"
            "Left paddel:  'w' up; 'd' down\n"
            "Right paddel: 'i' up; 'j' down\n\n"
            "Quit with 'q'. '?' for help.\n");


    // Start the game with x fps
    pong.Start(FRAME_RATE, udp_server_init(4321));

    return 0;
}
