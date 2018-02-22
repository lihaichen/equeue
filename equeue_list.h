#ifndef __EQUEUE_LIST_H__
#define __EQUEUE_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "equeue_config.h"
/**
 * Double List structure
 */
struct equeue_list_node {
  struct equeue_list_node *next;
  struct equeue_list_node *prev;
};
typedef struct equeue_list_node equeue_list_t;

#define equeue_container_of(ptr, type, member)                                 \
  ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define equeue_list_entry(node, type, member)                                  \
  equeue_container_of(node, type, member)

EQUEUE_INLINE void equeue_list_init(equeue_list_t *l) { l->next = l->prev = l; }

EQUEUE_INLINE void equeue_list_insert_after(equeue_list_t *l,
                                            equeue_list_t *n) {
  l->next->prev = n;
  n->next = l->next;

  l->next = n;
  n->prev = l;
}

EQUEUE_INLINE void equeue_list_insert_before(equeue_list_t *l,
                                             equeue_list_t *n) {
  l->prev->next = n;
  n->prev = l->prev;

  l->prev = n;
  n->next = l;
}

EQUEUE_INLINE void equeue_list_remove(equeue_list_t *n) {
  n->next->prev = n->prev;
  n->prev->next = n->next;

  n->next = n->prev = n;
}

EQUEUE_INLINE int equeue_list_isempty(const equeue_list_t *l) {
  return l->next == l;
}

EQUEUE_INLINE unsigned int equeue_list_len(const equeue_list_t *l) {
  unsigned int len = 0;
  const equeue_list_t *p = l;
  while (p->next != l) {
    p = p->next;
    len++;
  }
  return len;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
