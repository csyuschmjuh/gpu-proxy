#include "types_private.h"
#include <string.h>
#include <stdlib.h>

link_list_t *
link_list_new (void *data)
{
    link_list_t *list = (link_list_t *) malloc (sizeof (link_list_t));
    list->data = data;
    list->prev = NULL;
    list->next = NULL;

    return list;
}

void
link_list_append (link_list_t *list, link_list_t *element)
{
    element->prev = list;
    list->next = element;
}

void
link_list_free (link_list_t* array)
{
    if (!array)
        return;

    array->prev->next = NULL;
    while (array) {
        link_list_t *current = array;
        array = array->next;
        free (current->data);
        free (current);
    }
}
