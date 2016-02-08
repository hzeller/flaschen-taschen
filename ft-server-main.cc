// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Flaschen Taschen Server

#include "servers.h"

#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>

#define LPD_STRIP_GPIO 11

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
    // TODO(hzeller): remove hardcodedness, provide flags
    WS2811FlaschenTaschen top_display(10, 5);
    LPD6803FlaschenTaschen bottom_display(LPD_STRIP_GPIO, 10, 5);

    // Our LPD6803 look a little blue-greenish.
    bottom_display.SetColorCorrect(1.0, 0.8, 0.8);

    StackedFlaschenTaschen display(&top_display, &bottom_display);
    display.Send();  // Initialize with some black background.

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

    ft::Mutex mutex;

    OPCThread opc_thread(&display, &mutex);
    opc_thread.Start();
    pixel_pusher_run_threads(&display, &mutex);

    udp_server_run_blocking(&display, &mutex);  // last server blocks.
}
