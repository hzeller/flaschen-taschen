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

typedef void (*ServerRunFun)(FlaschenTaschen *, pthread_mutex_t *);
struct ServerClosure {
    ServerRunFun fun;
    FlaschenTaschen *display;
    pthread_mutex_t *mutex;
};

static void *ThreadRunner(void *data) {
    ServerClosure *closure = (ServerClosure*) data;
    closure->fun(closure->display, closure->mutex);
    return NULL;
}
static void StartServerThread(ServerRunFun fun, FlaschenTaschen *display,
                              pthread_mutex_t *mutex) {
    ServerClosure closure = { fun, display, mutex };
    pthread_t thread;
    pthread_create(&thread, NULL, &ThreadRunner, &closure);
}

int main(int argc, const char *argv[]) {
    // TODO(hzeller): remove hardcodedness, provide flags
    WS2811FlaschenTaschen top_display(10, 5);
    LPD6803FlaschenTaschen bottom_display(LPD_STRIP_GPIO, 10, 5);
    StackedFlaschenTaschen display(&top_display, &bottom_display);
    display.Send();  // Initialize with some black background.

    opc_server_init(7890);
    udp_server_init(1337);

    // After hardware is set up and all servers are listening, we can
    // drop the privileges.
    if (!drop_privs())
        return 1;

    if (daemon(0, 0) != 0) {  // Become daemon. TODO: maybe dependent on flag.
        fprintf(stderr, "Failed to become daemon");
    }

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    StartServerThread(&opc_server_run, &display, &mutex);
    udp_server_run(&display, &mutex);  // last server blocks.
}
