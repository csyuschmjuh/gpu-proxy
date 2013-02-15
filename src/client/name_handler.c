#include "name_handler.h"
#include <limits.h>

name_handler_t *
name_handler_create ()
{
    name_handler_t *name_handler = (name_handler_t *) malloc (sizeof(name_handler_t));
    name_handler->reusable_names = NULL;
    name_handler->last_name = 0;

    return name_handler;
}

void
name_handler_destroy (name_handler_t *name_handler)
{
    link_list_clear (&name_handler->reusable_names);
    free (name_handler);
}

void
name_handler_alloc_names (name_handler_t *name_handler,
                          GLsizei n,
                          GLuint *buffers)
{
    int i = 0;
    while (i < n) {
        unsigned next_name = 0;
        if (name_handler->reusable_names != NULL) {
            next_name = * ((GLuint *) name_handler->reusable_names->data);
            link_list_delete_element (&name_handler->reusable_names, name_handler->reusable_names);
        } else {
            next_name = ++name_handler->last_name;
            assert (next_name != UINT_MAX);
        }

        buffers[i] = next_name;
        i++;
    }
}

void
name_handler_alloc_name (name_handler_t *name_handler,
                         GLuint buffer)
{
    if (buffer >= name_handler->last_name)
        ++name_handler->last_name;
    else {
        link_list_t *current = name_handler->reusable_names;
        while (current) {
            if ( *(GLuint*)current->data == buffer) {
                link_list_delete_element (&name_handler->reusable_names, current);
                break;
            }
            current = current->next;
        }
    }
}

void
name_handler_delete_names (name_handler_t *name_handler,
                           GLsizei n,
                           const GLuint *buffers)
{
    int i;
    for (i = 0; i < n; i++) {
        if (buffers[i] == 0)
            continue;
        GLuint *reusable_name = malloc (sizeof (GLuint));
        *reusable_name = buffers[i];
        link_list_append (&name_handler->reusable_names, reusable_name, free);
    }
}
