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

#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>

// Demo RGB demo matrix to play with until the 'big' system is there.
// 1 for RGB, 0 for rest.
#define DEMO_RGB 1

// Pin 11 on Pi
#define MULTI_SPI_COMMON_CLOCK 17

#define LPD_STRIP_GPIO 11
#define WS_R0_STRIP_GPIO 8
#define WS_R1_STRIP_GPIO 7
#define WS_R2_STRIP_GPIO 10

#define DROP_PRIV_USER "daemon"
#define DROP_PRIV_GROUP "daemon"

bool drop_privs(const char *priv_user, const char *priv_group) {
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

int main(int argc, const char *argv[]) {
#if DEMO_RGB
    RGBMatrixFlaschenTaschen display(0, 0, 45, 32);
#else
    MultiSPI spi(MULTI_SPI_COMMON_CLOCK);
    ColumnAssembly display(&spi);
    // From right to left.
    display.AddColumn(new WS2801FlaschenTaschen(&spi, WS_R0_STRIP_GPIO, 2));
    display.AddColumn(new WS2801FlaschenTaschen(&spi, WS_R1_STRIP_GPIO, 4));
    display.AddColumn(new WS2811FlaschenTaschen(2));
    display.AddColumn(new LPD6803FlaschenTaschen(&spi, LPD_STRIP_GPIO, 2));
#endif
    opc_server_init(7890);
    pixel_pusher_init(&display);
    udp_server_init(1337);

    // After hardware is set up and all servers are listening, we can
    // drop the privileges.
    if (true && !drop_privs(DROP_PRIV_USER, DROP_PRIV_GROUP))
        return 1;

    if (true && daemon(0, 0) != 0) {  // Become daemon. TODO: maybe dependent on flag.
        fprintf(stderr, "Failed to become daemon");
    }

    display.Send();  // Clear screen.

    ft::Mutex mutex;
#if 1
    OPCThread opc_thread(&display, &mutex);
    opc_thread.Start();
    pixel_pusher_run_threads(&display, &mutex);

    udp_server_run_blocking(&display, &mutex);  // last server blocks.
#endif
}
