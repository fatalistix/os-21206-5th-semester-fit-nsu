/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE

#include "./my-core-threads.h"
#include <errno.h>
#include <linux/futex.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

uint32_t futex = 0;

void *routine1(void *args) {
  // printf("thread%d: HELLO, WORLD!!!\n", gettid());
  int i = 0;
  while (1) {
    printf("thread1: %d alive\n", i++);
    sleep(1);
  }
  //     (* (int*) args)++;
  return args;
}

void *routine2(void *args) {
  printf("thread2: Hello\n");
  *(int *)args = 15;
  return args;
}

void *routine(void *args) {
  sleep(5);
  futex = 1;
  int err = syscall(SYS_futex, &futex, FUTEX_WAKE, 2, NULL, NULL, 0);
  printf("thread1: %d\n", err);
  // printf("thread1: finished\n");
  return NULL;
}

void *routine_waiter(void *args) {
  int err = syscall(SYS_futex, &futex, FUTEX_WAIT, 0, NULL, NULL, 0);
  printf("thread2: %d\n", err);
  return NULL;
}

int main() {
  mycorethread_t t1;
  int err;

  int x = 42;
  int *x_ptr = &x;

  int i = 0;
  // while (1) {
  printf("main: %d\n", i++);
  err = mycorethread_create(&t1, routine1, x_ptr);
  if (err) {
    perror("main: error creating thread");
    return -1;
  }
  // err = mycorethread_detach(t1);
  // if (err) {
  //     perror("main: error detaching thread");
  //     return -1;
  // }
  // sleep(1);
  // }

  sleep(5);

  //  err = mycorethread_join(t1, &x_ptr);
  //  if (err) {
  //    perror("main: error creating thread");
  //    return -1;
  //  }

  err = mycorethread_cancel(t1);
  if (err) {
    perror("main");
    return -1;
  }
  printf("main: here\n");

  sleep(3);

  //  printf("main: %d %d\n", *x_ptr, x);

  // pthread_t thread;
  // pthread_t thread_waiter;
  // pthread_create(&thread, NULL, routine, NULL);
  // pthread_create(&thread_waiter, NULL, routine_waiter, NULL);
  // futex = 1;
  // int err = syscall(SYS_futex, &futex, FUTEX_WAIT, 0, NULL, NULL, 0);
  // while (err == -1 && errno == EAGAIN) {
  // err = syscall(SYS_futex, FUTEX_WAIT, 0, NULL, NULL, 0);
  // }
  // printf("main: %d %d - I AM HERE\n", err, errno);
  // sleep(1);
  // printf("main: futex = %d\n", futex);
  // printf("main: pid = %d\n", getpid());
  // mycorethread_t thread1;
  // mycorethread_t thread2;
  // printf("main: thread struct created\n");
  // int err;
  // err = mycorethread_create(&thread1, routine1, NULL);
  // if (err) {
  //   perror("main: error creating thread1");
  //   return -1;
  // }
  //
  // int value = 30;
  // printf("main: value = %d\n", value);
  // printf("main: create second thread...\n");
  // err = mycorethread_create(&thread2, routine2, &value);
  // if (err) {
  //   perror("main: error creating thread2");
  //   return -1;
  // }
  //
  // sleep(2);
  //
  // printf("main: value after modification = %d\n", value);
  //
  return 0;
}
