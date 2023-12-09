/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sched.h>

#define RED "\033[41m"
#define NOCOLOR "\033[0m"

#include <assert.h>

typedef struct _QueueNode {
  int val;
  struct _QueueNode *next;
} qnode_t;

typedef struct _Queue {
  qnode_t *first;
  qnode_t *last;

  pthread_spinlock_t spinlock;

  pthread_t qmonitor_tid;

  int count;
  int max_count;

  // queue statistics
  long add_attempts;
  long get_attempts;
  long add_count;
  long get_count;
} queue_t;

queue_t *queue_init(int max_count);
void queue_destroy(queue_t *q);
int queue_add(queue_t *q, int val);
int queue_get(queue_t *q, int *val);
void queue_print_stats(queue_t *q);

// static _Bool FALSE_MOCK = 0;
// static _Bool TRUE_MOCK = 1;

void *qmonitor(void *arg) {
  queue_t *q = (queue_t *)arg;

  printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

  while (1) {
    queue_print_stats(q);
    sleep(1);
  }

  return NULL;
}

queue_t *queue_init(int max_count) {
  int err;

  queue_t *q = malloc(sizeof(queue_t));
  if (!q) {
    printf("Cannot allocate memory for a queue\n");
    abort();
  }

  q->first = NULL;
  q->last = NULL;
  q->max_count = max_count;
  q->count = 0;

  q->add_attempts = q->get_attempts = 0;
  q->add_count = q->get_count = 0;
  err = pthread_spin_init(&(q->spinlock), PTHREAD_PROCESS_PRIVATE);
  if (err) {
    printf("queue_init: pthread_spin_init() failed: %s\n", strerror(err));
    abort();
  }

  err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
  if (err) {
    pthread_spin_destroy(&q->spinlock);
    printf("queue_init: pthread_create() failed: %s\n", strerror(err));
    abort();
  }

  return q;
}

void queue_destroy(queue_t *q) {
  int err;
  err = pthread_spin_lock(&q->spinlock);
  if (err) {
    printf("queue_destroy: pthread_spin_lock() failed: %s\n", strerror(err));
    abort();
  }

  err = pthread_cancel(q->qmonitor_tid);
  if (err) {
    printf("queue_destroy: pthread_cancel() failed: %s\n", strerror(err));
    abort();
  }

  if (q->first) {
    qnode_t *temp;
    while (q->first) {
      temp = q->first;
      q->first = q->first->next;
      free(temp);
    }
  }

  err = pthread_spin_unlock(&(q->spinlock));
  if (err) {
    printf("queue_destroy: pthread_spin_unlock() failed: %s\n", strerror(err));
    abort();
  }

  err = pthread_spin_destroy(&(q->spinlock));
  if (err) {
    printf(": pthread_spin_destroy() failed: %s\n", strerror(err));
    abort();
  }

  free(q);
}

int queue_add(queue_t *q, int val) {
  //  while (!atomic_compare_exchange_strong(&(q->is_modifying), &TRUE_MOCK,
  //                                         FALSE_MOCK)) {
  //  }
  //    printf("ADD IS WORKING %d!!!\n", val);
  int err;
  err = pthread_spin_lock(&q->spinlock);
  if (err) {
    printf("queue_get: pthread_spin_lock() failed: %s\n", strerror(err));
    abort();
  }
  q->add_attempts++;

  assert(q->count <= q->max_count);

  if (q->count == q->max_count) {
    err = pthread_spin_unlock(&q->spinlock);
    if (err) {
      printf("queue_add: pthread_spin_unlock() faild: %s\n", strerror(err));
      abort();
    }
    return 0;
  }

  qnode_t *new = malloc(sizeof(qnode_t));
  if (!new) {
    printf("Cannot allocate memory for new node\n");
    abort();
  }

  new->val = val;
  new->next = NULL;

  if (!q->first) {
    q->first = q->last = new;
  } else {
    q->last->next = new;
    q->last = q->last->next;
  }

  q->count++;
  q->add_count++;

  //  q->is_modifying = FALSE_MOCK;
  err = pthread_spin_unlock(&q->spinlock);
  if (err) {
    printf("queue_add: pthread_spin_unlock() faild: %s\n", strerror(err));
    abort();
  }
  return 1;
}

int queue_get(queue_t *q, int *val) {
  //  while (!atomic_compare_exchange_strong(&(q->is_modifying), &FALSE_MOCK,
  //                                         TRUE_MOCK)) {
  //  }
  int err;
  err = pthread_spin_lock(&q->spinlock);
  if (err) {
    printf("queue_get: pthread_spin_lock() failed: %s\n", strerror(err));
    abort();
  }
  q->get_attempts++;

  assert(q->count >= 0);

  if (q->count == 0) {
    err = pthread_spin_unlock(&q->spinlock);
    if (err) {
      printf("queue_get: pthread_spin_unlock() faild: %s\n", strerror(err));
      abort();
    }
    return 0;
  }

  qnode_t *tmp = q->first;

  *val = tmp->val;
  q->first = q->first->next;

  free(tmp);
  q->count--;
  q->get_count++;

  //  q->is_modifying = FALSE_MOCK;
  err = pthread_spin_unlock(&q->spinlock);
  if (err) {
    printf("queue_get: pthread_spin_unlock() faild: %s\n", strerror(err));
    abort();
  }
  return 1;
}

void queue_print_stats(queue_t *q) {
  printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld "
         "%ld %ld)\n",
         q->count, q->add_attempts, q->get_attempts,
         q->add_attempts - q->get_attempts, q->add_count, q->get_count,
         q->add_count - q->get_count);
}

void set_cpu(int n) {
  int err;
  cpu_set_t cpuset;
  pthread_t tid = pthread_self();

  CPU_ZERO(&cpuset);
  CPU_SET(n, &cpuset);

  err = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
  if (err) {
    printf("set_cpu: pthread_setaffinity failed for cpu %d\n", n);
    return;
  }

  printf("set_cpu: set cpu %d\n", n);
}

void *reader(void *arg) {
  int expected = 0;
  queue_t *q = (queue_t *)arg;
  printf("reader [%d %d %d]\n", getpid(), getppid(), gettid());

  set_cpu(1);

  while (1) {
    int val = -1;
    int ok = queue_get(q, &val);
    if (!ok)
      continue;

    if (expected != val)
      printf(RED "ERROR: get value is %d but expected - %d" NOCOLOR "\n", val,
             expected);

    expected = val + 1;
  }

  return NULL;
}

void *writer(void *arg) {
  int i = 0;
  queue_t *q = (queue_t *)arg;
  printf("writer [%d %d %d]\n", getpid(), getppid(), gettid());

  set_cpu(2);

  while (1) {
    int ok = queue_add(q, i);
    if (!ok)
      continue;
    i++;
  }

  return NULL;
}

int main() {
  pthread_t tid;
  queue_t *q;
  int err;

  printf("main [%d %d %d]\n", getpid(), getppid(), gettid());

  q = queue_init(1000000);

  err = pthread_create(&tid, NULL, reader, q);
  if (err) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
    return -1;
  }

  // sched_yield();

  err = pthread_create(&tid, NULL, writer, q);
  if (err) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
    return -1;
  }

  // TODO: join threads

  pthread_exit(NULL);

  return 0;
}
