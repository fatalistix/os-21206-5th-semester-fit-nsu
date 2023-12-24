/*
 * Copyright (c) 2023, Vyacheslav Balashov
 */

#pragma once

#include <stdint.h>

typedef uint32_t my_mutex_t;

int my_mutex_init(my_mutex_t *m);

int my_mutex_lock(my_mutex_t *m);

int my_mutex_unlock(my_mutex_t *m);
