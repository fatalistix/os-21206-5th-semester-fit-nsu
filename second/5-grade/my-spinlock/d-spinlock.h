#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

#define FREE 0
#define LOCKED 1

typedef struct myspin {
  int is_locked;
} myspin;

typedef myspin *myspin_t;

myspin_t init_spin();

void destroy_spin(myspin_t lock);

int spin_lock(myspin_t lock);

int spin_unlock(myspin_t lock);
