/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

/*
 * IMPORTS
 */
#include "./my-coroutine.h"
#include <errno.h>
#include <linux/futex.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

/*
 * CONSTS
 */
static const unsigned long int STACK_SIZE = 2 * 1024 * 1024;
static const time_t STARTUP_DELAY_TIMER_SEC = 0;
static const time_t RUNTIME_DELAY_TIMER_SEC = 0;
static const suseconds_t STARTUP_DELAY_TIMER_USEC = 50000;
static const suseconds_t RUNTIME_DELAY_TIMER_USEC = 50000;
static const uint32_t FUTEX_RUNNING = 0;
static const uint32_t FUTEX_FINISHED = 1;

/*
 * FUTEX WRAPPERS
 */
static long futex(uint32_t *uaddr, int futex_op, uint32_t val,
                  const struct timespec *timeout, uint32_t *uaddr2,
                  uint32_t val3) {
  return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

static long futex_wait(uint32_t *futx, uint32_t wait_value) {
  return futex(futx, FUTEX_WAIT, wait_value, NULL, NULL, 0);
}

static long futex_wake(uint32_t *futx, uint32_t num_to_wake) {
  return futex(futx, FUTEX_WAKE, num_to_wake, NULL, NULL, 0);
}

/*
 * COROUTINE STRUCT
 */
typedef struct {
  void *arg;
  void *result;
  void *(*routine)(void *);
  atomic_bool joinable;
  atomic_bool join_wait;
  atomic_bool canceled;
  ucontext_t context;
  void *stack;
  uint32_t join_futex;
} mycoroutine_real_t;

/*
 * DEQUE STRUCT
 */

typedef struct deque_element {
  struct deque_element *next;
  struct deque_element *prev;
  mycoroutine_real_t *coroutine;
} deque_element_t;

/*
 * CYCLE DEQUE HEAD STRUCT
 */

typedef struct {
  deque_element_t *current;
  uint32_t size;
} cycle_deque_t;

/*
 * CYCLE DEQUE FUNCTIONS
 */

static void cycle_deque_init(cycle_deque_t *d) {
  d->current = NULL;
  uint32_t size = 0;
}

static void cycle_deque_add(cycle_deque_t *d, void *memory,
                            mycoroutine_real_t *coroutine) {
  if (d->current == NULL) {
    d->current = memory;
    d->current->next = d->current;
    d->current->prev = d->current;
    d->current->coroutine = coroutine;
  } else {
    deque_element_t *new_element = memory;
    new_element->coroutine = coroutine;
    new_element->next = d->current;
    new_element->prev = d->current->prev;
    d->current->prev->next = new_element;
    d->current->prev = new_element;
  }
  ++d->size;
}

static void cycle_deque_delete(cycle_deque_t *d) {
  if (d == NULL) {
    return;
  }
  if (d->size == 1) {
    d->size = 0;
    free(d->current->coroutine->stack);
    d->current = NULL;
    return;
  }
  d->current->prev->next = d->current->next;
  d->current->next->prev = d->current->prev;
  free(d->current->coroutine->stack);
  d->current = d->current->next;
  --(d->size);
}

static void cycle_deque_next(cycle_deque_t *d) {
  d->current = d->current->next;
}

static mycoroutine_real_t *cycle_deque_get(cycle_deque_t *d) {
  return d->current->coroutine;
}

static void cycle_deque_destroy(cycle_deque_t *d) {
  while (d->size) {
    cycle_deque_delete(d);
  }
}

/*
 * CYCLE DEQUE SIGNLE GLOBAL INSTANCE
 */
static cycle_deque_t coroutines_cycle_deque;

/*
 * SWAP CONTEXT FUNCTION
 */
static int switch_context() {
  mycoroutine_real_t *current = cycle_deque_get(&coroutines_cycle_deque);
  cycle_deque_next(&coroutines_cycle_deque);
  mycoroutine_real_t *next = cycle_deque_get(&coroutines_cycle_deque);
  while (next->canceled || next->join_wait) {
    printf("%d - canceled, %p - arg\n", next->canceled, next->arg);
    if (next->canceled) {
      cycle_deque_delete(&coroutines_cycle_deque);
      next = cycle_deque_get(&coroutines_cycle_deque);
      printf("size = %d\n", coroutines_cycle_deque.size);
    } else {
      cycle_deque_next(&coroutines_cycle_deque);
      next = cycle_deque_get(&coroutines_cycle_deque);
    }
  }

  int err;
  err = swapcontext(&(current->context), &(next->context));
  if (err) {
    return -1;
  }
  return 0;
}

/*
 * ALARM SIGNAL HANDLER
 */
static void sigalarm_handler(int signal, siginfo_t *info, void *context) {
  switch_context();
}

/*
 * SCHEDULER INITIALIZATION
 */
int scheduler_init(void (*main_routine)(int argc, char *argv[]), int argc,
                   char *argv[]) {
  int err;
  struct sigaction action;
  err = sigemptyset(&(action.sa_mask));
  if (err) {
    return -1;
  }
  action.sa_sigaction = sigalarm_handler;
  action.sa_flags = SA_SIGINFO;
  err = sigaction(SIGALRM, &action, NULL);
  if (err) {
    return -1;
  }
  struct itimerval timer;
  timer.it_value.tv_sec = STARTUP_DELAY_TIMER_SEC;
  timer.it_value.tv_usec = STARTUP_DELAY_TIMER_USEC;
  timer.it_interval.tv_sec = RUNTIME_DELAY_TIMER_SEC;
  timer.it_interval.tv_usec = RUNTIME_DELAY_TIMER_USEC;
  err = setitimer(ITIMER_REAL, &timer, NULL);
  if (err) {
    return -1;
  }
  cycle_deque_init(&coroutines_cycle_deque);

  mycoroutine_real_t coroutine_mock;
  coroutine_mock.joinable = 0;
  coroutine_mock.join_wait = 0;
  coroutine_mock.canceled = 0;
  deque_element_t deque_element_mock;
  deque_element_mock.next = &deque_element_mock;
  deque_element_mock.prev = &deque_element_mock;
  cycle_deque_add(&coroutines_cycle_deque, &deque_element_mock,
                  &coroutine_mock);

  main_routine(argc, argv);
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &timer, NULL);
  cycle_deque_destroy(&coroutines_cycle_deque);

  return 0;
}

