/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
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
  // q->count = 0;

  q->add_attempts = q->get_attempts = 0;
  q->add_count = q->get_count = 0;

  // err = sem_init(&q->semaphore, 0, 1);
  // if (err) {
  //   printf("queue_init: sem_init() failed: %s\n", strerror(err));
  //   abort();
  // }

  err = sem_init(&(q->sem_count), 0, max_count);
  if (err) {
    printf("queue_init: sem_init() failed: %s\n", strerror(err));
    abort();
  }

  err = sem_init(&(q->sem_first_modify), 0, 1);
  if (err) {
    printf("queue_init: sem_init() failed: %s\n", strerror(err));
    abort();
  }

  err = sem_init(&(q->sem_last_modify), 0, 1);
  if (err) {
    printf("queue_init: sem_init() failed: %s\n", strerror(err));
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

  // err = sem_wait(&(q->semaphore));
  // if (err) {
  //   printf("queue_destroy: sem_wait() failed: %s\n", strerror(err));
  //   abort();
  // }

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

  err = sem_destroy(&(q->sem_count));
  if (err) {
    printf("queue_destroy: sem_destroy() failed: %s\n", strerror(err));
    abort();
  }

  err = sem_destroy(&(q->sem_last_modify));
  if (err) {
    printf("queue_destroy: sem_destroy() failed: %s\n", strerror(err));
    abort();
  }

  err = sem_destroy(&(q->sem_first_modify));
  if (err) {
    printf("queue_destroy: sem_destroy() failed: %s\n", strerror(err));
    abort();
  }

  free(q);
}

int queue_add(queue_t *q, int val) {
  int err;

  // err = sem_wait(&(q->semaphore));
  // if (err) {
  //   printf("queue_add: sem_wait() failed: %s\n", strerror(err));
  //   abort();
  // }

  q->add_attempts++;

  // assert(q->count <= q->max_count);

  // if (q->count == q->max_count) {
  //   err = sem_post(&(q->semaphore));
  //   if (err) {
  //     printf("queue_add: sem_post() failed: %s\n", strerror(err));
  //     abort();
  //   }
  //
  //   return 0;
  // }

  int count;
  err = sem_getvalue(&(q->sem_count), &count);
  if (err) {
    printf("queue_add: sem_getvalue() failed: %s\n", strerror(err));
    abort();
  }

  if (count == q->max_count) {
    return 0;
  }

  qnode_t *new = malloc(sizeof(qnode_t));
  if (!new) {
    printf("Cannot allocate memory for new node\n");
    abort();
  }

  new->val = val;
  new->next = NULL;

  err = sem_wait(&(q->sem_first_modify));
  if (err) {
    printf("queue_add: sem_wait() failed: %s\n", strerror(err));
    abort();
  }

  if (!q->first) {
    err = sem_wait(&(q->sem_last_modify));
    if (err) {
      printf("queue_add: sem_wait() failed: %s\n", strerror(err));
      abort();
    }

    q->first = q->last = new;

    err = sem_post(&(q->sem_first_modify));
    if (err) {
      printf("queue_add: sem_wait() failed: %s\n", strerror(err));
      abort();
    }
  } else {
    err = sem_post(&(q->sem_first_modify));
    if (err) {
      printf("queue_add: sem_wait() failed: %s\n", strerror(err));
      abort();
    }

    err = sem_wait(&(q->sem_last_modify));
    if (err) {
      printf("queue_add: sem_wait() failed: %s\n", strerror(err));
      abort();
    }

    q->last->next = new;
    q->last = q->last->next;
  }

  // q->count++;
  q->add_count++;

  err = sem_post(&(q->sem_last_modify));
  if (err) {
    printf("queue_add: sem_post() failed: %s\n", strerror(err));
    abort();
  }

  err = sem_post(&(q->sem_count));
  if (err) {
    printf("queue_add: sem_post() failed: %s\n", strerror(err));
    abort();
  }

  return 1;
}

int queue_get(queue_t *q, int *val) {
  int err;

  // err = sem_wait(&(q->semaphore));
  // if (err) {
  //   printf("queue_get: sem_wait() failed: %s\n", strerror(err));
  //   abort();
  // }

  q->get_attempts++;

  // assert(q->count >= 0);

  // if (q->count == 0) {
  //   err = sem_post(&(q->semaphore));
  //   if (err) {
  //     printf("queue_get: sem_post() failed: %s\n", strerror(err));
  //     abort();
  //   }
  //
  //   return 0;
  // }
  //

  err = sem_wait(&(q->sem_count));
  if (err) {
    printf("queue_get: sem_wait() failed: %s\n", strerror(err));
    abort();
  }

  err = sem_wait(&(q->sem_first_modify));
  if (err) {
    printf("queue_get: sem_wait() failed: %s\n", strerror(err));
    abort();
  }

  qnode_t *tmp = q->first;
  q->first = q->first->next;

  err = sem_post(&(q->sem_first_modify));
  if (err) {
    printf("queue_get: sem_wait() failed: %s\n", strerror(err));
    abort();
  }

  *val = tmp->val;

  free(tmp);
  q->get_count++;

  // err = sem_post(&(q->semaphore));
  // if (err) {
  //   printf("queue_get: sem_post() failed: %s\n", strerror(err));
  //   abort();
  // }

  return 1;
}

void queue_print_stats(queue_t *q) {
  int count;
  int err = sem_getvalue(&(q->sem_count), &count);
  if (err) {
    printf("queue_add: sem_getvalue() failed: %s\n", strerror(err));
    abort();
  }
  printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld "
         "%ld %ld)\n",
         count, q->add_attempts, q->get_attempts,
         q->add_attempts - q->get_attempts, q->add_count, q->get_count,
         q->add_count - q->get_count);
}
