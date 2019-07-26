// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Copyright (C) 2016 Henner Zeller <h.zeller@acm.org>
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

#include "udp-flaschen-taschen.h"

#include "bdf-font.h"

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#define DEFAULT_WIDTH 45
#define DEFAULT_HEIGHT 35
#define WIDEST_GLYPH 'W'

volatile bool got_ctrl_c = false;
static void InterruptHandler(int) {
  got_ctrl_c = true;
}

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options] [<TEXT>|-i <textfile>]\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-g <width>x<height>[+<off_x>+<off_y>[+<layer>]] : Output geometry. Default 45x<font-height>+0+0+1\n"
            "\t-l <layer>      : Layer 0..15. Default 1 (note if also given in -g, then last counts)\n"
            "\t-h <host>       : Flaschen-Taschen display hostname.\n"
            "\t-f <fontfile>   : Path to *.bdf font file\n"
            "\t-i <textfile>   : Optional: read text from file. '-' for stdin.\n"
            "\t-s<ms>          : Scroll milliseconds per pixel (default 60). 0 for no-scroll. Negative for opposite direction.\n"
            "\t-O              : Only run once, don't scroll forever.\n"
            "\t-S<px>          : Letter spacing in pixels (default: 0)\n"
            "\t-c<RRGGBB>      : Text color as hex (default: FFFFFF)\n"
            "\t-b<RRGGBB>      : Background color as hex (default: 000000)\n"
            "\t-o<RRGGBB>      : Outline color as hex (default: no outline)\n"
            "\t-v              : Scroll text vertically \n"
            );

    return 1;
}

