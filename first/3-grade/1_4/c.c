/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE

#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void routine(void *args) {
  printf("routine: cleaning memory...\n");
  free(args);
}

void *thread_foo(void *args) {
  char *hello_str = (char *)malloc(sizeof(char) * 14);
  strcpy(hello_str, "Hello, World!");
  pthread_cleanup_push(routine, hello_str);
  while (1) {
    printf("thread: %s\n", hello_str);
    sleep(1);
  }
  pthread_cleanup_pop(1);
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
