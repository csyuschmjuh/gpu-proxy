#include "config.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "ring_buffer.h"

#define TEMP_FILE_FORMAT  "-XXXXXX"

private void
report_exceptional_condition(const char* error)
{
    fprintf(stderr, "%s: %s\n", error, strerror (errno));
}

char *
get_path_string (const char *buffer_name,
                 const char* type_name)
{
    int buffer_length = strlen (buffer_name);
    int type_length = strlen (type_name);
    int format_length = strlen (TEMP_FILE_FORMAT);
    char *path = malloc (sizeof (char) * (buffer_length + type_length + format_length + 1));

    memcpy (path, type_name, type_length);
    memcpy (path + type_length, buffer_name, buffer_length);
    memcpy (path + type_length + buffer_length, TEMP_FILE_FORMAT, format_length);
    path[type_length + buffer_length + format_length] = 0;

    return path;
}

buffer_t *
buffer_create(int size, const char *buffer_name)
{
    /* The size of the buffer (in bytes). Note that for some buffers
     * such as the memory-mirrored ring buffer the actual buffer size
     * may be larger.
     */
    /* TODO: Make this configurable. */
    static unsigned long default_buffer_size = 1024 * 512;
    unsigned long buffer_size = 1024 * size;

    char *buffer_path = get_path_string (buffer_name, "/dev/shm/ring-buffer-");
    char *buffer_info_path = get_path_string (buffer_name, "/dev/shm/ring-buffer-info-");

    if (buffer_size < default_buffer_size)
        buffer_size = default_buffer_size;

    // char bufer_path[] = "/dev/shm/ring-buffer-XXXXXX";
    int buffer_file_descriptor;
    int buffer_info_file_descriptor;
    int status;

    buffer_file_descriptor = mkstemp (buffer_path);
    buffer_info_file_descriptor = mkstemp (buffer_info_path);
    if (buffer_file_descriptor < 0 || buffer_info_file_descriptor < 0) {
        free (buffer_path);
        free (buffer_info_path);
        report_exceptional_condition ("Could not get the file descriptors.");
        return NULL;
    }

    status = unlink (buffer_path);
    if (status)
        report_exceptional_condition("Could not unlink.");

    status = unlink (buffer_info_path);
    if (status)
        report_exceptional_condition("Could not unlink.");

   // Round up the length to the nearest page boundary.
    long page_size = sysconf(_SC_PAGESIZE);
    size_t length = ((buffer_size + page_size - 1) / page_size) * page_size;

    status = ftruncate(buffer_file_descriptor, length);
    if (status)
        report_exceptional_condition("Could not truncate.");

    size_t info_length = ((sizeof (buffer_t) + page_size - 1) / page_size) * page_size;
    status = ftruncate(buffer_info_file_descriptor, info_length);
    if (status)
        report_exceptional_condition("Could not truncate.");

    buffer_t *buffer = mmap (NULL, info_length, PROT_READ | PROT_WRITE,
                             MAP_SHARED, buffer_info_file_descriptor, 0);
    if (buffer == MAP_FAILED)
        report_exceptional_condition("Failed to map buffer memory.");

    buffer->fill_count = 0;
    buffer->head = buffer->tail = 0;
    buffer->last_token = 0;
    buffer->length = length;
    buffer->info_length = info_length;

    buffer->address = mmap (NULL, length << 1, PROT_NONE,
                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (buffer->address == MAP_FAILED)
        report_exceptional_condition("Failed to map full memory.");

    void *address = mmap (buffer->address, length, PROT_READ | PROT_WRITE,
                          MAP_FIXED | MAP_SHARED, buffer_file_descriptor, 0);

    if (buffer->address != address)
        report_exceptional_condition("Failed to map initial memory.");

    address = mmap (address + length,
                    length, PROT_READ | PROT_WRITE,
                    MAP_FIXED | MAP_SHARED, buffer_file_descriptor, 0);

    if (address != buffer->address + length)
        report_exceptional_condition("Failed to map mirror memory.");

    status = close(buffer_file_descriptor);
    if (status)
        report_exceptional_condition("Could not close file descriptor.");

    status = close(buffer_info_file_descriptor);
    if (status)
        report_exceptional_condition("Could not close file descriptor.");

    free (buffer_path);
    free (buffer_info_path);

    return buffer;
}

void
buffer_free (buffer_t *buffer)
{
    if (munmap (buffer->address, buffer->length << 1))
        report_exceptional_condition("Could not unmap memory.");

    if (munmap (buffer, buffer->info_length))
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
