/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#pragma once

#include <stdatomic.h>

typedef struct {
  atomic_int lock;
} my_spinlock_t;

void my_spinlock_init(my_spinlock_t *spin);

void my_spin_lock(my_spinlock_t *spin);

void my_spin_unlock(my_spinlock_t *spin);
