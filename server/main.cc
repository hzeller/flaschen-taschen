// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
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

// Flaschen Taschen Server

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "composite-flaschen-taschen.h"
#include "ft-thread.h"
#include "led-flaschen-taschen.h"
#include "servers.h"

#if FT_BACKEND == 0
#  include "multi-spi.h"
#  include "led-strip.h"
#endif

#define DROP_PRIV_USER "daemon"
#define DROP_PRIV_GROUP "daemon"

bool drop_privs(const char *priv_user, const char *priv_group) {
    uid_t ruid, euid, suid;
    if (getresuid(&ruid, &euid, &suid) >= 0) {
        if (euid != 0)   // not root anyway. No priv dropping.
            return true;
    }

    struct group *g = getgrnam(priv_group);
    if (g == NULL) {
        perror("group lookup.");
        return false;
    }
    if (setresgid(g->gr_gid, g->gr_gid, g->gr_gid) != 0) {
        perror("setresgid()");
        return false;
    }
    struct passwd *p = getpwnam(priv_user);
    if (p == NULL) {
        perror("user lookup.");
        return false;
    }
    if (setresuid(p->pw_uid, p->pw_uid, p->pw_uid) != 0) {
        perror("setresuid()");
        return false;
    }
    return true;
}

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-D <width>x<height> : Output dimension. Default 45x35\n"
            "\t-I <interface>      : Which network interface to wait for\n"
            "\t                      to be ready (e.g eth0. Empty string '' for no "
            "waiting).\n"
            "\t                      Default ''\n"
            "\t--layer-timeout <sec>: Layer timeout: clearing after non-activity (Default: 15)\n"
            "\t-d                  : Become daemon\n"
            "\t--pixel-pusher      : Run PixelPusher protocol (default: false)\n"
            "\t--opc               : Run OpenPixelControl protocol (default: false)\n"
            "(By default, only the FlaschenTaschen UDP protocol is enabled)\n"
            );
    return 1;
}

int main(int argc, char *argv[]) {
    std::string interface = "";
    int width = 45;
    int height = 35;
    int layer_timeout = 15;
    bool as_daemon = false;
    bool run_opc = false;
    bool run_pixel_pusher = false;

    enum LongOptionsOnly {
        OPT_OPC_SERVER = 1000,
        OPT_PIXEL_PUSHER = 1001,
        OPT_LAYER_TIMEOUT = 1002,
    };

    static struct option long_options[] = {
        { "interface",          required_argument, NULL, 'I'},
        { "dimension",          required_argument, NULL, 'D'},
        { "daemon",             no_argument,       NULL, 'd'},
        { "layer-timeout",      required_argument, NULL,  OPT_LAYER_TIMEOUT },
        { "opc",                no_argument,       NULL,  OPT_OPC_SERVER },
        { "pixel-pusher",       no_argument,       NULL,  OPT_PIXEL_PUSHER },
        { 0,                    0,                 0,    0  },
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "I:D:d", long_options, NULL)) != -1) {
        switch (opt) {
        case 'D':
            if (sscanf(optarg, "%dx%d", &width, &height) != 2) {
                fprintf(stderr, "Invalid size spec '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'd':
            as_daemon = true;
            break;
        case 'I':
            interface = optarg;
            break;
        case OPT_LAYER_TIMEOUT:
            layer_timeout = atoi(optarg);
            break;
        case OPT_OPC_SERVER:
            run_opc = true;
            break;
        case OPT_PIXEL_PUSHER:
            run_pixel_pusher = true;
            break;
        default:
            return usage(argv[0]);
        }
    }

    if (layer_timeout < 1) {
        layer_timeout = 1;
    }

#if FT_BACKEND == 0
    using spixels::MultiSPI;
    using spixels::CreateWS2801Strip;
    static const int kLedsPerCol = 7 * 25;
    MultiSPI *const spi = spixels::CreateDMAMultiSPI();
    ColumnAssembly display(spi);

#define MAKE_COLUMN(port) new CrateColumnFlaschenTaschen(CreateWS2801Strip(spi, port, kLedsPerCol))

    // Looking from the back of the display: leftmost column first.
    display.AddColumn(MAKE_COLUMN(MultiSPI::SPI_P8));
    display.AddColumn(MAKE_COLUMN(MultiSPI::SPI_P7));
    display.AddColumn(MAKE_COLUMN(MultiSPI::SPI_P6));
    display.AddColumn(MAKE_COLUMN(MultiSPI::SPI_P5));

    // Center column. Connected to front part
    display.AddColumn(MAKE_COLUMN(MultiSPI::SPI_P13));

    // Rest: continue on the back part
    display.AddColumn(MAKE_COLUMN(MultiSPI::SPI_P4));
    display.AddColumn(MAKE_COLUMN(MultiSPI::SPI_P3));
    display.AddColumn(MAKE_COLUMN(MultiSPI::SPI_P2));
    display.AddColumn(MAKE_COLUMN(MultiSPI::SPI_P1));
#undef MAKE_COLUMN
#elif FT_BACKEND == 1
    RGBMatrixFlaschenTaschen display(0, 0, width, height);
#elif FT_BACKEND == 2
    TerminalFlaschenTaschen display(STDOUT_FILENO, width, height);
#endif

    // Start all the services and report problems (such as sockets already
    // bound to) before we become a daemon
    if (!udp_server_init(1337)) {
        return 1;
    }

    // Optional services.
    if (run_opc && !opc_server_init(7890)) {
        return 1;
    }
    if (run_pixel_pusher
        && !pixel_pusher_init(interface.c_str(), &display)) {
        return 1;
    }

    // Commandline parsed, immediate errors reported. Time to become daemon.
    if (as_daemon && daemon(0, 0) != 0) {  // Become daemon.
        perror("Failed to become daemon");
    }

    // Only after we have become a daemon, we can do all the things that
    // require starting threads. These can be various realtime priorities,
    // we so need to stay root until all threads are set up.
    display.PostDaemonInit();

    display.Send();  // Clear screen.

    ft::Mutex mutex;

    // The display we expose to the user provides composite layering which can
    // be used by the UDP server.
    CompositeFlaschenTaschen layered_display(&display, 16);
    layered_display.StartLayerGarbageCollection(&mutex, layer_timeout);

    // Optional services as thread.
    if (run_opc) opc_server_run_thread(&layered_display, &mutex);
    if (run_pixel_pusher) pixel_pusher_run_thread(&layered_display, &mutex);

    // After hardware is set up, all servers are listening and all
    // threads are started with their respective priorities, we can drop
    // privileges.
    if (!drop_privs(DROP_PRIV_USER, DROP_PRIV_GROUP))
        return 1;

    udp_server_run_blocking(&layered_display, &mutex);  // last server blocks.
}
