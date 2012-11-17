#ifndef GPUPROCESS_TYPES_PRIVATE_H
#define GPUPROCESS_TYPES_PRIVATE_H

#include <stdbool.h>

typedef void (*list_delete_function_t)(void *data);

typedef struct link_list
{
    void *data;
    struct link_list *next;
    struct link_list *prev;
    list_delete_function_t delete_function;
} link_list_t;

#define v_ref_count_t unsigned int

#define v_client_id_t pid_t

void
link_list_append (link_list_t **list,
                  void *data,
                  list_delete_function_t delete_function);

void
link_list_delete_first_entry_matching_data (link_list_t **list,
                                            void *data);

void
link_list_delete_element (link_list_t **list,
                          link_list_t *element);

void
link_list_clear (link_list_t **array);

#endif /* GPUPROCESS_TYPES_PRIVATE_H */
