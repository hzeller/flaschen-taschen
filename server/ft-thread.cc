// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Copyright (C) 2013 Henner Zeller <h.zeller@acm.org>
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

#include "ft-thread.h"

#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#ifndef __APPLE__
#  include <time.h>      // clock_gettime()
#else
#  include <sys/time.h>  // gettimeofday()
#endif

namespace ft {
void *Thread::PthreadCallRun(void *tobject) {
    reinterpret_cast<Thread*>(tobject)->Run();
    return NULL;
}

Thread::Thread() : started_(false) {}
Thread::~Thread() {
    WaitStopped();
}

void Thread::WaitStopped() {
    if (!started_) return;
    int result = pthread_join(thread_, NULL);
    if (result != 0) {
        fprintf(stderr, "err code: %d %s\n", result, strerror(result));
    }
    started_ = false;
}

void Thread::Start(int priority, uint32_t affinity_mask) {
    assert(!started_);  // Did you call WaitStopped() ?
    pthread_create(&thread_, NULL, &PthreadCallRun, this);

    if (priority > 0) {
        struct sched_param p;
        p.sched_priority = priority;
        pthread_setschedparam(thread_, SCHED_FIFO, &p);
    }

#ifndef __APPLE__
    if (affinity_mask != 0) {
        cpu_set_t cpu_mask;
        CPU_ZERO(&cpu_mask);
        for (int i = 0; i < 32; ++i) {
            if ((affinity_mask & (1<<i)) != 0) {
                CPU_SET(i, &cpu_mask);
            }
        }
        pthread_setaffinity_np(thread_, sizeof(cpu_mask), &cpu_mask);
    }
#endif

    started_ = true;
}

bool Mutex::WaitOnWithTimeout(pthread_cond_t *cond, int wait_ms) {
    if (wait_ms <= 0) return 1;  // already expired.
    struct timespec abs_time;
#ifndef __APPLE__
    clock_gettime(CLOCK_REALTIME, &abs_time);
#else
    struct timeval apple_time;
    gettimeofday(&apple_time, NULL);
    abs_time.tv_sec = apple_time.tv_sec;
    abs_time.tv_nsec = apple_time.tv_usec * 1000;
#endif
    abs_time.tv_sec += wait_ms / 1000;
    abs_time.tv_nsec += 1000000 * (wait_ms % 1000);
    if (abs_time.tv_nsec > 1000000000) {
        abs_time.tv_sec += 1;
        abs_time.tv_nsec -= 1000000000;
    }
    return pthread_cond_timedwait(cond, &mutex_, &abs_time) == 0;
}

}  // namespace ft
