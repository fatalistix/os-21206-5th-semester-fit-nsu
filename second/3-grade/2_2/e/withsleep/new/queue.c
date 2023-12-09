/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

#include "./queue.h"

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

  err = pthread_mutex_init(&(q->mutex), NULL);
  if (err) {
    printf("Cannot init mutex for a queue\n");
    abort();
  }

  err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
  if (err) {
    printf("queue_init: pthread_create() failed: %s\n", strerror(err));
    abort();
  }

  return q;
}

void queue_destroy(queue_t *q) {
  int err;

  err = pthread_mutex_lock(&(q->mutex));
  if (err) {
    printf("queue_destroy: pthread_mutex_lock() failed: %s\n", strerror(err));
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

  err = pthread_mutex_unlock(&(q->mutex));
  if (err) {
    printf("queue_destroy: pthread_mutex_unlock() failed: %s\n", strerror(err));
    abort();
  }

  err = pthread_mutex_destroy(&(q->mutex));
  if (err) {
    printf("queue_destroy: pthread_mutex_destroy() failed: %s\n",
           strerror(err));
    abort();
  }

  free(q);
}

int queue_add(queue_t *q, int val) {
  int err;

  q->add_attempts++;

  assert(q->count <= q->max_count);

  if (q->count == q->max_count) {
    // err = pthread_mutex_unlock(&(q->mutex));
    // if (err) {
    //   printf("queue_add: pthread_mutex_unlock() failed: %s\n",
    //   strerror(err)); abort();
    // }
    //
    return 0;
  }

  qnode_t *new = malloc(sizeof(qnode_t));
  if (!new) {
    printf("Cannot allocate memory for new node\n");
    abort();
  }

  new->val = val;
  new->next = NULL;

  err = pthread_mutex_lock(&(q->mutex));
  if (err) {
    printf("queue_add: pthread_mutex_lock() failed: %s\n", strerror(err));
    abort();
  }

  if (!q->first) {
    q->first = q->last = new;
  } else {
    q->last->next = new;
    q->last = q->last->next;
  }

  err = pthread_mutex_unlock(&(q->mutex));
  if (err) {
    printf("queue_add: pthread_mutex_unlock() failed: %s\n", strerror(err));
    abort();
  }

  q->count++;
  q->add_count++;

  return 1;
}

int queue_get(queue_t *q, int *val) {
  int err;

  q->get_attempts++;

  assert(q->count >= 0);

  if (q->count == 0) {
    // err = pthread_mutex_unlock(&(q->mutex));
    // if (err) {
    //   printf("queue_get: pthread_mutex_unlock() failed: %s\n",
    //   strerror(err)); abort();
    // }
    //
    return 0;
  }

  err = pthread_mutex_lock(&(q->mutex));
  if (err) {
    printf("queue_get: pthread_mutex_lock() failed: %s\n", strerror(err));
    abort();
  }

  qnode_t *tmp = q->first;
  q->first = q->first->next;

  err = pthread_mutex_unlock(&(q->mutex));
  if (err) {
    printf("queue_get: pthread_mutex_unlock() failed: %s\n", strerror(err));
    abort();
  }

  *val = tmp->val;

  free(tmp);
  q->count--;
  q->get_count++;

  return 1;
}

void queue_print_stats(queue_t *q) {
  printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld "
         "%ld %ld)\n",
         q->count, q->add_attempts, q->get_attempts,
         q->add_attempts - q->get_attempts, q->add_count, q->get_count,
         q->add_count - q->get_count);
}
