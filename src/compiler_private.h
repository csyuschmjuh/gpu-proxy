#ifndef GPUPROCESS_COMPILER_PRIVATE_H
#define GPUPROCESS_COMPILER_PRIVATE_H

#include <assert.h>

#define public  __attribute__((visibility("default")))
#define private __attribute__((visibility("hidden")))

#ifndef ENABLE_TESTING
#define exposed_to_tests static
#else
#define exposed_to_tests
#endif

#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define likely(expr) (__builtin_expect (!!(expr), 1))
#define unlikely(expr) (__builtin_expect (!!(expr), 0))
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif


#endif
