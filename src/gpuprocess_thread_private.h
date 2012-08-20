#ifndef GPUPROCESS_THREAD_H
#define GPUPROCESS_THREAD_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* mutex definition */
typedef pthread_mutex_t                        gpu_mutex_t;

#define gpu_mutex_init(name)                pthread_mutex_init (&(name), NULL)
#define gpu_mutex_destroy(name)        pthread_mutex_destroy (&(name))
#define gpu_mutex_lock(name)                pthread_mutex_lock (&(name))
#define gpu_mutex_unlock(name)                pthread_mutex_unlock (&(name))

/* pthread condition */
typedef pthread_cond_t                        gpu_signal_t;

#define gpu_wait_signal(name1, name2)        pthread_cond_wait (&(name1), &(name2))
#define gpu_signal(name)                pthread_cond_signal (&(name))
#define gpu_signal_init(name)                pthread_cond_init (&(name), NULL)
#define gpu_signal_destroy(name)        pthread_cond_destroy (&(name))

/* static initializer */
#define gpu_mutex_static_init(name) \
    static gpu_mutex_t name = PTHREAD_MUTEX_INITIALIZER

#define gpu_signal_static_init(name) \
    static gpu_signal_t name = PTHREAD_COND_INITIALIZER

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_THREAD_H */
