#include "equeue.h"
#include <stdio.h>
#include <string.h>

static equeue_t test_equeue;

static void test_call(void *obj, void *data) {
  PRINT("call==>%s %s\r\n", (char *)obj, (char *)data);
}
static void test_call_in(void *obj, void *data) {
  PRINT("call in ==>%s %s\r\n", (char *)obj, (char *)data);
}
static void test_call_every(void *obj, void *data) {
  PRINT("call every ==>%s %s\r\n", (char *)obj, (char *)data);
}

int main() {
  equeue_create(&test_equeue, 3);
  equeue_call(&test_equeue, test_call, "hello", "equeue_call!");
  equeue_call_in(&test_equeue, 10000, test_call_in, "hello", "equeue_call_in!");
  equeue_call_every(&test_equeue, 3000, test_call_every, "hello",
                    " equeue_call_every!");
  equeue_run(&test_equeue, 0);
  return 0;
}