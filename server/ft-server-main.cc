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
#include <grp.h>
#include <linux/netdevice.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>

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

// Figuring out if a particular interface is already initialized.
static bool IsEthernetReady(const char *interface) {
    int s;
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return false;
    }

    bool success = true;

    struct ifreq ip_addr_query;
    strcpy(ip_addr_query.ifr_name, interface);
    if (ioctl(s, SIOCGIFADDR, &ip_addr_query) == 0) {
        struct sockaddr_in *s_in = (struct sockaddr_in *) &ip_addr_query.ifr_addr;
        char printed_ip[64];
        inet_ntop(AF_INET, &s_in->sin_addr, printed_ip, sizeof(printed_ip));
        fprintf(stderr, "%s ip: %s\n", interface, printed_ip);
    } else {
        perror("Getting IP address");
        success = false;
    }

    close(s);

    return success;
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

    // Wait up to a minute for the network to show up before we start
    // our servers that listen on 0.0.0.0
    // TODO: maybe we should fork/exec already first stage here (but still stay
    // root) to not have this happen in foreground ?
    for (int i = 0; !IsEthernetReady("eth0") && i < 60; ++i) {
        sleep(1);
    }

    opc_server_init(7890);
    pixel_pusher_init(&display);
    udp_server_init(1337);

    // After hardware is set up and all servers are listening, we can
    // drop the privileges.
    if (!drop_privs(DROP_PRIV_USER, DROP_PRIV_GROUP))
        return 1;

    if (daemon(0, 0) != 0) {  // Become daemon. TODO: maybe dependent on flag.
        fprintf(stderr, "Failed to become daemon");
    }

#if DEMO_RGB
    display.PostDaemonInit();
#endif

    display.Send();  // Clear screen.

    ft::Mutex mutex;

    OPCThread opc_thread(&display, &mutex);
    opc_thread.Start();
    pixel_pusher_run_threads(&display, &mutex);

    udp_server_run_blocking(&display, &mutex);  // last server blocks.
}
