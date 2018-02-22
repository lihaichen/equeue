#include "equeue.h"

static int equeue_timer_check(equeue_t *equeue) {
  equeue_tick_t current_tick;
  equeue_list_t *node = NULL;
  equeue_timer_t *timer = NULL;
  if (equeue == NULL)
    return -1;
  current_tick = equeue_tick();
  node = &equeue->timer_list;
  equeue_mutex_lock(&equeue->equeue_lock);
  for (; node != equeue->timer_list.prev; node = node->next) {
    timer =
        (equeue_timer_t *)equeue_list_entry(node->next, equeue_object_t, list);
    if ((current_tick - timer->timeout_tick) < MAX_TICK / 2) {
      equeue_list_remove(&timer->parent.list);
      equeue_list_insert_before(&equeue->do_list, &timer->parent.list);
    } else
      break;
  }
  equeue_mutex_unlock(&equeue->equeue_lock);
  return 0;
}

// insert by time
static void equeue_insert_timer_bytime(equeue_t *equeue,
                                       equeue_timer_t *timer) {
  equeue_timer_t *timer_compare = NULL;
  equeue_list_t *node = NULL;
  node = &equeue->timer_list;
  equeue_mutex_lock(&equeue->equeue_lock);
  for (; node != equeue->timer_list.prev; node = node->next) {
    timer_compare =
        (equeue_timer_t *)equeue_list_entry(node->next, equeue_object_t, list);
    if ((timer_compare->timeout_tick - timer->timeout_tick) == 0) {
      continue;
    } else if ((timer_compare->timeout_tick - timer->timeout_tick) <
               MAX_TICK / 2) {
      break;
    }
  }
  equeue_list_insert_after(node, &timer->parent.list);
  equeue_mutex_unlock(&equeue->equeue_lock);
}

int equeue_init(equeue_t *equeue) {
  int err;
  if (equeue == NULL)
    return -1;
  err = equeue_mutex_create(&equeue->equeue_lock);
  if (err < 0)
    return err;
  err = equeue_sema_create(&equeue->equeue_sem);
  equeue_list_init(&equeue->do_list);
  equeue_list_init(&equeue->event_list);
  equeue_list_init(&equeue->timer_list);
  equeue->do_event_count = 0;
  return 0;
}

void equeue_run(equeue_t *equeue, equeue_tick_t ms) {
  equeue_tick_t tick = equeue_tick();
  // equeue_tick_t timeout = tick + ms;
  if (equeue == NULL)
    return;
  while (1) {
    // tick = equeue_tick();
    equeue_timer_check(equeue);
    while (!equeue_list_isempty(&equeue->do_list)) {
      equeue_object_t *object =
          equeue_list_entry(equeue->do_list.next, equeue_object_t, list);
      tick = equeue_tick();
      if (object->cb) {
        object->cb(object->obj, object->data);
      }
      // tick = equeue_tick();
      equeue_mutex_lock(&equeue->equeue_lock);
      equeue_list_remove(equeue->do_list.next);
      equeue_mutex_unlock(&equeue->equeue_lock);
      equeue->do_event_count++;
      switch (object->type) {
      case EQUEUE_TIMER: {
        equeue_timer_t *timer = (equeue_timer_t *)object;
        if (timer->period_tick == 0) {
          equeue_free(timer);
        } else {
          // add timer_list
          timer->timeout_tick = tick + timer->period_tick;
          // timer->timeout_tick += timer->period_tick;
          equeue_insert_timer_bytime(equeue, timer);
        }
      } break;
      case EQUEUE_EVENT: {
        equeue_list_insert_before(&equeue->event_list, &object->list);
      } break;
      }
    }
    if (equeue->stop) {
      return;
    }
    HAL_Delay(2);
  }
}

int equeue_call(equeue_t *equeue, callback cb, void *obj, void *data) {
  equeue_timer_t *timer = NULL;
  if (equeue == NULL)
    return -1;
  timer = (equeue_timer_t *)equeue_malloc(sizeof(equeue_timer_t));
  if (timer == NULL) {
    return -1;
  }
  timer->parent.data = data;
  timer->parent.obj = obj;
  timer->parent.cb = cb;
  timer->parent.type = EQUEUE_TIMER;
  timer->period_tick = 0;
  timer->timeout_tick = 0;
  // add do_list
  equeue_mutex_lock(&equeue->equeue_lock);
  equeue_list_insert_before(&equeue->do_list, &timer->parent.list);
  equeue_mutex_unlock(&equeue->equeue_lock);
  return 0;
}

equeue_timer_t *equeue_call_in(equeue_t *equeue, equeue_tick_t ms, callback cb,
                               void *obj, void *data) {
  equeue_timer_t *timer = NULL;

  if (equeue == NULL)
    return timer;
  timer = (equeue_timer_t *)equeue_malloc(sizeof(equeue_timer_t));
  if (timer == NULL) {
    return timer;
  }
  timer->parent.data = data;
  timer->parent.obj = obj;
  timer->parent.cb = cb;
  timer->parent.type = EQUEUE_TIMER;
  timer->period_tick = 0;
  timer->timeout_tick = equeue_tick() + ms;
  equeue_insert_timer_bytime(equeue, timer);
  return timer;
}

