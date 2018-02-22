#include "equeue.h"
#include <stdio.h>
#include <string.h>

static equeue_t test_equeue;
static void test_call(void *obj, void *data) {
  PRINT("%s %s\r\n", (char *)obj, (char *)data);
}

int main() {
  equeue_create(&test_equeue, 2);
  // equeue_call(&test_equeue, test_call, "hello1", " lhc1");
  equeue_call_every(&test_equeue, 1000, test_call, "hello2", " lhc2");
  equeue_run(&test_equeue, 0);
  return 0;
}