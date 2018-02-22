#ifndef __EQUEUE_CONFIG_H__
#define __EQUEUE_CONFIG_H__
/**
A simple equeue drive framework

this is a config file
**/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// supported platforms
#define EQUEUE_PLATFORM_NO_OS
// #define EQUEUE_PLATFORM_POSIX
// #define equeue_PLATFORM_RTTHREAD

#define MAX_EVENT_NAME 16

#if !defined(EQUEUE_PLATFORM_NO_OS) && !defined(EQUEUE_PLATFORM_POSIX) &&      \
    !defined(equeue_PLATFORM_RTTHREAD)
#warning "Unknown platform! Please update equeue_config.h"
#endif

// Platform includes
#if defined(EQUEUE_PLATFORM_POSIX)
#include <pthread.h>
#define equeue_malloc malloc
#define equeue_free free
#elif defined(equeue_PLATFORM_RTTHREAD)

#elif defined(EQUEUE_PLATFORM_NO_OS)
#define equeue_malloc malloc
#define equeue_free free
#endif

typedef unsigned int equeue_tick_t;
#define MAX_TICK 0xFFFFFFFF
#define EQUEUE_INLINE static __inline

// Platform us counter
equeue_tick_t equeue_tick(void);

// Platform mutex type
#if defined(EQUEUE_PLATFORM_POSIX)
typedef pthread_mutex_t equeue_mutex_t;
#elif defined(EQUEUE_PLATFORM_NO_OS)
typedef unsigned equeue_mutex_t;
#endif

// Platform mutex operations
int equeue_mutex_create(equeue_mutex_t *mutex);
void equeue_mutex_destroy(equeue_mutex_t *mutex);
void equeue_mutex_lock(equeue_mutex_t *mutex);
void equeue_mutex_unlock(equeue_mutex_t *mutex);

// Platform semaphore type
#if defined(EQUEUE_PLATFORM_NO_OS)
typedef volatile int equeue_sem_t;
#endif

// Platform semaphore operations
int equeue_sema_create(equeue_sem_t *sem);
void equeue_sema_destroy(equeue_sem_t *sem);
void equeue_sema_signal(equeue_sem_t *sem);
bool equeue_sema_wait(equeue_sem_t *sem, int ms);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
