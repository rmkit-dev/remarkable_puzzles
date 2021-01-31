#ifndef RMP_TIMER_HPP
#define RMP_TIMER_HPP

#include <sys/time.h>

namespace timer {

const int INTERVAL_MS = 100;
const double INTERVAL_S = (double)INTERVAL_MS / 1000;

inline double now() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec + (now.tv_usec * 0.000001);
}

} // namespace

#endif // RMP_TIMER_HPP
