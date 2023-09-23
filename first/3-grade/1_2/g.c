/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE

#include <asm-generic/errno-base.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

void *thread_foo(void *arg) {
  // sleep(10);
  printf("thread: finished\n");
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

  err = pthread_create(&tid, &attr, thread_foo, NULL);
  if (err) {
    perror("main: error creating thread\n");
    return -1;
  }

  // err = pthread_detach(tid);
  // if (err) {
  // perror("main: error detaching thread\n");
  // return -1;
  // }

  sleep(10);

  err = pthread_join(tid, NULL);
  if (err == EINVAL) {
    printf("THREAD IS DETACHED\n");
  }
  return 0;
}
