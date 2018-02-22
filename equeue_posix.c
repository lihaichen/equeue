#include "equeue_config.h"
#if defined(EQUEUE_PLATFORM_POSIX)

equeue_tick_t equeue_tick() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int equeue_mutex_create(equeue_mutex_t *m) {
  return pthread_mutex_init(m, NULL);
}
void equeue_mutex_destroy(equeue_mutex_t *m) { pthread_mutex_destroy(m); }

void equeue_mutex_lock(equeue_mutex_t *m) { pthread_mutex_lock(m); }

void equeue_mutex_unlock(equeue_mutex_t *m) { pthread_mutex_unlock(m); }

int equeue_sema_create(equeue_sem_t *sem) { return sem_init(sem, 0, 0); }
void equeue_sema_destroy(equeue_sem_t *sem) { sem_destroy(sem); }
void equeue_sema_signal(equeue_sem_t *sem) { sem_post(sem); }
int  equeue_sema_wait(equeue_sem_t *sem, int ms) {
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  return sem_timedwait(sem, &ts);
}

#endif
