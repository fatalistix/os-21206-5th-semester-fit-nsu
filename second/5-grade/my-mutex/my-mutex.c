/*
 * Copyright (c) 2023, Vyaheslav Balashov
 */

#include "./my-mutex.h"

#include <atomic_ops.h>
#include <errno.h>
#include <linux/futex.h>
#include <stdatomic.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

static long futex(uint32_t *uaddr, int futex_op, uint32_t val,
                  const struct timespec *timeout, uint32_t *uaddr2,
                  uint32_t val3) {
  return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

static long futex_wait(uint32_t *futx, uint32_t wait_value) {
  return futex(futx, FUTEX_WAIT, wait_value, NULL, NULL, 0);
}

static long futex_wake(uint32_t *futx, uint32_t num_to_wake) {
  return futex(futx, FUTEX_WAKE, num_to_wake, NULL, NULL, 0);
}

enum { UNLOKED, LOCKED_NO_WAITERS, LOCKED_WITH_WAITERS };

int my_mutex_init(my_mutex_t *m) {
  *m = UNLOKED;
  return 0;
}

// int my_mutex_lock(my_mutex_t *m) {
//   int c;
//   uint32_t zero;
//   long err;
//   if ((c = atomic_compare_exchange_strong(m, &zero, 1)) != 0) {
//     if (c != 2) {
//       c = atomic_exchange(m, 2);
//     }
//     while (c != 0) {
//       err = futex_wait(m, 2);
//       if (err == -1 && errno != EAGAIN) {
//         return err;
//       }
//       c = atomic_exchange(m, 2);
//     }
//   }
//   return 0;
// }

int my_mutex_lock(my_mutex_t *m) {
  int c;
  long err;
  uint32_t zero = 0;
  if (!AO_int_compare_and_swap(m, zero, 1)) {
    if (c != 2) {
      c = atomic_exchange(m, 2);
    }
    while (c != 0) {
      err = futex_wait(m, 2);
      if (err == -1 && errno != EAGAIN) {
        return err;
      }
      c = atomic_exchange(m, 2);
    }
  }
  return 0;
}

int my_mutex_unlock(my_mutex_t *m) {
  if (atomic_fetch_sub(m, 1) != 1) {
    *m = 0;
    futex_wake(m, 1);
  }
  return 0;
}
