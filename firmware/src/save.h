/*
 * Controller Flash Save and Load
 * WHowe <github.com/whowechina>
 */

#ifndef SAVE_H
#define SAVE_H

#include <stdlib.h>
#include <stdbool.h>

/* It's safer to lock other I/O ops during saving, so we need a locker */
typedef void (*io_locker_func)(bool pause);
void save_init(io_locker_func locker);

void save_loop();

void *save_alloc(size_t size, void *def, void (*after_load)());
void save_request(bool immediately);

#endif
