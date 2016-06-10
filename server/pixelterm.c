/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>
 *
 * This file is taken from pixelterm by jaseg <pixelterm@jaseg.net>,
 * the most recent version may be found at https://github.com/jaseg/pixelterm
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>

#include "xtermcolors.h"

#define TERM_RESET      "\033[39;49m"
#define BG_RESET        "\033[49m"
#define BG_FORMAT       "\033[48;5;%hhum"
#define FG_FORMAT       "\033[38;5;%hhum"
#define FULL_BLOCK      "\xe2\x96\x88" /* █ */
#define TOP_HALF_BLOCK  "\xe2\x96\x80" /* ▀ */
#define BOT_HALF_BLOCK  "\xe2\x96\x84" /* ▄ */

ssize_t balloon(const uint32_t *img, size_t sx, size_t x, size_t y) {
    union rgba_color ref = {.c = {0, 255, 0, 127}};
    if (x+1 == sx || ref.i != img[y*sx+x+1]) {
        size_t w = 1;
        while(x-w >= 0 && ref.i == img[y*sx+x-w])
            w++;
        return w;
    }
    return -1;
}

char *termify_pixels(const uint32_t *img_rgba, size_t sx, size_t sy) {
    const size_t bufsize = sx*((sy+1)/2)*25; /* 25 → worst-case length for a single pixel, two color codes with 8+3 char
                                                ea. plus 3 byte unicode char */
    size_t bufpos = 0;
    char *buf = calloc(1, bufsize);
#define BUF_FORMAT(...) bufpos += snprintf(buf+bufpos, bufsize-bufpos, __VA_ARGS__)

    if (!buf)
        return 0;

    uint8_t xfg = -2, xbg = -1;

    for (size_t y=0; y<sy; y+=2) {
        for (size_t x=0; x<sx; x++) {
            union rgba_color coltop = {.i = img_rgba[y*sx + x]};
            union rgba_color colbot = {.i = (y+1 < sy) ? img_rgba[(y+1)*sx + x] : 0};

            if (coltop.c.a == 127) { /* Control colors */
                xfg = -2;
                xbg = -1;

                BUF_FORMAT(TERM_RESET);

                union rgba_color ref_bs = {.c = {255, 0, 0, 127}},
                                 ref_fs = {.c = {0, 0, 255, 127}},
                                 ref_bb = {.c = {0, 255, 0, 127}};
               if (coltop.i == ref_bs.i) {
                    BUF_FORMAT("$\\$");
                } else if (coltop.i == ref_fs.i) {
                    BUF_FORMAT("$/$");
                } else if (coltop.i == ref_bb.i) {
                    ssize_t w = balloon(img_rgba, sx, x, y);
                    if (w >= 0)
                        BUF_FORMAT("$balloon%zd$", w);
                }
                continue;
            }

            int xtt, xtb;
            if (coltop.c.a != 255) {
                xtt = -1;
            } else {
                xtt = xterm_closest_color(coltop.i);
            }

            if (colbot.c.a != 255) {
                xtb = -1;
            } else {
                xtb = xterm_closest_color(colbot.i);
            }

            if (xtt == xtb) { /* top and bottom are equal, draw full blocks */ 
                if (xtt == xfg) {
                    BUF_FORMAT(                                 FULL_BLOCK);
                } else if (xtt == xbg) {
                    BUF_FORMAT(" ");
                } else if (xtt == -1) {
                    xbg = xtt;
                    BUF_FORMAT(         BG_RESET                " ");
                } else {
                    xfg = xtt;
                    BUF_FORMAT(         FG_FORMAT               FULL_BLOCK, xfg);
                }
            } else { /* top and bottom differ. */
                /* try to reuse the foreground color */
                if (xtt == xfg) {
                    /* try to also reuse the background color */
                    if (xtb == xbg) {
                        BUF_FORMAT(                             TOP_HALF_BLOCK);
                    } else if (xtb == -1) {
                        xbg = -1;
                        BUF_FORMAT(                 BG_RESET    TOP_HALF_BLOCK);
                    } else {
                        xbg = xtb;
                        BUF_FORMAT(                 BG_FORMAT   TOP_HALF_BLOCK, xbg);
                    }
                } else if (xtb == xfg) {
                    /* try to also reuse the background color */
                    if (xtt == xbg) {
                        BUF_FORMAT(                             BOT_HALF_BLOCK);
                    } else if (xtt == -1) {
                        xbg = -1;
                        BUF_FORMAT(                 BG_RESET    BOT_HALF_BLOCK);
                    } else {
                        xbg = xtt;
                        BUF_FORMAT(                 BG_FORMAT   BOT_HALF_BLOCK, xbg);
                    }
                /* try to reuse the background color */
                /* here we already know we can't use the foreground color */
                } else if (xtt == xbg && xtb != -1) {
                    xfg = xtb;
                    BUF_FORMAT(         FG_FORMAT               BOT_HALF_BLOCK, xfg);
                } else if (xtb == xbg && xtt != -1) {
                    xfg = xtt;
                    BUF_FORMAT(         FG_FORMAT               TOP_HALF_BLOCK, xfg);
                } else { /* worst case, set both colors */
                    if (xtt == -1) {
                        xbg = -1;
                        xfg = xtb;
                        BUF_FORMAT(         FG_FORMAT   BG_RESET    BOT_HALF_BLOCK, xfg);
                    } else if (xtb == -1) {
                        xbg = -1;
                        xfg = xtt;
                        BUF_FORMAT(         FG_FORMAT   BG_RESET    TOP_HALF_BLOCK, xfg);
                    } else {
                        xfg = xtt;
                        xbg = xtb;
                        BUF_FORMAT(         FG_FORMAT   BG_FORMAT   TOP_HALF_BLOCK, xfg, xbg);
                    }
                }
            }
        }

        /* strip unnecessary trailing spaces */
        if (xbg == -1)
            while (buf[bufpos-1] == ' ')
                bufpos--;
        BUF_FORMAT("\n");
    }

    bufpos--;

    BUF_FORMAT("%s ", TERM_RESET);

    buf[bufpos++] = 0;

    if (bufpos < bufsize)
        return realloc(buf, bufpos);

    free(buf);
    return 0;
}

