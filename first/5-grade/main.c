/*
 * Copyright (c) 2023, Balashov Vyachesav
 */

#include "my-coroutine.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

void *routine(void *arg) {
  for (int i = 0; i < (long long)arg; ++i) {
    printf("thread %p: alive\n", arg);
    usleep(100000);
  }
  return NULL;
}

void *infinity_routine(void *arg) {
  while (1) {
    printf("thread %p: infinity\n", arg);
    usleep(100000);
  }
  return NULL;
}

void main_routine(int argc, char *argv[]) {
  mycoroutine_t c1;
  mycoroutine_t c2;
  mycoroutine_t c3;
  mycoroutine_create(&c1, routine, (void *)3);
  mycoroutine_create(&c2, routine, (void *)6);
  mycoroutine_create(&c3, infinity_routine, (void *)9);
  for (int i = 0; i < 13; ++i) {
    printf("thread main: alive\n");
    usleep(100000);
  }

  mycoroutine_join(c1, NULL);
  mycoroutine_detach(c2);
  mycoroutine_cancel(c3);
  printf("main: c3 canceled\n");
  for (int i = 0; i < 3; ++i) {
    printf("thread main: alive\n");
    sleep(1);
  }
}

int main() { return scheduler_init(main_routine, 0, NULL); }
