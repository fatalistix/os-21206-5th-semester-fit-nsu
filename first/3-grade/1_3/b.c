/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */
#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
  int a;
  char *b;
} some_struct;

// depends on situation
void *thread_foo_main(void *args) {
  some_struct *s = (some_struct *)args;
  printf("thread: some_struct.a: %d; some_struct.b: %s\n", s->a, s->b);
  return NULL;
}

some_struct global_s;

void *thread_foo_global(void *args) {
  printf("thread: global_s.a: %d; global_s.b: %s\n", global_s.a, global_s.b);
  return NULL;
}

void *thread_foo_heap(void *args) {
  some_struct *s = (some_struct *)args;
  printf("thread: heap_s.a: %d; heap_s.b: %s\n", s->a, s->b);
  return NULL;
}

void *thread_foo_static(void *args) {
  some_struct *s = (some_struct *)args;
  printf("thread: static_s.a: %d; static_s.b: %s\n", s->a, s->b);
  return NULL;
}

int main() {
  printf("main: program started\n");

  some_struct s;
  s.a = 10;
  s.b = "HELLO, WORLD!!!\n";

  global_s.a = 11;
  global_s.b = "Helo, world\n";

  static some_struct ss;
  ss.a = 12;
  ss.b = "HElo static\n";

  some_struct *hs = (some_struct *)malloc(sizeof(some_struct) * 1);
  hs->a = 13;
  hs->b = "hhhhhhhhhhhlll\n";

  printf("main: struct stack  initialized with %d, %s\n", s.a, s.b);
  printf("main: struct global initialized with %d, %s\n", global_s.a,
         global_s.b);
  printf("main: struct static initialized with %d, %s\n", ss.a, ss.b);
  printf("main: struct heap   initialized with %d, %s\n", hs->a, hs->b);

  pthread_t tid;
  int err;
  pthread_attr_t attr;

  err = pthread_attr_init(&attr);
  if (err) {
    perror("main: error init attr");
    return -1;
  }

  err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (err) {
    perror("main: error seting detach attr");
    return -1;
  }

  printf("main: attr set\n");

  err = pthread_create(&tid, NULL, thread_foo_main, &s);
  if (err) {
    perror("main: error during thread creation");
    return -1;
  }

  printf("main: thread created\n");

  sleep(2);

  err = pthread_create(&tid, NULL, thread_foo_global, NULL);
  if (err) {
    perror("main: error during thread creation");
    return -1;
  }

  printf("main: thread created\n");

  sleep(2);

  err = pthread_create(&tid, NULL, thread_foo_static, &ss);
  if (err) {
    perror("main: error during thread creation");
    return -1;
  }

  printf("main: thread created\n");

  sleep(2);

  err = pthread_create(&tid, NULL, thread_foo_heap, hs);
  if (err) {
    perror("main: error during thread creation");
    return -1;
  }

  printf("main: thread created\n");

  sleep(2);

  free(hs);
  printf("main: program finished\n");

  return 0;
}
