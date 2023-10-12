/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct {
  int a;
  char *b;
} some_struct;

void *thread_foo(void *args) {
  some_struct *s = (some_struct *)args;
  printf("thread: some_struct.a: %d; some_struct.b: %s\n", s->a, s->b);
  return NULL;
}

int main() {
  printf("main: program started\n");

  some_struct s;
  s.a = 10;
  s.b = "HELLO, WORLD!!!\n";

  printf("main: struct initialized with %d, %s\n", s.a, s.b);

  pthread_t tid;
  int err;

  err = pthread_create(&tid, NULL, thread_foo, &s);
  if (err) {
    perror("main: error during thread creation");
    return -1;
  }

  printf("main: thread created\n");

  err = pthread_join(tid, NULL);
  if (err) {
    perror("main: error joining thread");
    return -1;
  }

  printf("main: program finished\n");

  return 0;
}
