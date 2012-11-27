#include "name_handler.h"
#include <limits.h>

typedef struct name_handler {
    unsigned int last_name;
    bool initialized;
    link_list_t *reusable_names;
} name_handler_t;
static name_handler_t name_handler;

mutex_static_init (name_handler_mutex);

void
name_handler_init ()
{
    if (name_handler.initialized)
        return;

    mutex_lock (name_handler_mutex);
    if (name_handler.initialized)
        return;

    name_handler.reusable_names = NULL;
    name_handler.last_name = 0;

    mutex_unlock (name_handler_mutex);
}

void
name_handler_alloc_names (GLsizei n,
                          GLuint *buffers)
{
    mutex_lock (name_handler_mutex);

    int i = 0;
    while (i < n) {
        unsigned next_name = 0;
        if (name_handler.reusable_names != NULL) {
            next_name = * ((GLuint *) name_handler.reusable_names->data);
            link_list_delete_element (&name_handler.reusable_names, name_handler.reusable_names);
        } else {
            next_name = ++name_handler.last_name;
            assert (next_name != UINT_MAX);
        }

        buffers[i] = next_name;
        i++;
    }

    mutex_unlock (name_handler_mutex);
}

void
name_handler_delete_names (GLsizei n,
                           const GLuint *buffers)
{
    mutex_lock (name_handler_mutex);

    int i;
    for (i = 0; i < n; i++) {
        GLuint *reusable_name = malloc (sizeof (GLuint));
        *reusable_name = buffers[i];
        link_list_append (&name_handler.reusable_names, reusable_name, free);
    }

    mutex_unlock (name_handler_mutex);
}
