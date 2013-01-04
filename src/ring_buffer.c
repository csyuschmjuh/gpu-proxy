#include "config.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "ring_buffer.h"

private void
report_exceptional_condition(const char* error)
{
    fprintf(stderr, "%s: %s\n", error, strerror (errno));
}

void
buffer_create(buffer_t *buffer, int size, const char *buffer_name)
{
    /* The size of the buffer (in bytes). Note that for some buffers
     * such as the memory-mirrored ring buffer the actual buffer size
     * may be larger.
     */
    /* TODO: Make this configurable. */
    static unsigned long default_buffer_size = 1024 * 512;
    unsigned long buffer_size = 1024 * size;
    int name_length = strlen (buffer_name);
    
    char *path = malloc (sizeof (char) * (name_length + 29));
    memcpy (path, "/dev/shm/ring-buffer-", 21);
    memcpy (path + 21, buffer_name, name_length);
    memcpy (path + 21 + name_length, "-XXXXXX", 7);
    path[name_length+28] = 0;
 
    if (buffer_size < default_buffer_size)
        buffer_size = default_buffer_size;

    //char path[] = "/dev/shm/ring-buffer-XXXXXX";
    int file_descriptor;
    void *address;
    int status;

    file_descriptor = mkstemp (path);
    if (file_descriptor < 0) {
        free (path);
        report_exceptional_condition("Could not get a file descriptor.");
        return;
    }

    status = unlink(path);
    if (status)
        report_exceptional_condition("Could not unlink.");

    // Round up the length to the nearest page boundary.
    long page_size = sysconf(_SC_PAGESIZE);
    buffer->length = ((buffer_size + page_size - 1) / page_size) * page_size;

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
    free (path);
}

void
buffer_free (buffer_t *buffer)
{
    if (munmap (buffer->address, buffer->length << 1))
        report_exceptional_condition("Could not unmap memory.");
}

size_t
buffer_num_entries(buffer_t *buffer)
{
    return buffer->fill_count;
}

size_t
buffer_size(buffer_t *buffer)
{
    return buffer->length;
}

void *
buffer_write_address (buffer_t *buffer,
                     size_t *writable_bytes)
{
    *writable_bytes = (buffer->length - buffer->fill_count);
    if (*writable_bytes == 0)
        return NULL;
    return ((char*)buffer->address + buffer->head);
}

void
buffer_write_advance (buffer_t *buffer,
                     size_t count_bytes)
{
    buffer->head = (buffer->head + count_bytes) % buffer->length;
    __sync_add_and_fetch (&buffer->fill_count, count_bytes);
}

void *
buffer_read_address(buffer_t *buffer,
                    size_t *bytes_to_read)
{
    *bytes_to_read = buffer->fill_count;
    if (*bytes_to_read == 0)
        return NULL;
    return ((char*) buffer->address + buffer->tail);
}

void
buffer_read_advance(buffer_t *buffer,
                    size_t count_bytes)
{
    buffer->tail = (buffer->tail + count_bytes) % buffer->length;
    __sync_sub_and_fetch (&buffer->fill_count, count_bytes);
}

void
buffer_clear(buffer_t *buffer)
{
    buffer->head = buffer->tail = 0;
    buffer->fill_count = 0;
}
