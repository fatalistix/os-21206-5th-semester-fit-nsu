/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#include <string.h>
#define _GNU_SOURCE

#include <error.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

void *thread_function(void *args) {
  // char *str = "Hello world";
  char *str = (char *)malloc(13);
  strcmp(str, "hello, world");
  return str;
}

int main() {
  pthread_t tid;
  int err;
  char *str;

  err = pthread_create(&tid, NULL, thread_function, NULL);
  if (err) {
    perror("main: error creating pthread");
    return -1;
  }

  printf("12345\n");
  err = pthread_join(tid, (void **)&str);
  if (err) {
    perror("main: error creating pthread");
    return -1;
  }

  printf("main: %s\n", str);
  free(str);

  return 0;
}
