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

int global = 103;

void *mythread(void *arg) {
  printf("mythread [%d %d %d %lu]: Hello from mythread!\n", getpid(), getppid(),
         gettid(), pthread_self());
  int local = 100;
  static int local_static = 102; // value must be known in compile time
  const int kLocal = getpid();
  printf("mythread [%d %d %d %lu]: Printing threads pointers:\n[local: %p, "
         "local_static: %p, kLocal: %p, global: %p]\n",
         getpid(), getppid(), gettid(), pthread_self(), &local, &local_static,
         &kLocal, &global);
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
    pthread_join(tids[i], NULL);
  }

  return 0;
}
