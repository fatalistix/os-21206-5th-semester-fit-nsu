/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#pragma once

typedef unsigned long int mycorethread_t;

int mycorethread_create(mycorethread_t *thread, void *(*routine)(void *),
                        void *args);

int mycorethread_join(mycorethread_t thread, void **result);

int mycorethread_detach(mycorethread_t thread);

int mycorethread_cancel(mycorethread_t thread);
