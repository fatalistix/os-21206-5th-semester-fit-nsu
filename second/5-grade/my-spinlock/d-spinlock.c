
#include "./d-spinlock.h"

myspin_t init_spin() {
  myspin_t lock = (myspin_t)malloc(sizeof(myspin) * 1);
  lock->is_locked = FALSE;
  return lock;
}

int spin_lock(myspin_t lock) {
  int compare = FREE;
  int desired = LOCKED;
  while (!__atomic_compare_exchange_n(&(lock->is_locked), &compare, desired,
                                      FALSE, __ATOMIC_SEQ_CST,
                                      __ATOMIC_SEQ_CST)) {
  }
  return 1;
}

int spin_unlock(myspin_t lock) {
  __atomic_store_n(&(lock->is_locked), FREE, __ATOMIC_SEQ_CST);
  return 1;
}

void destroy_spin(myspin_t lock) {
  spin_lock(lock);
  free(lock);
}