/*
 * COROUTINE's WRAPPER FUNCTION
 */
void wrapper(void *arg) {
  mycoroutine_real_t *c = arg;
  c->result = c->routine(c->arg);
  if (c->joinable) {
    c->join_wait = 1;
    c->join_futex = FUTEX_FINISHED;
    futex_wake(&(c->join_futex), 1);
    switch_context();
  } else {
    c->canceled = 1;
    switch_context();
  }
}

/*
 * COROUTINE CREATION FUNCTION
 */
int mycoroutine_create(mycoroutine_t *coroutine, void *(*routine)(void *),
                       void *args) {
  void *stack = calloc(STACK_SIZE, 1);
  if (stack == NULL) {
    return -1;
  }

  mycoroutine_real_t *coroutine_data =
      stack + STACK_SIZE - sizeof(mycoroutine_real_t);

  coroutine_data->routine = routine;
  coroutine_data->arg = args;
  coroutine_data->canceled = 0;
  coroutine_data->joinable = 1;

  int err = getcontext(&(coroutine_data->context));
  if (err) {
    free(stack);
    return -1;
  }

  coroutine_data->context.uc_stack.ss_sp = stack;
  coroutine_data->context.uc_stack.ss_size =
      STACK_SIZE - sizeof(mycoroutine_real_t) - sizeof(deque_element_t);

  makecontext(&(coroutine_data->context), (void (*)())wrapper, 1,
              coroutine_data);
  *coroutine = (mycoroutine_t)coroutine_data;
  cycle_deque_add(&coroutines_cycle_deque,
                  stack + STACK_SIZE - sizeof(mycoroutine_real_t) -
                      sizeof(deque_element_t),
                  coroutine_data);
  return 0;
}

int mycoroutine_join(mycoroutine_t coroutine, void **result) {
  mycoroutine_real_t *c = (mycoroutine_real_t *)coroutine;
  if (c->joinable && !c->canceled) {
    int err;
    err = futex_wait(&(c->join_futex), FUTEX_RUNNING);

    if (err == -1 && errno != EAGAIN) {
      c->canceled = 1;
      return err;
    }
    if (result) {
      *result = c->result;
    }
    c->canceled = 1;
    return 0;
  } else {
    errno = EINVAL;
    return -1;
  }
}

int mycoroutine_detach(mycoroutine_t coroutine) {
  mycoroutine_real_t *c = (mycoroutine_real_t *)coroutine;
  if (c->canceled) {
    errno = EINVAL;
    return -1;
  }
  c->joinable = 0;
  if (c->join_wait) {
    c->canceled = 1;
  }
  return 0;
}

int mycoroutine_cancel(mycoroutine_t coroutine) {
  mycoroutine_real_t *c = (mycoroutine_real_t *)coroutine;
  printf("I CANCELED thread %p, canceled = %d\n", c->arg, c->canceled);
  c->canceled = 1;
  return 0;
}
