#include "config.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "ring_buffer.h"

static bool
_use_mutex (void)
{
    const char *env = getenv ("GPUPROCESS_LOCK");
    return env && !strcmp (env, "mutex");
}

private void
report_exceptional_condition(const char* error)
{
    fprintf(stderr, "%s: %s\n", error, strerror (errno));
}

void
buffer_create(buffer_t *buffer)
{
    /* The size of the buffer (in bytes). Note that for some buffers
     * such as the memory-mirrored ring buffer the actual buffer size
     * may be larger.
     */
    /* TODO: Make this configurable. */
    static unsigned long default_buffer_size = 1024 * 1024;

    char path[] = "/dev/shm/ring-buffer-XXXXXX";
    int file_descriptor;
    void *address;
    int status;

    file_descriptor = mkstemp (path);
    if (file_descriptor < 0) {
        report_exceptional_condition("Could not get a file descriptor.");
        return;
    }

    status = unlink(path);
    if (status)
        report_exceptional_condition("Could not unlink.");

    // Round up the length to the nearest page boundary.
    long page_size = sysconf(_SC_PAGESIZE);
    buffer->length = ((default_buffer_size + page_size - 1) / page_size) * page_size;

    buffer->fill_count = 0;
    buffer->head = buffer->tail = 0;

    status = ftruncate(file_descriptor, buffer->length);
    if (status)
        report_exceptional_condition("Could not truncate.");

    buffer->address = mmap (NULL, buffer->length << 1, PROT_NONE,
                                                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (buffer->address == MAP_FAILED)
        report_exceptional_condition("Failed to map full memory.");

    address =
        mmap (buffer->address, buffer->length, PROT_READ | PROT_WRITE,
                    MAP_FIXED | MAP_SHARED, file_descriptor, 0);

    if (address != buffer->address)
        report_exceptional_condition("Failed to map initial memory.");

    address = mmap (buffer->address + buffer->length,
                                    buffer->length, PROT_READ | PROT_WRITE,
                                    MAP_FIXED | MAP_SHARED, file_descriptor, 0);

    if (address != buffer->address + buffer->length)
        report_exceptional_condition("Failed to map mirror memory.");

    status = close(file_descriptor);
    if (status)
        report_exceptional_condition("Could not close file descriptor.");

    buffer->last_token = 0;

    buffer->mutex_lock = _use_mutex ();
    
    if (buffer->mutex_lock) {
        pthread_mutex_init (&buffer->mutex, NULL);
        pthread_cond_init (&buffer->signal, NULL);
    }
}

void
buffer_free(buffer_t *buffer)
{
    int status;

    status = munmap (buffer->address, buffer->length << 1);
    if (status)
        report_exceptional_condition("Could not unmap memory.");

    if (buffer->mutex_lock) {
        pthread_mutex_destroy (&buffer->mutex);
        pthread_cond_destroy (&buffer->signal);
    }
}

size_t
buffer_num_entries(buffer_t *buffer)
{
    return buffer->fill_count;
}

void *
buffer_write_address(buffer_t *buffer,
                     size_t *writable_bytes)
{
    *writable_bytes = buffer->length - buffer->fill_count;
    if (*writable_bytes == 0) {
        if (buffer->mutex_lock) {
            pthread_mutex_lock (&buffer->mutex);

            while (buffer->length - buffer->fill_count == 0) {
                pthread_cond_wait (&buffer->signal, &buffer->mutex);
            }

            pthread_mutex_unlock (&buffer->mutex);
        }
        else
            return NULL;
    }
    return ((char*)buffer->address + buffer->head);
}

void
buffer_write_advance(buffer_t *buffer,
                     size_t count_bytes)
{
    buffer->head = (buffer->head + count_bytes) % buffer->length;
    __sync_add_and_fetch (&buffer->fill_count, count_bytes);
   
    if (buffer->mutex_lock) 
        pthread_cond_signal (&buffer->signal);
}

void *
buffer_read_address(buffer_t *buffer,
                    size_t *bytes_to_read)
{
    *bytes_to_read = buffer->fill_count;
    if (buffer->fill_count == 0) {
        if (buffer->mutex_lock) {
            pthread_mutex_lock (&buffer->mutex);
            while (buffer->fill_count == 0) {
                pthread_cond_wait (&buffer->signal, &buffer->mutex);
            }

            pthread_mutex_unlock (&buffer->mutex);
        }
        else
            return NULL;
    }

    return ((char*) buffer->address + buffer->tail);
}

void
buffer_read_advance(buffer_t *buffer,
                    size_t count_bytes)
{
    buffer->tail = (buffer->tail + count_bytes) % buffer->length;
    __sync_sub_and_fetch (&buffer->fill_count, count_bytes);

    if (buffer->mutex_lock)
        pthread_cond_signal (&buffer->signal);
}

void
buffer_clear(buffer_t *buffer)
{
    buffer->head = buffer->tail = 0;
    buffer->fill_count = 0;
}

bool
buffer_use_mutex (buffer_t *buffer)
{
    return buffer->mutex_lock;
}

private void
buffer_signal_waiter (buffer_t *buffer)
{
    if (buffer->mutex_lock)
        pthread_cond_signal (&buffer->signal);
}
