/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void destr_func(void *args) { printf("DESTROYEEEED\n"); }

void *thread_foo(void *args) {
  unsigned long long cntr = 0;
  while (1) {
    ++cntr;
  }
  return NULL;
}

int main() {
  pthread_t tid;
  int err;

  err = pthread_create(&tid, NULL, thread_foo, NULL);
  if (err) {
    perror("main: error creating thread");
    return -1;
  }

  printf("main: thread created\n");

  sleep(3);

  err = pthread_cancel(tid);
  if (err) {
    perror("main: error canceling thread");
    return -1;
  }

  printf("main: sent thread cancel request\n");

  sleep(5);

  int *result;
  err = pthread_join(tid, (void **)&result);
  if (err) {
    perror("main: error joining thread");
    return -1;
  }

  printf("main: pthread canceled: %d\n", result == PTHREAD_CANCELED);

  printf("main: exiting...\n");

  return 0;
}