equeue_timer_t *equeue_call_every(equeue_t *equeue, equeue_tick_t ms,
                                  callback cb, void *obj, void *data) {
  equeue_timer_t *timer = NULL;

  if (equeue == NULL)
    return timer;
  timer = (equeue_timer_t *)equeue_malloc(sizeof(equeue_timer_t));
  if (timer == NULL) {
    return timer;
  }
  timer->parent.data = data;
  timer->parent.obj = obj;
  timer->parent.cb = cb;
  timer->parent.type = EQUEUE_TIMER;
  timer->period_tick = ms;
  timer->timeout_tick = equeue_tick() + ms;
  equeue_insert_timer_bytime(equeue, timer);
  return timer;
}

int equeue_call_cancel(equeue_t *equeue, equeue_timer_t *timer) {
  int status = -1;
  equeue_list_t *node = NULL;
  equeue_timer_t *timer_compare = NULL;
  if (equeue == NULL || timer == NULL)
    return status;
  node = &equeue->timer_list;
  // just find in timer_list  not find in do_list
  equeue_mutex_lock(&equeue->equeue_lock);
  for (; node != equeue->timer_list.prev; node = node->next) {
    timer_compare =
        (equeue_timer_t *)equeue_list_entry(node->next, equeue_object_t, list);
    if (timer_compare == timer) {
      equeue_list_remove(&timer->parent.list);
      equeue_free(timer);
      status = 0;
      break;
    }
  }
  equeue_mutex_unlock(&equeue->equeue_lock);
  return status;
}

int equeue_call_restart(equeue_t *equeue, equeue_timer_t *timer, int ms) {
  int status = -1;
  equeue_list_t *node = NULL;
  equeue_timer_t *timer_compare = NULL;
  if (equeue == NULL || timer == NULL)
    return status;
  node = &equeue->timer_list;
  // just find in timer_list  not find in do_list
  equeue_mutex_lock(&equeue->equeue_lock);
  for (; node != equeue->timer_list.prev; node = node->next) {
    timer_compare =
        (equeue_timer_t *)equeue_list_entry(node->next, equeue_object_t, list);
    if (timer_compare == timer) {
      equeue_list_remove(&timer->parent.list);
      timer->timeout_tick = equeue_tick() + ms;
      equeue_insert_timer_bytime(equeue, timer);
      status = 0;
      break;
    }
  }
  equeue_mutex_unlock(&equeue->equeue_lock);
  return status;
}

int equeue_add_listener(equeue_t *equeue, const char *name, callback cb,
                        void *obj) {
  equeue_event_t *event = NULL;
  if (equeue == NULL || name == NULL)
    return -1;
  event = (equeue_event_t *)equeue_malloc(sizeof(equeue_event_t));
  if (event == NULL) {
    return -1;
  }
  memset(event->name, 0, sizeof(event->name));
  strncpy(event->name, name, MAX_EVENT_NAME - 1);
  event->parent.cb = cb;
  event->parent.data = NULL;
  event->parent.obj = obj;
  event->parent.type = EQUEUE_EVENT;
  equeue_mutex_lock(&equeue->equeue_lock);
  equeue_list_insert_after(&equeue->event_list, &event->parent.list);
  equeue_mutex_unlock(&equeue->equeue_lock);
  return 0;
}

int equeue_remove_listener(equeue_t *equeue, const char *name) {
  equeue_event_t *event = NULL;
  equeue_list_t *node = NULL;
  node = &equeue->timer_list;
  equeue_mutex_lock(&equeue->equeue_lock);
  for (; node != equeue->event_list.prev; node = node->next) {
    event =
        (equeue_event_t *)equeue_list_entry(node->next, equeue_object_t, list);
    if (strncmp(event->name, name, sizeof(event->name)) == 0) {
      equeue_list_remove(node->next);
      break;
    }
  }
  equeue_mutex_unlock(&equeue->equeue_lock);
  equeue_free(event);
  return 0;
}

int equeue_dispatch_event(equeue_t *equeue, const char *name, void *data) {
  equeue_event_t *event = NULL;
  equeue_list_t *node = NULL;
  node = &equeue->event_list;
  equeue_mutex_lock(&equeue->equeue_lock);
  for (; node != equeue->event_list.prev; node = node->next) {
    event =
        (equeue_event_t *)equeue_list_entry(node->next, equeue_object_t, list);
    if (strncmp(event->name, name, MAX_EVENT_NAME) == 0) {
      equeue_list_remove(node->next);
      event->parent.data = data;
      equeue_list_insert_before(&equeue->do_list, &event->parent.list);
      break;
    }
  }
  equeue_mutex_unlock(&equeue->equeue_lock);
  return 0;
}

void equeue_list(equeue_t *equeue) {
  equeue_event_t *event = NULL;
  equeue_timer_t *timer = NULL;
  equeue_list_t *node = NULL;
  node = &equeue->event_list;
  printf("event: \r\n");
  equeue_mutex_lock(&equeue->equeue_lock);
  for (; node != equeue->event_list.prev; node = node->next) {
    event =
        (equeue_event_t *)equeue_list_entry(node->next, equeue_object_t, list);
    printf("0x%X %s  ", &event->parent, event->name);
  }
  printf("\r\n");
  equeue_mutex_unlock(&equeue->equeue_lock);
  printf("timer: \r\n");
  equeue_mutex_lock(&equeue->equeue_lock);
  node = &equeue->timer_list;
  for (; node != equeue->timer_list.prev; node = node->next) {
    timer =
        (equeue_timer_t *)equeue_list_entry(node->next, equeue_object_t, list);
    printf("0x%X %d-%d  ", &timer->parent, timer->timeout_tick,
           timer->period_tick);
  }
  printf("\r\n");
  equeue_mutex_unlock(&equeue->equeue_lock);
}
