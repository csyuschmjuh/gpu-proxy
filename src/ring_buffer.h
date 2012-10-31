#ifndef GPUPROCESS_RING_BUFFER_H
#define GPUPROCESS_RING_BUFFER_H

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include "compiler_private.h"

#include "thread_private.h"

typedef struct buffer
{
    void *address;

    size_t length;
    size_t tail;
    size_t head;
    unsigned int last_token;

    volatile size_t fill_count;

    mutex_t mutex;
    signal_t signal;
    bool mutex_lock;
} buffer_t;

private void
buffer_create(buffer_t *buffer);

private void
buffer_free(buffer_t *buffer);

private size_t
buffer_num_entries(buffer_t *buffer);

private void *
buffer_write_address(buffer_t *buffer, size_t *writable_bytes);

private void
buffer_write_advance(buffer_t *buffer, size_t count_bytes);

private void *
buffer_read_address(buffer_t *buffer, size_t *bytes_to_read);

private void
buffer_read_advance(buffer_t *buffer, size_t count_bytes);

private void
buffer_clear(buffer_t *buffer);

private bool
buffer_use_mutex (buffer_t *buffer);

#endif /* GPUPROCESS_RING_BUFFER_H */
