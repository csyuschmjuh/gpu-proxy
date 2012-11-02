
#include "name_handler.h"

#include <limits.h>

static char *dummyData = "";

name_handler_t *
name_handler_create () {
    name_handler_t *name_handler;
    int i;

    name_handler = (name_handler_t *)malloc (sizeof (name_handler_t));
    for (i = 0; i < RESOURCE_MAX; i++) {
        name_handler->last_name[i] = 0;
        name_handler->used_names[i] = NewHashTable();
    }

    return name_handler;
}

void
name_handler_destroy (name_handler_t *name_handler) {

    int i;

    for (i = 0; i < RESOURCE_MAX; i++)
        DeleteHashTable (name_handler->used_names[i]);

    free (name_handler);
}

void
name_handler_alloc_names (name_handler_t *name_handler,
                          resource_namespace_t namespace,
                          GLsizei n,
                          GLuint *buffers) {
    int i = 0;

    while (i<n) {
        int next_name = ++name_handler->last_name[namespace];

        if (next_name == UINT_MAX)
            name_handler->last_name[namespace] = next_name = 0;

        if (!HashLookup (name_handler->used_names[i], next_name)) {
            buffers[i] = next_name;
            HashInsert (name_handler->used_names[i], next_name, dummyData);
            i++;
        }
    }
}

void
name_handler_delete_names (name_handler_t *name_handler,
                           resource_namespace_t namespace,
                           GLsizei n,
                           const GLuint *buffers) {
    int i;

    for (i=0;i<n;i++)
        HashRemove (name_handler->used_names[i], buffers[i]);
}
