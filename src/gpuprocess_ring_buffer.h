#ifndef GPUPROCESS_RING_BUFFER_H
#define GPUPROCESS_RING_BUFFER_H

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct buffer
{
    void *address;

    size_t length;
    size_t tail;
    size_t head;

    volatile size_t fill_count;
} buffer_t;

void
buffer_create(buffer_t *buffer, unsigned long length);
void
buffer_free(buffer_t *buffer);
size_t
buffer_num_entries(buffer_t *buffer);
void *
buffer_write_address(buffer_t *buffer, size_t *writable_bytes);
void
buffer_write_advance(buffer_t *buffer, size_t count_bytes);
void *
buffer_read_address(buffer_t *buffer, size_t *bytes_to_read);
void
buffer_read_advance(buffer_t *buffer, size_t count_bytes);
void
buffer_clear(buffer_t *buffer);

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_RING_BUFFER_H */