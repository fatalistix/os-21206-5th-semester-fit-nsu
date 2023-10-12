/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg) {
  printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(),
         gettid());
  return NULL;
}

#define NUM_THREADS 5

int main() {
  pthread_t tids[NUM_THREADS];
  int err;

  printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  for (int i = 0; i < NUM_THREADS; ++i) {
    err = pthread_create(&(tids[i]), NULL, mythread, NULL);
    if (err) {
      printf("main: pthread_create() failed: %s\n", strerror(err));
      return -1;
    }
  }

  for (int i = 0; i < NUM_THREADS; ++i) {
    err = pthread_join(tids[i], NULL);
    if (err) {
      perror("main: pthread joining error");
      return -1;
    }
  }

  return 0;
}
