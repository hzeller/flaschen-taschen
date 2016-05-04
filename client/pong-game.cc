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

#include <arpa/inet.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <vector>

// Size of the display. 9 x 7 crates.
#define DEFAULT_DISPLAY_WIDTH  (9 * 5)
#define DEFAULT_DISPLAY_HEIGHT (7 * 5)

#define PLAYER_ROWS 5
#define PLAYER_COLUMN 1

#define BALL_ROWS 1
#define BALL_COLUMN 1
#define BALL_SPEED 20

#define FRAME_RATE 60
#define KEYBOARD_STEP 2
#define MAXBOUNCEANGLE (M_PI * 2 / 12.0)

#define INITIAL_GAME_WAIT (4 * FRAME_RATE)  // First start
#define NEW_GAME_WAIT     (1 * FRAME_RATE)  // Time when a new game starts
#define HELP_DISPLAY_TIME (2 * FRAME_RATE)

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

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
    PongGame(UDPFlaschenTaschen *display,
             const int width, const int height,
             const ft::Font &font, const Color &bg);
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

PongGame::PongGame(UDPFlaschenTaschen *display,
                   const int width, const int height,
                   const ft::Font &font, const Color &bg)
    : width_(width), height_(height), font_(font), background_(bg),
      start_countdown_(INITIAL_GAME_WAIT), help_countdown_(start_countdown_),
      frame_buffer_(display),
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
    Timer t;
    float dt = 0;
    char command;

    // Select stuff
    fd_set rfds;
    struct timeval tv;
    int retval, bytes_read;

    int fd = socket_fd; // Default to stdout

    // UDP Socket stuff
    uint16_t receive_data;
    struct sockaddr_in6 client_address;
    unsigned int address_length;
    uint16_t p1port = 0, p2port = 0;
    unsigned char p1addr[16], p2addr[16];
    address_length = sizeof(struct sockaddr);

    if (fd > 0) {
        // Wait first player
        fprintf(stderr, "Waiting for first player...(Move something)\n");
        bytes_read = recvfrom(fd, &receive_data, sizeof(receive_data),
                              0, (struct sockaddr *) &client_address,
                              &address_length);
        p1port = ntohs(client_address.sin6_port);
        inet_ntop(AF_INET6,&client_address,(char *)&p1addr,INET6_ADDRSTRLEN);
        fprintf(stderr,"Waiting for second player...(Move something)\n");
        // Wait until second player send something
        for (;;) {
            bytes_read = recvfrom(fd, &receive_data, sizeof(receive_data),
                                  0, (struct sockaddr *) &client_address,
                                  &address_length);
            p2port = ntohs(client_address.sin6_port);
            if (p2port != p1port) {
                inet_ntop(AF_INET6, &client_address, (char *) &p2addr, INET6_ADDRSTRLEN);
                break;
            }
        }
        // Also prints ipv4 into ipv6 format
        fprintf(stderr, "Registered p1: %s:%u, p2: %s:%u\n", p1addr, p1port, p2addr, p2port);
    } else {
        // Keyboard mode
        fprintf(stderr, "Game keys:\n"
                "Left paddel:  'w' up; 'd' down\n"
                "Right paddel: 'i' up; 'j' down\n\n"
                "Quit with 'q'. '?' for help.\n");
        fd = 0;
        set_conio_terminal_mode();
    }

    while (!interrupt_received) {
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
        else if (retval && fd == 0 && read(0, &command, sizeof(char)) == 1) {
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
            case 3:   // In raw mode, we receive Ctrl-C as raw byte.
            case 'q':
                return;
            }
        } else {
            bytes_read = recvfrom(fd, &receive_data, sizeof(receive_data),
                                  0, (struct sockaddr *) &client_address,
                                  &address_length);
            if (bytes_read == 2) {
                const int16_t value = ntohs(receive_data);
                if (ntohs(client_address.sin6_port) == p2port)
                    p2_.pos[1] = (int)(((float)value / (SHRT_MAX) + 1) * (height_ - PLAYER_ROWS) / 2);
                else {
                    p1_.pos[1] = (int)(((float)value / (SHRT_MAX) + 1) * (height_ - PLAYER_ROWS) / 2);
                }
            }
        }
        dt = t.GetElapsedInMilliseconds();
    }
}

void PongGame::sim(const float dt) {
    float angle;
    float new_pos[2];
    if (help_countdown_ > 0)
        --help_countdown_;

    if (start_countdown_ > 0) {
        --start_countdown_;
        return;
    }

    new_pos[0] = ball_.pos[0] + dt * ball_.speed[0] / 1e3;
    new_pos[1] = ball_.pos[1] + dt * ball_.speed[1] / 1e3;

    // Check if bouncin on a player cursor, (careful, for low fps
    // QUANTUM TUNNELING may arise.
    // TODO: we should check if we are going through a paddle from one frame to the other)
    if(p1_.IsOverMe(new_pos[0], new_pos[1])) {
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
    if( ball_.pos[1] < 0 || ball_.pos[1] > height_ ) ball_.speed[1] *= -1;

    // Evaluate the new ball delta
    ball_.pos[0] += dt * ball_.speed[0] / 1e3;
    ball_.pos[1] += dt * ball_.speed[1] / 1e3;

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

int udp_server_init(int port) {
    int server_socket;

    if ((server_socket = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("IPv6 enabled ? While reating listen socket");
        return false;
    }
    int opt = 0;   // Unset IPv6-only, in case it is set. Best effort.
    setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));

    // Set a non blocking socket
    int flags = fcntl(server_socket, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(server_socket, F_SETFL, flags);

    struct sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);
    if (bind(server_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Error on bind");
        return -1;
    }

    fprintf(stderr, "UDP-server: ready to listen on %d\n", port);
    return server_socket;
}

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-g <width>x<height>[+<off_x>+<off_y>[+<layer>]] : Output geometry. Default 45x35\n"
            "\t-l <layer>      : Layer 0..15. Default 0 (note if also given in -g, then last counts)\n"
            "\t-h <host>       : Flaschen-Taschen display hostname.\n"
            "\t-r <port>       : remote operation: listen on port\n"
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
    int remote_port = -1;
    Color bg(0, 0, 0);

    int opt;
    while ((opt = getopt(argc, argv, "g:l:h:r:b:")) != -1) {
        switch (opt) {
        case 'g':
            if (sscanf(optarg, "%dx%d%d%d%d", &width, &height, &off_x, &off_y, &off_z)
                < 2) {
                fprintf(stderr, "Invalid size spec '%s'", optarg);
                return usage(argv[0]);
            }
            break;
        case 'r':
            remote_port = atoi(optarg);
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
    PongGame pong(&display, width, height, font, bg);

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // Start the game with x fps
    pong.Start(FRAME_RATE, remote_port > 0 ? udp_server_init(remote_port) : -1);
    reset_terminal_mode();

    fprintf(stderr, "Good bye.\n");
    return 0;
}
