/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>

void sigint_handler(int arg) {
  printf("SIGINT HANDLER: got sigint (%d),  %d\n", SIGINT, arg);
}

void *thread_foo_sigint(void *args) {
  // int err;
  signal(SIGINT, sigint_handler);
  if (errno) {
    perror("thread 1: error setting sigint handler");
    return NULL;
  }

  printf("thread 1: waiting for SIGINT\n");
  while (1) {
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

  printf("thread 2: waiting for SIGQUIT\n");
  while (1) {
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

void *thread_foo_immortal(void *args) {
  int err;
  sigset_t mask;

  err = sigfillset(&mask);
  if (err) {
    perror("main: error filing sigset for thread 1");
    return NULL;
  }

  err = pthread_sigmask(SIG_SETMASK, &mask, NULL);
  if (err) {
    perror("main: error setting mask for thread main");
    return NULL;
  }

  sleep(7);
  printf("thread 3: I AM ALIVE\n");
  return NULL;
}

int main() {
  pthread_t tid1;
  pthread_t tid2;
  pthread_t tid3;
  int err;

  err = pthread_create(&tid1, NULL, thread_foo_sigint, NULL);
  if (err) {
    perror("main: error creating thread 1");
    return -1;
  }
  printf("main: thread sigint started\n");

  err = pthread_create(&tid2, NULL, thread_foo_sigquit, NULL);
  if (err) {
    perror("main: error creating thread 2");
    return -1;
  }
  printf("main: thread sigquit started\n");

  err = pthread_create(&tid3, NULL, thread_foo_immortal, NULL);
  if (err) {
    perror("main: error creating thread 2");
    return -1;
  }
  printf("main: thread immortal started\n");

  sleep(15);

  err = pthread_kill(tid1, SIGINT);
  if (err) {
    perror("main: error killing thread");
    return -1;
  }
  printf("main: sigint to siginter sent\n");

  err = pthread_kill(tid2, SIGQUIT);
  if (err) {
    perror("main: error killing thread");
    return -1;
  }
  printf("main: sigquit to sigquiter sent\n");

  err = pthread_kill(tid3, SIGSEGV);
  if (err) {
    perror("main: error killing thread");
    return -1;
  }
  printf("main: sigsegv to immortal sent\n");

  sleep(5);

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
