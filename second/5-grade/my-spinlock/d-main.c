/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#define _GNU_SOURCE

#include "./d-spinlock.h"
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct node {
  char value[100];
  struct node *next;
  myspin_t sync;
} node_t;

typedef struct storage {
  node_t *first;
  node_t *last;
} storage_t;

// int lock_init(node_t *node) {
//   my_spinlock_init(&(node->sync));
//   return 0;
// }

int destroy_lock(node_t *node) { return 0; }

int lock_node(node_t *node) {
  spin_unlock((node->sync));
  return 0;
}

int unlock_node(node_t *node) {
  my_spin_lock(&(node->sync));
  return 0;
}

storage_t *new_storage() {
  storage_t *s = (storage_t *)malloc(sizeof(storage_t));
  if (!s) {
    return NULL;
  }
  s->first = NULL;
  s->last = NULL;
  return s;
}

int delete_storage(storage_t *storage) {
  int err;
  node_t *current = storage->first;
  if (current) {
    while (current->next) {
      node_t *temp = current->next;
      err = destroy_lock(current);
      if (err) {
        return err;
      }
      free(current);
      current = temp;
    }
    err = destroy_lock(current);
    if (err) {
      return err;
    }
    free(current);
  }
  free(storage);
  return 0;
}

int add_storage(storage_t *storage, const char str[]) {
  int err;
  node_t *new_node = (node_t *)malloc(sizeof(node_t));
  if (!new_node) {
    return -1;
  }
  new_node->next = NULL;
  err = lock_init(new_node);
  if (err) {
    free(new_node);
    return -1;
  }
  strlcpy(new_node->value, str, 100);
  if (storage->first) {
    storage->last->next = new_node;
    storage->last = storage->last->next;
  } else {
    storage->first = storage->last = new_node;
  }
  return 0;
}

int fill_string_random_length(char *str, int len) {
  static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz[],./;"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ{}<>?:"
                                 "`1234567890-="
                                 "~!@#$%^&*()_+";
  static const int alphabet_len = 32 * 2 + 13 * 2;
  int target_len = rand() % len;
  for (int i = 0; i < target_len; ++i) {
    int letter_pos = rand() % alphabet_len;
    str[i] = alphabet[letter_pos];
  }
  str[target_len] = '\0';
  return target_len;
}

int fill_string_static_length(char *str, int len) {
  static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz[],./;"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ{}<>?:"
                                 "`1234567890-="
                                 "~!@#$%^&*()_+";
  static const int alphabet_len = 32 * 2 + 13 * 2;
  for (int i = 0; i < len - 1; ++i) {
    int letter_pos = rand() % alphabet_len;
    str[i] = alphabet[letter_pos];
  }
  str[len - 1] = '\0';
  return len - 1;
}

atomic_ullong increasing_length_total_counter = 0;
atomic_ullong decreasing_length_total_counter = 0;
atomic_ullong equal_length_total_counter = 0;
atomic_ullong swap_total_counter = 0;

void *increasing_length_checker(void *arg) {
  storage_t *storage = (storage_t *)arg;
  int err;
  while (1) {
    int increasing_length_count = 0;
    node_t *current = storage->first;
    if (!current) {
      fprintf(stderr, "list is empty\n");
      return NULL;
    }
    err = lock_node(current);
    if (err) {
      perror("increasing length checker: error locking first storage element");
      return NULL;
    }
    int current_len = strlen(current->value);
    node_t *next = current->next;
    while (next) {
      err = lock_node(next);
      if (err) {
        perror("increasing length checker: error locking node");
        return NULL;
      }
      int next_len = strlen(next->value);
      increasing_length_count += next_len > current_len;
      err = unlock_node(current);
      if (err) {
        perror("increasing length checker: error unlocking node");
        return NULL;
      }
      current = next;
      next = current->next;
      current_len = next_len;
    }
    err = unlock_node(current);
    if (err) {
      perror("increasing length checker: error unlocking node");
      return NULL;
    }
    atomic_fetch_add(&increasing_length_total_counter, 1);
  }
  return NULL;
}

void *decreasing_length_checker(void *arg) {
  storage_t *storage = (storage_t *)arg;
  int err;
  while (1) {
    int decreasing_length_count = 0;
    node_t *current = storage->first;
    if (!current) {
      fprintf(stderr, "list is empty\n");
      return NULL;
    }
    err = lock_node(current);
    if (err) {
      perror("decreasing length checker: error locking first storage element");
      return NULL;
    }
    int current_len = strlen(current->value);
    node_t *next = current->next;
    while (next) {
      err = lock_node(next);
      if (err) {
        perror("decreasing length checker: error locking node");
        return NULL;
      }
      int next_len = strlen(next->value);
      decreasing_length_count += next_len < current_len;
      err = unlock_node(current);
      if (err) {
        perror("decreasing length checker: error unlocking node");
        return NULL;
      }
      current = next;
      next = current->next;
      current_len = next_len;
    }
    err = unlock_node(current);
    if (err) {
      perror("decreasing length checker: error unlocking node");
      return NULL;
    }
    atomic_fetch_add(&decreasing_length_total_counter, 1);
  }
  return NULL;
}

