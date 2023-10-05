/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

/* We rely heavily on various flags the CLONE function understands:
     CLONE_VM, CLONE_FS, CLONE_FILES
These flags select semantics with shared address space and
file descriptors according to what POSIX requires.
    CLONE_SIGHAND, CLONE_THREAD
This flag selects the POSIX signal semantics and various
other kinds of sharing (itimers, POSIX timers, etc.).
    CLONE_SETTLS
The sixth parameter to CLONE determines the TLS area for the
new thread.
    CLONE_PARENT_SETTID
The kernels writes the thread ID of the newly created thread
into the location pointed to by the fifth parameters to CLONE.
Note that it would be semantically equivalent to use
CLONE_CHILD_SETTID but it is be more expensive in the kernel.
    CLONE_CHILD_CLEARTID
The kernels clears the thread ID of a thread that has called
sys_exit() in the location pointed to by the seventh parameter
to CLONE.
    The termination signal is chosen to be zero which means no signal
    is sent.  */
// const int clone_flags = (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SYSVSEM
//         | CLONE_SIGHAND | CLONE_THREAD
//         | CLONE_SETTLS | CLONE_PARENT_SETTID
//         | CLONE_CHILD_CLEARTID
//         | 0);

#define _GNU_SOURCE

#define SIGCANCEL SIGRTMIN

#include "./my-core-threads.h"
#include <errno.h>
#include <linux/futex.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <threads.h>
#include <unistd.h>

static const unsigned long int STACK_SIZE = 2 * 1024 * 1024;

static const uint32_t MYCORETHREAD_EXECUTING = 0;
static const uint32_t MYCORETHREAD_FINISHED = 1;

typedef struct {
  void *arg;
  void *result;
  void *(*routine)(void *);
} foo_with_args_and_result;

typedef struct {
  foo_with_args_and_result foo;
  atomic_bool joinable;
  atomic_bool canceled;
  uint32_t futex;
  pid_t tid;
} mycorethread_real_t;

const int clone_flags = (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SYSVSEM |
                         CLONE_SIGHAND | CLONE_THREAD);

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

static void *create_stack(off_t size) {
  void *stack = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
  if (stack == MAP_FAILED) {
    return NULL;
  }

  if (mprotect(stack, getpagesize(), PROT_NONE)) {
    return NULL;
  }
  return stack;
}

void sigcancelhandler(int signal, siginfo_t *info, void *context) {
  //    setcontext((ucontext_t*) context);
  kill(getpid(), SIGTERM);
  // printf("i am here\n");
}

static int wrapper(void *arg) {
  mycorethread_real_t *strct = arg;
  int err;

  struct sigaction action;
  action.sa_sigaction = sigcancelhandler;
  action.sa_flags = SA_SIGINFO;

  err = sigemptyset(&action.sa_mask);
  if (err) {
    return -1;
  }

  err = sigaction(SIGCANCEL, &action, NULL);
  if (err) {
    return -1;
  }
  strct->tid = gettid();
  strct->foo.result = strct->foo.routine(strct->foo.arg);
  if (strct->joinable) {
    strct->futex = MYCORETHREAD_FINISHED;
    return futex_wake(&(strct->futex), 1);
  } else {
    return 0;
  }
}

int mycorethread_create(mycorethread_t *thread, void *(*routine)(void *),
                        void *args) {
  void *stack = create_stack(STACK_SIZE);
  if (stack == NULL) {
    return -1;
  }

  mycorethread_real_t *thread_data =
      stack + STACK_SIZE - sizeof(mycorethread_real_t);

  thread_data->foo.routine = routine;
  thread_data->foo.arg = args;
  thread_data->joinable = 1;
  thread_data->futex = MYCORETHREAD_EXECUTING;
  thread_data->canceled = 0;

  pid_t pid;
  pid = clone(wrapper, thread_data, clone_flags, thread_data);
  if (pid == -1) {
    return -1;
  }

  //        sigset_t sigset;
  //    err = sigemptyset(&sigset);
  //    if (err) {
  //        return -1;
  //    }
  //
  //    err = sigaddset(&sigset, SIGCANCEL);
  //    if (err) {
  //        return -1;
  //    }
  //
  //    err = sigprocmask(SIG_BLOCK, &sigset, NULL);
  //    if (err) {
  //        return -1;
  //    }

  *thread = (mycorethread_t)thread_data;
  return 0;
}

int mycorethread_join(mycorethread_t thread, void **result) {
  mycorethread_real_t *thread_data = (mycorethread_real_t *)thread;
  if (thread_data->joinable && !thread_data->canceled) {
    int err;
    err = futex_wait(&(thread_data->futex), MYCORETHREAD_EXECUTING);
    if (err == -1 && errno != EAGAIN) {
      //        int errno_temp = errno;
      //        munmap(thread_data->stack, STACK_SIZE);
      //        errno = errno_temp;
      return err;
    }
    if (result) {
      *result = (thread_data->foo).result;
    }
    return 0; // munmap(thread_data->stack, STACK_SIZE);
  } else {
    errno = EINVAL;
    return -1;
  }
}

int mycorethread_detach(mycorethread_t thread) {
  mycorethread_real_t *thread_data = (mycorethread_real_t *)thread;
  if (thread_data->canceled) {
    errno = EINVAL;
    return -1;
  }
  thread_data->joinable = 0;
  return 0;
}

int mycorethread_cancel(mycorethread_t thread) {
  mycorethread_real_t *thread_data = (mycorethread_real_t *)thread;
  thread_data->canceled = 1;
  //    printf("%d %d\n", getpid(), thread_data->tid);
  int err = tgkill(getpid(), thread_data->tid, SIGCANCEL);
  if (err) {
    return -1;
  }
  return 0;
}
