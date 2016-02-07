// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Flaschen Taschen Server

#include "servers.h"

#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>

#define LPD_STRIP_GPIO 11

#define DROP_PRIV_USER "daemon"
#define DROP_PRIV_GROUP "daemon"

bool drop_privs() {
    struct group *g = getgrnam(DROP_PRIV_GROUP);
    if (g == NULL) {
        perror("group lookup");
        return false;
    }
    if (setresgid(g->gr_gid, g->gr_gid, g->gr_gid) != 0) {
        perror("setresgid()");
        return false;
    }
    struct passwd *p = getpwnam(DROP_PRIV_USER);
    if (p == NULL) {
        perror("user lookup");
        return false;
    }
    if (setresuid(p->pw_uid, p->pw_uid, p->pw_uid) != 0) {
        perror("setresuid()");
        return false;
    }
    return true;
}

int main(int argc, const char *argv[]) {
    // TODO(hzeller): remove hardcodedness, provide flags
    WS2811FlaschenTaschen top_display(10, 5);
    LPD6803FlaschenTaschen bottom_display(LPD_STRIP_GPIO, 10, 5);
    StackedFlaschenTaschen display(&top_display, &bottom_display);
    display.Send();  // Initialize.

    opc_server_init(7890);

    if (!drop_privs())
        return 1;

    opc_server_run(&display);
}
