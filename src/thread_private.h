#ifndef GPUPROCESS_THREAD_H
#define GPUPROCESS_THREAD_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* mutex definition */
typedef pthread_mutex_t                 gpuprocess_mutex_t;

#define gpuprocess_mutex_init(name)     pthread_mutex_init (&(name), NULL)
#define gpuprocess_mutex_destroy(name)  pthread_mutex_destroy (&(name))
#define gpuprocess_mutex_lock(name)     pthread_mutex_lock (&(name))
#define gpuprocess_mutex_unlock(name)   pthread_mutex_unlock (&(name))

/* pthread condition */
typedef pthread_cond_t                  gpuprocess_signal_t;

typedef pthread_t                       gpuprocess_thread_t;

#define gpuprocess_wait_signal(name1, name2)   pthread_cond_wait (&(name1), &(name2))
#define gpuprocess_signal(name)         pthread_cond_signal (&(name))
#define gpuprocess_signal_init(name)    pthread_cond_init (&(name), NULL)
#define gpuprocess_signal_destroy(name) pthread_cond_destroy (&(name))

/* static initializer */
#define gpuprocess_mutex_static_init(name) \
    static gpuprocess_mutex_t name = PTHREAD_MUTEX_INITIALIZER

#define gpuprocess_signal_static_init(name) \
    static gpuprocess_signal_t name = PTHREAD_COND_INITIALIZER


#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_THREAD_H */
