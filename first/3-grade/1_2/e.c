/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#include <asm-generic/errno-base.h>
#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

void *thread_foo(void *arg) {
  sleep(10);

  return NULL;
}

int main() {
  pthread_t tid;
  int err;

  err = pthread_create(&tid, NULL, thread_foo, NULL);
  if (err) {
    perror("main: error creating thread\n");
    return -1;
  }

  err = pthread_detach(tid);
  if (err) {
    perror("main: error detaching thread\n");
    return -1;
  }

  err = pthread_join(tid, NULL);
  if (err == EINVAL) {
    printf("THREAD IS DETACHED\n");
  }

  return 0;
}
