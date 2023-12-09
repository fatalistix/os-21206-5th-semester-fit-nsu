/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#include "./my-spinlock.h"

#include <stdatomic.h>

void my_spinlock_init(my_spinlock_t *spin) { spin->lock = 1; }

void my_spin_lock(my_spinlock_t *spin) {
  while (1) {
    int one = 1;
    if (atomic_compare_exchange_strong(&(spin->lock), &one, 0)) {
      break;
    }
    asm("pause");
  }
}

void my_spin_unlock(my_spinlock_t *spin) {
  int zero = 0;
  atomic_compare_exchange_strong(&(spin->lock), &zero, 1);
}
