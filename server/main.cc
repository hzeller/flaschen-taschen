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
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "composite-flaschen-taschen.h"
#include "priority-flaschen-taschen-sender.h"
#include "ft-thread.h"
#include "led-flaschen-taschen.h"
#include "multi-spi.h"
#include "servers.h"

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-D <width>x<height> : Output dimension. Default 45x35\n"
            "\t-I <interface>      : Which network interface to wait for\n"
            "\t                      to be ready (e.g eth0. Empty string '' for no "
            "waiting).\n"
            "\t                      Default ''\n"
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
    bool run_opc = false;

    enum LongOptionsOnly {
        OPT_OPC_SERVER = 1000,
        OPT_PIXEL_PUSHER = 1001,
    };

    static struct option long_options[] = {
        { "interface",          required_argument, NULL, 'I'},
        { "dimension",          required_argument, NULL, 'D'},
        { "opc",                no_argument,       NULL,  OPT_OPC_SERVER },
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
        case 'I':
            interface = optarg;
            break;
        case OPT_OPC_SERVER:
            run_opc = true;
            break;
        default:
            return usage(argv[0]);
        }
    }

#if FT_BACKEND == 0
    MultiSPI spi;
    ColumnAssembly column_disp(&spi);
    // Looking from the back of the display: leftmost column first.
    column_disp.AddColumn(new WS2801FlaschenTaschen(&spi, MultiSPI::SPI_P19, 4));
    column_disp.AddColumn(new WS2801FlaschenTaschen(&spi, MultiSPI::SPI_P20, 4));
    column_disp.AddColumn(new WS2801FlaschenTaschen(&spi, MultiSPI::SPI_P15, 4));
    column_disp.AddColumn(new WS2801FlaschenTaschen(&spi, MultiSPI::SPI_P16, 4));
    column_disp.AddColumn(new StackedColumn(
           new LPD6803FlaschenTaschen(&spi, MultiSPI::SPI_P11, 2),
           new WS2801FlaschenTaschen(&spi, MultiSPI::SPI_P12, 2)));
    // Wrap in an implementation that executes Send() in high-priority thread
    // to prevent possible timing glitches.
    PriorityFlaschenTaschenSender display(&column_disp);
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

    // Only after we have become a daemon, we can do all the things that
    // require starting threads. These can be various realtime priorities,
    // we so need to stay root until all threads are set up.
    display.PostDaemonInit();

    display.Send();  // Clear screen.

    ft::Mutex mutex;

    // The display we expose to the user provides composite layering which can
    // be used by the UDP server.
    CompositeFlaschenTaschen layered_display(&display, 16);
    layered_display.StartLayerGarbageCollection(&mutex, 10);

    // Optional services as thread.
    if (run_opc) opc_server_run_thread(&layered_display, &mutex);

    udp_server_run_blocking(&layered_display, &mutex);  // last server blocks.
}
