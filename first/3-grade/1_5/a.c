/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>

int got_sigint = 0;

void sigint_handler(int arg) {
  printf("SIGINT HANDLER: got sigint (%d),  %d\n", SIGINT, arg);
  got_sigint = 1;
}

void *thread_foo_sigint(void *args) {
  // int err;
  signal(SIGINT, sigint_handler);
  if (errno) {
    perror("thread 1: error setting sigint handler");
    return NULL;
  }
  while (1) {
    printf("thread 1: waiting for SIGINT\n");
    if (got_sigint) {
      printf("thread 1: GOT SIGINT\n");
    }
    sleep(1);
  }
  return NULL;
}

void *thread_foo_sigquit(void *args) {
  int err;
  sigset_t set;
  int signal_code;

  err = sigemptyset(&set);
  if (err) {
    perror("thread 2: error setting empty set");
    return NULL;
  }

  err = sigaddset(&set, SIGQUIT);
  if (err) {
    perror("thread 2: error adding SIGQUIT to set");
    return NULL;
  }

  while (1) {
    printf("thread 2: waiting for SIGQUIT\n");
    err = sigwait(&set, &signal_code);
    if (err) {
      perror("thread 2: error during sigwait");
      return NULL;
    }

    if (signal_code == SIGQUIT) {
      printf("thread 2: got SIGQUIT\n");
    }
  }
}

int main() {
  pthread_t tid1;
  pthread_t tid2;
  int err;
  sigset_t mask;

  err = sigfillset(&mask);
  if (err) {
    perror("main: error filing sigset for thread 1");
    return -1;
  }

  err = pthread_sigmask(SIG_SETMASK, &mask, NULL);
  if (err) {
    perror("main: error setting mask for thread main");
    return -1;
  }

  err = pthread_create(&tid1, NULL, thread_foo_sigint, NULL);
  if (err) {
    perror("main: error creating thread 1");
    return -1;
  }

  err = pthread_create(&tid2, NULL, thread_foo_sigquit, NULL);
  if (err) {
    perror("main: error creating thread 2");
    return -1;
  }

  sleep(100);

  return 0;
}

// err = pthread_attr_init(&attr1);
//   if (err) {
//     perror("main: error attr init for thread 1");
//     return -1;
//   }
//
//   err = pthread_attr_setsigmask_np(&attr1, &mask1);
//   if (err) {
//     perror("main: error seting attr for thead 1");
//     return -1;
//   }
//
// int err;
//   sigset_t mask;
//   err = sigfillset(&mask);
//   if (err) {
//     perror("main: error filling set");
//     return NULL;
//   }
//
//   err = pthread_sigmask(SIG_SETMASK, &mask, NULL);
//   if (err) {
//     perror("main: setting mask");
//     return NULL;
//   }