int main(int argc, char *argv[]) {
    int width = -1;
    int height = -1;
    int off_x = 0;
    int off_y = 0;
    int off_z = 1;
    int scroll_delay_ms = 50;
    int letter_spacing = 0;
    bool run_forever = true;
    bool vertical = false;
    bool with_outline = false;
    bool reverse = false;
    const char *host = NULL;
    std::string textfilename;

    Color fg(0xff, 0xff, 0xff);
    Color bg(0, 0, 0);
    Color outline(0, 0, 0);
    unsigned int r, g, b;

    ft::Font text_font;
    int opt;
    while ((opt = getopt(argc, argv, "f:g:h:s:vo:c:b:l:OS:i:")) != -1) {
        switch (opt) {
        case 'g':
            if (sscanf(optarg, "%dx%d%d%d%d", &width, &height, &off_x, &off_y, &off_z)
                < 2) {
                fprintf(stderr, "Invalid size spec '%s'", optarg);
                return usage(argv[0]);
            }
            break;
        case 'h':
            host = strdup(optarg); // leaking. Ignore.
            break;
        case 'i':
            textfilename = optarg;
            break;
        case 'f':
            if (!text_font.LoadFont(optarg)) {
                fprintf(stderr, "Couldn't load font '%s'\n", optarg);
            }
            break;
        case 'O':
            run_forever = false;
            break;
        case 'S':
            letter_spacing = atoi(optarg);
            break;
        case 'l':
            if (sscanf(optarg, "%d", &off_z) != 1 || off_z < 0 || off_z >= 16) {
                fprintf(stderr, "Invalid layer '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 's':
            scroll_delay_ms = atoi(optarg);
            if (scroll_delay_ms < 0) {
                reverse = true;
                scroll_delay_ms = -scroll_delay_ms;
            }
            if (scroll_delay_ms > 0 && scroll_delay_ms < 10) {
                // Don't do crazy packet sending.
                scroll_delay_ms = 10;
            }
            break;
        case 'c':
            if (optarg[0] == '-'
                || sscanf(optarg, "%02x%02x%02x", &r, &g, &b) != 3) {
                fprintf(stderr, "Foreground color parse error (%s)\n", optarg);
                return usage(argv[0]);
            }
            fg.r = r; fg.g = g; fg.b = b;
            break;
        case 'b':
            if (optarg[0] == '-'
                || sscanf(optarg, "%02x%02x%02x", &r, &g, &b) != 3) {
                fprintf(stderr, "Background color parse error (%s)\n", optarg);
                return usage(argv[0]);
            }
            bg.r = r; bg.g = g; bg.b = b;
            break;
        case 'o':
            if (optarg[0] == '-'
                || sscanf(optarg, "%02x%02x%02x", &r, &g, &b) != 3) {
                fprintf(stderr, "Outline color parse error (%s)\n", optarg);
                return usage(argv[0]);
            }
            outline.r = r; outline.g = g; outline.b = b;
            with_outline = true;
            break;
        case 'v':
            vertical = true;
            break;

        default:
            return usage(argv[0]);
        }
    }

    // check for valid initial conditions
    if (text_font.height() < 0) {
        fprintf(stderr, "Need to provide a font.\n");
        return usage(argv[0]);
    }

    ft::Font *measure_font = &text_font;
    ft::Font *outline_font = NULL;
    if (with_outline) {
        outline_font = text_font.CreateOutlineFont();
        measure_font = outline_font;
    }

    // check height input and use default value if necessary
    if (height < 0) {
        height = vertical ? DEFAULT_HEIGHT : measure_font->height();
    }

    // check width input and use default font width of WIDEST_GLYPH if necessary
    if (width < 0) {
        width = vertical ? measure_font->CharacterWidth(WIDEST_GLYPH) : DEFAULT_WIDTH;
    }

    if (width < 1 || height < 1) {
        fprintf(stderr, "%dx%d is a rather unusual size\n", width, height);
        return usage(argv[0]);
    }

    int fd = OpenFlaschenTaschenSocket(host);
    if (fd < 0) {
        fprintf(stderr, "Cannot connect.\n");
        return 1;
    }

    UDPFlaschenTaschen display(fd, width, height);
    display.SetOffset(off_x, off_y, off_z);

    std::string str;
    if (textfilename == "-") textfilename = "/dev/stdin";
    if (!textfilename.empty()) {
        int fd = open(textfilename.c_str(), O_RDONLY);
        if (fd < 0) { perror("Opening textfile"); return 1; }
        char buf[1024];
        int r;
        while ((r = read(fd, buf, sizeof(buf))) > 0) {
            str.append(buf, r);
        }
        close(fd);
    }
    // Assemble all non-option arguments to one text.
    for (int i = optind; i < argc; ++i) {
        str.append(argv[i]).append(" ");
    }
    const char *text = str.c_str();

    // Eat leading spaces and figure out if we end up with any
    // text at all.
    while (isspace(*text)) {
        ++text;
    }
    if (!*text) {
        fprintf(stderr, "This looks like a very empty text.\n");
        return 1;
    }

    // Center in in the available display space.
    const int y_pos = (height - measure_font->height()) / 2
        + measure_font->baseline();
    const int x_pos = (width - measure_font->CharacterWidth(WIDEST_GLYPH)) / 2
        + (with_outline ? 1 : 0);

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // This nested 'if' is a mess.

    // scrolling horizontally l-r or vertically b-t
    if (scroll_delay_ms > 0) {
        if (!vertical) {
            // Dry run to determine total width.
            const int total_width = DrawText(&display, *measure_font,
                                             0, 0, fg, NULL, text,
                                             letter_spacing);
            do {
                for (int s = 0; s < total_width + width && !got_ctrl_c; ++s) {
                    const int scroll_pos = reverse
                        ? -total_width + s
                        : width - s;
                    display.Fill(bg);
                    if (outline_font) {
                        DrawText(&display, *outline_font, scroll_pos, y_pos,
                                 outline, NULL, text, letter_spacing - 2);
                    }
                    DrawText(&display, text_font, scroll_pos + 1, y_pos,
                             fg, NULL, text, letter_spacing);
                    display.Send();
                    usleep(scroll_delay_ms * 1000);
                }
            } while (run_forever && !got_ctrl_c);
        }
        else {  // Vertical banner text
            // Dry run to determine total height.
            const int total_height = VerticalDrawText(&display, *measure_font,
                                                      0, 0, fg, NULL, text,
                                                      letter_spacing);
            do {
                for (int s = 0; s < total_height + height && !got_ctrl_c; ++s) {
                    display.Fill(bg);
                    const int scroll_pos = reverse
                        ? (-total_height + measure_font->height() + s)
                        : (height + measure_font->height() - s);
                    if (outline_font) {
                        VerticalDrawText(&display, *outline_font, x_pos - 1,
                                         scroll_pos,
                                         outline, NULL,
                                         text, letter_spacing - 2);
                    }
                    VerticalDrawText(&display, text_font, x_pos,
                                     scroll_pos,
                                     fg, NULL, text, letter_spacing);
                    display.Send();
                    usleep(scroll_delay_ms * 1000);
                }
            } while (run_forever && !got_ctrl_c);
        }
    }
    // No scrolling, just show directly and once.
    else if (scroll_delay_ms == 0) {
        if (!vertical) {
            display.Fill(bg);
            if (outline_font) {
                DrawText(&display, *outline_font, 0, y_pos, outline, NULL,
                         text, letter_spacing - 2);
            }
            DrawText(&display, text_font, 1, y_pos, fg, NULL,
                     text, letter_spacing);
            display.Send();
        }
        else {
            display.Fill(bg);
            if (outline_font) {
                VerticalDrawText(&display, *outline_font,
                                 x_pos - 1, measure_font->height() - 1,
                                 outline, NULL, text, letter_spacing - 2);
            }
            VerticalDrawText(&display, text_font, x_pos,
                             measure_font->height() - 1,
                             fg, NULL, text, letter_spacing);
            display.Send();
        }
    }

    // Don't let leftovers cover up content unless we set a no-scrolling
    // text explicitly
    if (off_z > 0 && scroll_delay_ms > 0) {
        display.Clear();
        display.Send();
    }

    close(fd);

    if (got_ctrl_c) {
        fprintf(stderr, "Interrupted. Exit.\n");
    }
    return 0;
}
