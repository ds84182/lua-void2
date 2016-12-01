#include "thread_compat.h"

// Thread compatibility shim

#if defined _WIN32

int clock_gettime(int, struct timespec *spec) {
    uint64_t wintime;
    GetSystemTimeAsFileTime((FILETIME*)&wintime);
    wintime      -= 116444736000000000ULL;  //1jan1601 to 1jan1970
    spec->tv_sec  = wintime / 10000000ULL;           //seconds
    spec->tv_nsec = wintime % 10000000ULL * 100;     //nano-seconds
    return 0;
}

static uint64_t toFileTime(const struct timespec *spec) {
    uint64_t wintime;
    wintime = spec->tv_sec * 10000000ULL;
    wintime += spec->tv_nsec % 10000000ULL * 100;
    wintime += 116444736000000000ULL;
    return wintime;
}

static uint64_t currentFileTime() {
    uint64_t wintime;
    GetSystemTimeAsFileTime((FILETIME*)&wintime);
    return wintime;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime) {
    uint64_t curtime = currentFileTime();
    uint64_t filetime = toFileTime(abstime);
    
    if (curtime > filetime) {
        return SleepConditionVariableCS(cond, mutex, 0);
    } else {
        return SleepConditionVariableCS(cond, mutex, (filetime-curtime) / 10000);
    }
}

#endif
