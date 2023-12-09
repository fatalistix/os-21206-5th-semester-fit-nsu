#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

#include "./queue.h"

_Bool FALSE_MOCK = 1;
_Bool TRUE_MOCK = 2;

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

  printf("MOCKS = %d %d\n", FALSE_MOCK, TRUE_MOCK);
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
  q->is_modifying = FALSE_MOCK;

  err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
  if (err) {
    printf("queue_init: pthread_create() failed: %s\n", strerror(err));
    abort();
  }

  return q;
}

void queue_destroy(queue_t *q) {
  while (!atomic_compare_exchange_strong(&(q->is_modifying), &FALSE_MOCK,
                                         TRUE_MOCK)) {
    asm("pause");
  }
  int err;

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

  free(q);
}

static _Bool temp_add;

int queue_add(queue_t *q, int val) {
  temp_add = q->is_modifying;
  while (!atomic_compare_exchange_strong(&(q->is_modifying), &FALSE_MOCK,
                                         TRUE_MOCK)) {
    asm("pause");
  }
  // printf("ADD LOCKED: old modif = %d\n", temp_add);

  q->add_attempts++;

  assert(q->count <= q->max_count);

  if (q->count == q->max_count) {
    // atomic_store(&(q->is_modifying), FALSE_MOCK);
    q->is_modifying = 0;
    // printf("VALUE OF IS MODIFYING after add = %d\n", q->is_modifying);
    return 0;
  }

  qnode_t *new = malloc(sizeof(qnode_t));
  if (!new) {
    // printf("Cannot allocate memory for new node\n");
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

  // atomic_store(&(q->is_modifying), FALSE_MOCK);

  q->is_modifying = 0;
  // printf("VALUE OF IS MODIFYING after add = %d\n", q->is_modifying);
  return 1;
}

static _Bool temp_get;

int queue_get(queue_t *q, int *val) {
  temp_get = q->is_modifying;
  while (!atomic_compare_exchange_strong(&(q->is_modifying), &FALSE_MOCK,
                                         TRUE_MOCK)) {
    asm("pause");
  }
  // printf("GET LOCKET: old modif = %d, curr = %d\n", temp_get,
  // q->is_modifying);
  q->get_attempts++;

  assert(q->count >= 0);

  if (q->count == 0) {
    // atomic_store(&(q->is_modifying), FALSE_MOCK);
    // printf("VALUE OF IS MODIFYING after get = %d\n", q->is_modifying);
    q->is_modifying = 0;
    // printf("VALUE OF IS MODIFYING after get = %d\n", q->is_modifying);
    return 0;
  }

  qnode_t *tmp = q->first;

  *val = tmp->val;
  q->first = q->first->next;

  free(tmp);
  q->count--;
  q->get_count++;

  // atomic_store(&(q->is_modifying), FALSE_MOCK);
  // printf("VALUE OF IS MODIFYING after get = %d\n", q->is_modifying);
  q->is_modifying = 0;
  // printf("VALUE OF IS MODIFYING after get = %d\n", q->is_modifying);

  return 1;
}

void queue_print_stats(queue_t *q) {
  printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld "
         "%ld %ld)\n",
         q->count, q->add_attempts, q->get_attempts,
         q->add_attempts - q->get_attempts, q->add_count, q->get_count,
         q->add_count - q->get_count);
}