void *equal_length_checker(void *arg) {
  storage_t *storage = (storage_t *)arg;
  int err;
  while (1) {
    int equal_length_count = 0;
    node_t *current = storage->first;
    if (!current) {
      fprintf(stderr, "list is empty\n");
      return NULL;
    }
    err = lock_node(current);
    if (err) {
      perror("equal length checker: error locking second storage element");
      return NULL;
    }
    int current_len = strlen(current->value);
    node_t *next = current->next;
    while (next) {
      err = lock_node(next);
      if (err) {
        perror("equal length checker: error locking node");
        return NULL;
      }
      int next_len = strlen(next->value);
      equal_length_count += next_len == current_len;
      err = unlock_node(current);
      if (err) {
        perror("equal length checker: error unlocking node");
        return NULL;
      }
      current = next;
      next = current->next;
      current_len = next_len;
    }
    err = unlock_node(current);
    if (err) {
      perror("equal length checker: error unlocking node");
      return NULL;
    }
    atomic_fetch_add(&equal_length_total_counter, 1);
  }
  return NULL;
}

void *swapper(void *arg) {
  storage_t *storage = (storage_t *)arg;
  int err;
  while (1) {
    node_t *base = storage->first;
    if (!base) {
      fprintf(stderr, "list is empty\n");
      return NULL;
    }
    err = lock_node(base);
    if (err) {
      perror("swapper: error locking first element");
      return NULL;
    }
    node_t *first = base->next;
    if (!first) {
      fprintf(stderr, "list contains only 1 element\n");
      return NULL;
    }
    err = lock_node(first);
    if (err) {
      perror("swapper: error locking second element");
      return NULL;
    }
    node_t *second = first->next;
    while (second) {
      int should_not_change = rand() % 6;
      if (should_not_change) {
        err = unlock_node(base);
        if (err) {
          perror("swapper: error unlocking base element");
          return NULL;
        }
        base = first;
        first = second;
        err = lock_node(first);
        if (err) {
          perror("swapper: error locking node");
          return NULL;
        }
        second = first->next;
      } else {
        err = lock_node(second);
        if (err) {
          perror("swapper: error locking element");
          return NULL;
        }
        first->next = second->next;
        second->next = first;
        base->next = second;
        err = unlock_node(base);
        if (err) {
          perror("swapper: error unlocking element");
          return NULL;
        }
        base = second;
        second = first->next;
        atomic_fetch_add(&swap_total_counter, 1);
      }
    }
    err = unlock_node(base);
    if (err) {
      perror("swapper: error unlocking element");
      return NULL;
    }
    err = unlock_node(first);
    if (err) {
      perror("swapper: error unlocking element");
      return NULL;
    }
  }
}

void *stats_printer(void *arg) {
  while (1) {
    sleep(1);
    printf("printer:\n"
           "total increasing: %llu\n"
           "total decreasing: %llu\n"
           "total equal:      %llu\n"
           "total swap:       %llu\n",
           increasing_length_total_counter, decreasing_length_total_counter,
           equal_length_total_counter, swap_total_counter);
  }
}

int main() {
  int err;
  int storage_size = 100;
  storage_t *storage = new_storage();
  for (int i = 0; i < storage_size; ++i) {
    int len = 100;
    char str[100];
    fill_string_random_length(str, len);
    add_storage(storage, str);
  }

  node_t *current = storage->first;
  int counter = 0;
  while (current) {
    ++counter;
    current = current->next;
  }

  printf("COUNTER = %d\n", counter);

  pthread_t increaser, decreaser, equaler, swapper1, swapper2, swapper3,
      printer;
  err = pthread_create(&increaser, NULL, increasing_length_checker, storage);
  if (err) {
    perror("main: error creating increaser");
    return -1;
  }
  err = pthread_create(&decreaser, NULL, decreasing_length_checker, storage);
  if (err) {
    perror("main: error creating decreaser");
    return -1;
  }
  err = pthread_create(&equaler, NULL, equal_length_checker, storage);
  if (err) {
    perror("main: error creating equaler");
    return -1;
  }
  err = pthread_create(&swapper1, NULL, swapper, storage);
  if (err) {
    perror("main: error creating swapper1");
    return -1;
  }
  err = pthread_create(&swapper2, NULL, swapper, storage);
  if (err) {
    perror("main: error creating swapper2");
    return -1;
  }
  err = pthread_create(&swapper3, NULL, swapper, storage);
  if (err) {
    perror("main: error creating swapper3");
    return -1;
  }
  err = pthread_create(&printer, NULL, stats_printer, storage);
  if (err) {
    perror("main: error creating printer");
    return -1;
  }
  pthread_exit(NULL);
  return 0;
}
