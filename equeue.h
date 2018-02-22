#ifndef __EQUEUE_H__
#define __EQUEUE_H__
/**
A simple equeue drive framework

this is a equeue header file
**/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "equeue_config.h"
#include "equeue_list.h"

typedef void (*callback)(void *obj, void *arg);

typedef enum equeue_type { EQUEUE_TIMER = 0x11, EQUEUE_EVENT } equeue_type_t;

typedef struct equeue_object {
  equeue_list_t list;
  equeue_type_t type;
  callback cb;
  void *obj;
  void *data;
} equeue_object_t;

struct equeue_event {
  equeue_object_t parent;
  char name[MAX_EVENT_NAME];
};
typedef struct equeue_event equeue_event_t;

typedef struct equeue_timer {
  equeue_object_t parent;
  unsigned int period_tick;
  unsigned int timeout_tick;
} equeue_timer_t;

struct equeue {
  equeue_mutex_t equeue_lock;
  equeue_sem_t equeue_sem;
  equeue_list_t event_list;
  equeue_list_t timer_list;
  equeue_list_t do_list;
  unsigned int do_event_count;
  int stop;
};
typedef struct equeue equeue_t;
//  init equeue container
int equeue_init(equeue_t *equeue);
// run equeue container
void equeue_run(equeue_t *equeue, equeue_tick_t ms);
// add equeue listener
int equeue_add_listener(equeue_t *equeue, const char *name, callback cb,
                        void *obj);
// remove equeue listener
int equeue_remove_listener(equeue_t *equeue, const char *name);
// dispatch event
int equeue_dispatch_event(equeue_t *equeue, const char *name, void *data);
// call
int equeue_call(equeue_t *equeue, callback cb, void *obj, void *data);
// call in
equeue_timer_t *equeue_call_in(equeue_t *equeue, equeue_tick_t ms, callback cb,
                               void *obj, void *data);
// call every
equeue_timer_t *equeue_call_every(equeue_t *equeue, equeue_tick_t ms,
                                  callback cb, void *obj, void *data);
int equeue_call_cancel(equeue_t *equeue, equeue_timer_t *timer);
int equeue_call_restart(equeue_t *equeue, equeue_timer_t *timer, int ms);
// stop equeue loop
void equeue_stop(equeue_t *equeue);
void equeue_list(equeue_t *equeue);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
