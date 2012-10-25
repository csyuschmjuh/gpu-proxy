#ifndef GPUPROCESS_TYPES_PRIVATE_H
#define GPUPROCESS_TYPES_PRIVATE_H

#include <stdbool.h>

typedef struct link_list
{
    void *data;
    struct link_list *next;
    struct link_list *prev;
} link_list_t;

#define v_ref_count_t unsigned int

#define v_client_id_t pid_t

#endif /* GPUPROCESS_TYPES_PRIVATE_H */
