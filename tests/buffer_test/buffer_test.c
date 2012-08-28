#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "ring_buffer.h"

// The number of chunks to produce for the run.
#define AMOUNT_TO_PRODUCE 100

// The size of each chunk.
#define CHUNK_SIZE 64

// The size of the buffer (in bytes). Note that for some buffers
// such as the memory-mirrored ring buffer the actual buffer size
// may be larger.
#define BUFFER_SIZE 10

buffer test_buffer;

pthread_mutex_t consumer_thread_started_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline float
get_tick()
{
    struct timespec now;
    clock_gettime (CLOCK_THREAD_CPUTIME_ID, &now);
    return now.tv_sec * 1000000.0 + now.tv_nsec / 1000.0;
}

static inline float
random_amount_of_work ()
{
    float time_before_work = get_tick ();

    int number_of_loops = drand48 () * 1000;
    int i;
    for (i = 0; i < number_of_loops; i++)
        drand48 ();

    return get_tick () - time_before_work;
}

static inline float
random_amount_of_sleep (bool long_sleep)
{
    float time_before_work = get_tick ();
    usleep (drand48 () * (long_sleep ? 500 : 10));
    return get_tick () - time_before_work;
}

static inline void
sleep_nanoseconds (int num_nanoseconds)
{
    struct timespec spec;
    spec.tv_sec = 0;
    spec.tv_nsec = num_nanoseconds;
    nanosleep (&spec, NULL);
}

static inline void *
consumer_thread_func (void *ptr)
{
    // This signals the producer thread to start producing.
    pthread_mutex_unlock (&consumer_thread_started_mutex);

    int i = 0;
    char chunk_retrieved[CHUNK_SIZE];
    while (1) {

        float before = get_tick ();

        // Get the current read location for the ring buffer and
        // the amount of chunks left before it's empty.
        size_t data_left_to_read;
        char *read_location = (char *) buffer_read_address (&test_buffer,
                                                            &data_left_to_read);

        // The buffer is empty, so wait until there's something to read.
        while (! read_location || data_left_to_read < CHUNK_SIZE) {
            sleep_nanoseconds (100);
            read_location = (char *) buffer_read_address (&test_buffer,
                                                          &data_left_to_read);
        }

        // Consume!
        memcpy (chunk_retrieved, read_location, CHUNK_SIZE);
        buffer_read_advance (&test_buffer, CHUNK_SIZE);

        float time_to_consume = get_tick () - before;

        // Do a random amount of work to simulate the cost of the GL command.
        float time_slept = random_amount_of_sleep (i++ % 5 == 0);

        printf("\tConsumed command number %i in %0.3f and then slept for %0.3f (total=%0.3f)\n",
               *((int*) chunk_retrieved), time_to_consume, time_slept, get_tick () - before);

        // We know we've consumed everything when the chunk data is the last index.
        if (*((size_t*) chunk_retrieved) == AMOUNT_TO_PRODUCE - 1)
            break;
    }
}

static void
start_consumer_thread (pthread_t *thread)
{
    // We use a mutex here to wait until the thread has started.
    pthread_mutex_lock (&consumer_thread_started_mutex);
    pthread_create (thread, NULL, consumer_thread_func, NULL);
    pthread_mutex_lock (&consumer_thread_started_mutex);
}

static void
force_buffer_flush ()
{
    while (buffer_num_entries (&test_buffer) > 0)
        sleep_nanoseconds (100);
}

static void
produce_chunks ()
{
    char zero_chunk[CHUNK_SIZE];
    memset (zero_chunk, 0, CHUNK_SIZE);

    int i;
    for (i = 0; i < AMOUNT_TO_PRODUCE; i++) {

        float before = get_tick ();

        // Get the current write location for the ring buffer and
        // the amount of space left before it's full.
        size_t available_space;
        char *write_location = (char *) buffer_write_address (&test_buffer,
                                                              &available_space);

        // The buffer is too full to write to, we wait until there's space
        // available to write to it.
        while (! write_location || available_space < CHUNK_SIZE) {
            sleep_nanoseconds (100);
            write_location = (char *) buffer_write_address (&test_buffer,
                                                            &available_space);
        }

        // Produce!
        *((size_t *) zero_chunk) = i;
        memcpy (write_location, zero_chunk, CHUNK_SIZE);
        buffer_write_advance (&test_buffer, CHUNK_SIZE);

        if (i == AMOUNT_TO_PRODUCE / 2)
            force_buffer_flush ();

        float time_to_produce = get_tick () - before;

        // Do a random amount of work to simulate the benefits of parallelism.
        float time_worked = random_amount_of_work ();

        if (i == AMOUNT_TO_PRODUCE / 2) {
            printf("Produced synchronous command number %i in %0.3f and then worked for %0.3f (total=%0.3f)\n",
                   i, time_to_produce, time_worked, get_tick () - before);

        } else {
            printf("Produced command number %i in %0.3f and then worked for %0.3f (total=%0.3f)\n",
                   i, time_to_produce, time_worked, get_tick () - before);
        }
    }
}

static void
print_clock_resolution ()
{
   struct timespec clock_resolution;
   clock_getres (CLOCK_THREAD_CPUTIME_ID, &clock_resolution);
   printf ("Clock resolution seconds = %ld, nanoseconds = %ld\n", clock_resolution.tv_sec, clock_resolution.tv_nsec); 
}

int main (int argc, char **argv)
{
    print_clock_resolution ();

    buffer_create (&test_buffer, BUFFER_SIZE);

    pthread_t consumer_thread;
    start_consumer_thread (&consumer_thread);

    produce_chunks ();

    pthread_join (consumer_thread, NULL);

    buffer_free (&test_buffer);
    return 0;
}
