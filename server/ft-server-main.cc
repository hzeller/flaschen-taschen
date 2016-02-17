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

#include "servers.h"
#include "led-flaschen-taschen.h"
#include "multi-spi.h"

#include <arpa/inet.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

// Pin 11 on Pi
#define MULTI_SPI_COMMON_CLOCK 17

#define LPD_STRIP_GPIO 11
#define WS_R0_STRIP_GPIO 8
#define WS_R1_STRIP_GPIO 7
#define WS_R2_STRIP_GPIO 10

class OPCThread : public ft::Thread {
public:
    OPCThread(FlaschenTaschen *display, ft::Mutex *mutex)
        : display_(display), mutex_(mutex) {}
    virtual void Run() {
        opc_server_run_blocking(display_, mutex_);
    }

private:
    FlaschenTaschen *const display_;
    ft::Mutex *const mutex_;
};

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-D <width>x<height> : Output dimension. Default 45x35\n"
            "\t-I <interface>      : Which network interface to wait for\n"
            "\t                      to be ready (Empty string '' for no "
            "waiting)."
            "Default 'eth0'\n"
            "\t-d                  : Become daemon\n");
    return 1;
}

int main(int argc, char *argv[]) {
    std::string interface = "eth0";
    int width = 45;
    int height = 35;
    bool as_daemon = false;

    static struct option long_options[] = {
        { "interface",          required_argument, NULL, 'I'},
        { "dimension",          required_argument, NULL, 'D'},
        { "daemon",             no_argument,       NULL, 'd'},
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
        default:
            return usage(argv[0]);
        }
    }

#if FT_BACKEND == 0
    MultiSPI spi(MULTI_SPI_COMMON_CLOCK);
    ColumnAssembly display(&spi);
    // From right to left.
    display.AddColumn(new WS2801FlaschenTaschen(&spi, WS_R0_STRIP_GPIO, 2));
    display.AddColumn(new WS2801FlaschenTaschen(&spi, WS_R1_STRIP_GPIO, 4));
    display.AddColumn(new WS2811FlaschenTaschen(2));
    display.AddColumn(new LPD6803FlaschenTaschen(&spi, LPD_STRIP_GPIO, 2));
#elif FT_BACKEND == 1
    RGBMatrixFlaschenTaschen display(0, 0, width, height);
#elif FT_BACKEND == 2
    TerminalFlaschenTaschen display(width, height);
#endif

    opc_server_init(7890);
    udp_server_init(1337);

#if FT_BACKEND == 1
    display.PostDaemonInit();
#endif

    display.Send();  // Clear screen.

    ft::Mutex mutex;

    OPCThread opc_thread(&display, &mutex);
    opc_thread.Start();

    udp_server_run_blocking(&display, &mutex);  // last server blocks.
}
