/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void *thread_foo(void *args) {
  while (1) {
    printf("thread: I AM PRINTING...\n");
    sleep(1);
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

  sleep(5);

  err = pthread_cancel(tid);
  if (err) {
    perror("main: error canceling thread");
    return -1;
  }

  printf("main: thread cancel signal sent\n");

  sleep(2);

  int result;
  int *resultPtr = &result;

  err = pthread_join(tid, (void **)&resultPtr);
  printf("main: err = %d, EINVAL = %d, result = %d, resultPtr = %p, "
         "PTHREAD_CANCELED = %p\n",
         err, 0, result, resultPtr, PTHREAD_CANCELED);

  return 0;
}
