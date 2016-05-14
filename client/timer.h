
#ifndef TIMER_H
#define TIMER_H

class Timer {
public:
    Timer() : start_(0) {}

    void Start() {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        start_ = tp.tv_sec * 1000000 + tp.tv_usec;
    }

    float GetElapsedInMicroseconds() {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        return (tp.tv_sec * 1000000 + tp.tv_usec - start_);
    }

private:
    uint64_t start_;
};

#endif // TIMER_H
