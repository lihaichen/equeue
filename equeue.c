#include "equeue.h"
/*
Check whether the timer expires.
*/
static int equeue_timer_check(equeue_t *equeue);
/*
Insert timer list according to time.
*/
static void equeue_insert_timer_bytime(equeue_t *equeue, equeue_timer_t *timer);

/*
  this function call malloc,do not call this func in interrupt context.
*/
int equeue_create(equeue_t *equeue, int event_size) {
  int err = -1;
  char *p = equeue_malloc(event_size * EQUEUE_MATE_SIZE);
  if (p == NULL || equeue == NULL || event_size < 1) {
    return err;
  }
  err = equeue_init(equeue, p, event_size);
  equeue->is_malloc = 1;
  return err;
}

int equeue_init(equeue_t *equeue, char *buf, int event_size) {
  int err = -1;
  if (equeue == NULL || buf == NULL || event_size < 1)
    return err;
  err = equeue_mutex_create(&equeue->equeue_lock);
  if (err < 0)
    return err;
  memset(equeue, 0, sizeof(equeue_t));
  memset(buf, 0, event_size * EQUEUE_MATE_SIZE);
  err = equeue_sema_create(&equeue->equeue_sem);
  equeue_list_init(&equeue->do_list);
  equeue_list_init(&equeue->event_list);
  equeue_list_init(&equeue->timer_list);
  equeue->do_event_count = 0;
  equeue->total_count = event_size;
  equeue->buffer = buf;
  return 0;
}

void equeue_run(equeue_t *equeue, equeue_tick_t ms) {
  equeue_tick_t tick = equeue_tick();
  // equeue_tick_t timeout = tick + ms;
  if (equeue == NULL)
    return;
  while (1) {
    equeue_timer_check(equeue);
    while (!equeue_list_isempty(&equeue->do_list)) {
      equeue_object_t *object =
          equeue_list_entry(equeue->do_list.next, equeue_object_t, list);
      tick = equeue_tick();
      if (object->cb) {
        object->cb(object->obj, object->data);
      }
      equeue_mutex_lock(&equeue->equeue_lock);
      equeue_list_remove(equeue->do_list.next);
      equeue_mutex_unlock(&equeue->equeue_lock);
      equeue->do_event_count++;
      switch (object->type) {
      case EQUEUE_TIMER: {
        equeue_timer_t *timer = (equeue_timer_t *)object;
        if (timer->period_tick == 0) {
          timer->parent.is_use = 0;
          if (equeue->use_count > 1)
            equeue->use_count--;
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
    equeue_sema_wait(&equeue->equeue_sem, 2);
  }
}

int equeue_call(equeue_t *equeue, callback cb, void *obj, void *data) {
  equeue_timer_t *timer = NULL;
  int i = 0;
  if (equeue == NULL)
    return -1;
  if (equeue->use_count >= equeue->total_count) {
    return -1;
  }
  for (i = 0; i < equeue->total_count; i++) {
    equeue_object_t *obj =
        (equeue_object_t *)(equeue->buffer + EQUEUE_MATE_SIZE * i);
    if (!obj->is_use)
      break;
  }
  if (i >= equeue->total_count) {
    PRINT("equeue_call error\r\n");
    return -1;
  }
  timer = (equeue_timer_t *)(equeue->buffer + EQUEUE_MATE_SIZE * i);
  timer->parent.is_use = 1;
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
  int i = 0;
  if (equeue == NULL)
    return timer;

  if (equeue->use_count >= equeue->total_count) {
    return timer;
  }
  for (i = 0; i < equeue->total_count; i++) {
    equeue_object_t *obj =
        (equeue_object_t *)(equeue->buffer + EQUEUE_MATE_SIZE * i);
    if (!obj->is_use)
      break;
  }
  if (i >= equeue->total_count) {
    PRINT("equeue_call error\r\n");
    return timer;
  }
  timer = (equeue_timer_t *)(equeue->buffer + EQUEUE_MATE_SIZE * i);
  timer->parent.is_use = 1;
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
  int i = 0;
  if (equeue == NULL)
    return timer;

  if (equeue->use_count >= equeue->total_count) {
    return timer;
  }
  for (i = 0; i < equeue->total_count; i++) {
    equeue_object_t *obj =
        (equeue_object_t *)(equeue->buffer + EQUEUE_MATE_SIZE * i);
    if (!obj->is_use)
      break;
  }
  if (i >= equeue->total_count) {
    PRINT("equeue_call_every error\r\n");
    return timer;
  }
  timer = (equeue_timer_t *)(equeue->buffer + EQUEUE_MATE_SIZE * i);
  timer->parent.is_use = 1;
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
      timer->parent.is_use = 0;
      if (equeue->use_count > 1)
        equeue->use_count--;
      equeue_list_remove(&timer->parent.list);
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
  event->parent.is_use = 0;
  if (equeue->use_count > 1)
    equeue->use_count--;
  equeue_mutex_unlock(&equeue->equeue_lock);
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
  PRINT("event: \r\n");
  equeue_mutex_lock(&equeue->equeue_lock);
  for (; node != equeue->event_list.prev; node = node->next) {
    event =
        (equeue_event_t *)equeue_list_entry(node->next, equeue_object_t, list);
    PRINT("0x%p %s  ", &event->parent, event->name);
  }
  PRINT("\r\n");
  equeue_mutex_unlock(&equeue->equeue_lock);
  PRINT("timer: \r\n");
  equeue_mutex_lock(&equeue->equeue_lock);
  node = &equeue->timer_list;
  for (; node != equeue->timer_list.prev; node = node->next) {
    timer =
        (equeue_timer_t *)equeue_list_entry(node->next, equeue_object_t, list);
    PRINT("0x%p %d-%d  ", &timer->parent, timer->timeout_tick,
          timer->period_tick);
  }
  PRINT("\r\n");
  equeue_mutex_unlock(&equeue->equeue_lock);
}

/*
Check whether the timer expires.
*/
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

/*
Insert timer list according to time.
*/
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
