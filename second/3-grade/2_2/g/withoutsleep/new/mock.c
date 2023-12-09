#define _GNU_SOURCE
#include <errno.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

typedef struct _QueueNode {
    int val;
    struct _QueueNode *next;
} qnode_t;

typedef struct _Queue {
    qnode_t *first;
    qnode_t *last;

    sem_t sem_count;
    sem_t sem_first_modify;
    sem_t sem_last_modify;

    pthread_t qmonitor_tid;

    // atomic_int count;
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

/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

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

    err = sem_wait(&(q->sem_count));
    if (err) {
        printf("queue_get: sem_wait() failed: %s\n", strerror(err));
        abort();
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
//        err = sem_wait(&(q->sem_last_modify));
//        if (err) {
//            printf("queue_add: sem_wait() failed: %s\n", strerror(err));
//            abort();
//        }

        q->first = q->last = new;

//        err = sem_post(&(q->sem_first_modify));
//        if (err) {
//            printf("queue_add: sem_wait() failed: %s\n", strerror(err));
//            abort();
//        }
    } else {
//        err = sem_post(&(q->sem_first_modify));
//        if (err) {
//            printf("queue_add: sem_wait() failed: %s\n", strerror(err));
//            abort();
//        }

//        err = sem_wait(&(q->sem_last_modify));
//        if (err) {
//            printf("queue_add: sem_wait() failed: %s\n", strerror(err));
//            abort();
//        }

        q->last->next = new;
        q->last = q->last->next;
    }

    // q->count++;
    q->add_count++;

    err = sem_post(&(q->sem_first_modify));
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

    int count;
    err = sem_getvalue(&(q->sem_count), &count);
    if (err) {
        printf("queue_add: sem_getvalue() failed: %s\n", strerror(err));
        abort();
    }

    if (count == q->max_count) {
        return 0;
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

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <sched.h>


#define RED "\033[41m"
#define NOCOLOR "\033[0m"

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
        usleep(1);
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
