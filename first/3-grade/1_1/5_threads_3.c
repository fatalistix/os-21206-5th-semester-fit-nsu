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

#define NUM_THREADS 5

int global = 1;

void *mythread(void *arg) {
  printf("mythread [%d %d %d %lu]: Hello from mythread!\n", getpid(), getppid(),
         gettid(), pthread_self());
  int local = 100;
  // static int local_static = 102; // value must be known in compile time
  sleep(global += 3);
  // const int kLocal = getpid();
  printf("mythread [%d %d %d %lu]: Printing threads old values:\n[local: %d, "
         "global: %d]\n",
         getpid(), getppid(), gettid(), pthread_self(), local, global);
  ++global;
  ++local;
  printf("mythread [%d %d %d %lu]: Printing threads new values:\n[local: %d, "
         "global: %d]\n",
         getpid(), getppid(), gettid(), pthread_self(), local, global);

  sleep(20);
  return NULL;
}

int main() {
  pthread_t tids[NUM_THREADS];
  int err;

  printf("main [%d %d %d %lu]: Hello from main!\n", getpid(), getppid(),
         gettid(), pthread_self());

  for (int i = 0; i < NUM_THREADS; ++i) {
    err = pthread_create(&(tids[i]), NULL, mythread, NULL);
    if (err) {
      printf("main: pthread_create() failed: %s\n", strerror(err));
      return -1;
    }
    printf("main [%d %d %d %lu]: Created thread [%lu]!\n", getpid(), getppid(),
           gettid(), pthread_self(), tids[i]);
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
