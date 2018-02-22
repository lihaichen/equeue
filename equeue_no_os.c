#include "equeue_config.h"
#if defined(EQUEUE_PLATFORM_NO_OS)
#include <sys/time.h>
#include <unistd.h>

equeue_tick_t equeue_tick() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int equeue_mutex_create(equeue_mutex_t *m) { return 0; }
void equeue_mutex_destroy(equeue_mutex_t *m) {}

void equeue_mutex_lock(equeue_mutex_t *m) {}

void equeue_mutex_unlock(equeue_mutex_t *m) {}

int equeue_sema_create(equeue_sem_t *sem) { return 0; }
void equeue_sema_destroy(equeue_sem_t *sem) {}
void equeue_sema_signal(equeue_sem_t *sem) {}
int equeue_sema_wait(equeue_sem_t *sem, int ms) {
  usleep(1000);
  return 0;
}

#endif
