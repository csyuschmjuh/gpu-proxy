#ifndef GPUPROCESS_THREAD_H
#define GPUPROCESS_THREAD_H

#include <pthread.h>

/* mutex definition */
typedef pthread_mutex_t                 mutex_t;

#define mutex_init(name)     pthread_mutex_init (&(name), NULL)
#define mutex_destroy(name)  pthread_mutex_destroy (&(name))
#define mutex_lock(name)     pthread_mutex_lock (&(name))
#define mutex_unlock(name)   pthread_mutex_unlock (&(name))

/* pthread condition */
typedef pthread_cond_t                  signal_t;

typedef pthread_t                       thread_t;

#define wait_signal(name1, name2)   pthread_cond_wait (&(name1), &(name2))
#define signal(name)         pthread_cond_signal (&(name))
#define signal_init(name)    pthread_cond_init (&(name), NULL)
#define signal_destroy(name) pthread_cond_destroy (&(name))
#define broadcast(name)      pthread_cond_broadcast (&(name))

/* static initializer */
#define mutex_static_init(name) \
    static mutex_t name = PTHREAD_MUTEX_INITIALIZER

#define signal_static_init(name) \
    static signal_t name = PTHREAD_COND_INITIALIZER

#endif /* GPUPROCESS_THREAD_H */
