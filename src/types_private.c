#include "types_private.h"
#include <string.h>
#include <stdlib.h>

link_list_t *
link_list_append (link_list_t **list, void *data)
{
    link_list_t *new_element = (link_list_t *) malloc (sizeof (link_list_t));
    new_element->data = data;
    new_element->next = NULL;
    new_element->prev = NULL;

    if (!*list) {
        *list = new_element;
        return new_element;
    }

    link_list_t *current = *list;
    while (current->next != NULL)
        current = current->next;

    new_element->prev = current;
    current->next = new_element;
    return new_element;
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
