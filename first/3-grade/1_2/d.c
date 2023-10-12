/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

void *thread_foo(void *arg) {
  printf("%lu\n", pthread_self());
  return NULL;
}

int main() {
  pthread_t tid;
  int err;
  pthread_attr_t attr;

  err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (err) {
    perror("main: setting attr");
    return -1;
  }

  while (1) {
    err = pthread_create(&tid, &attr, thread_foo, NULL);
    if (err) {
      perror("main: error creating thread\n");
      return -1;
    }
  }

  printf("%d\n", RLIMIT_NPROC);

  return 0;
}
