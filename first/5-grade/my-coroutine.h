/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#pragma once

typedef unsigned long int mycoroutine_t;

int mycoroutine_create(mycoroutine_t *coroutine, void *(*routine)(void *),
                       void *args);

int mycoroutine_join(mycoroutine_t coroutine, void **result);

int mycoroutine_detach(mycoroutine_t coroutine);

int mycoroutine_cancel(mycoroutine_t coroutine);

int scheduler_init(void (*main_routine)(int argc, char *argv[]), int argc,
                   char *argv[]);
