#include "types_private.h"
#include <string.h>
#include <stdlib.h>

void
link_list_append (link_list_t **list,
                  void *data,
                  list_delete_function_t delete_function)
{
    link_list_t *new_element = (link_list_t *) malloc (sizeof (link_list_t));
    new_element->data = data;
    new_element->next = NULL;
    new_element->prev = NULL;
    new_element->delete_function = delete_function;

    if (!*list) {
        *list = new_element;
        return;
    }

    link_list_t *current = *list;
    while (current->next != NULL)
        current = current->next;

    new_element->prev = current;
    current->next = new_element;
}

void
link_list_prepend (link_list_t **list,
                   void *data,
                   list_delete_function_t delete_function)
{
    link_list_t *new_list = (link_list_t *) malloc (sizeof (link_list_t));
    new_list->data = data;
    new_list->delete_function = delete_function;
    new_list->prev = NULL;

    if (*list) {
        new_list->next = *list;
        (*list)->prev = new_list;
    } else {
        new_list->next = NULL;
    }

    *list = new_list;
}

void
link_list_delete_first_entry_matching_data (link_list_t **list, void *data)
{
    link_list_t *current = *list;
    while (current) {
        if (current->data == data) {
            link_list_delete_element (list, current);
            return;
        }
        current = current->next;
    }
}

void
link_list_delete_element (link_list_t **list, link_list_t *element)
{
    if (*list == element) {
        *list = element->next;
    } else {
        element->prev->next = element->next;
        if (element->next)
            element->next->prev = element->prev;
    }

    element->delete_function (element->data);
    free (element);
}

void
link_list_clear (link_list_t** list)
{
    if (!*list)
        return;

    link_list_t *current = *list;
    while (current) {
        link_list_t *next = current->next;
        current->delete_function (current->data);
        free (current);
        current = next;
    }

    *list = NULL;
}
