#ifndef THREAD_COMPAT_H
#define THREAD_COMPAT_H

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#if defined _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

struct timespec { long tv_sec; long tv_nsec; };
#define CLOCK_REALTIME 0
int clock_gettime(int, struct timespec *spec);

typedef CRITICAL_SECTION pthread_mutex_t;
#define PTHREAD_MUTEX_INITIALIZER {(void *)-1,-1,0,0,0,0}
#define pthread_mutex_init(m,a) InitializeCriticalSection(m)
#define pthread_mutex_destroy(m) DeleteCriticalSection(m)
#define pthread_mutex_lock(m) EnterCriticalSection(m)
#define pthread_mutex_unlock(m) LeaveCriticalSection(m)
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);

typedef CONDITION_VARIABLE pthread_cond_t;
#define PTHREAD_COND_INITIALIZER {0)
#define pthread_cond_init(c,a) InitializeConditionVariable(c)
#define pthread_cond_destroy(c) (void)c
#define pthread_cond_wait(c,m) SleepConditionVariableCS(c,m,INFINITE)
#define pthread_cond_broadcast(c) WakeAllConditionVariable(c)
#define ERRNO WSAGetLastError()
#else
#include <errno.h>
#include <pthread.h>
#include <time.h>

#define ERRNO errno
#endif

#endif
