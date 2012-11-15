#include "name_handler.h"
#include <limits.h>

typedef struct name_handler {
    unsigned int last_name;
    bool initialized;
    HashTable *used_names;
} name_handler_t;

static char *dummyData = "";
static name_handler_t name_handler;

mutex_static_init (name_handler_mutex);

void
name_handler_init ()
{
    if (name_handler.initialized)
        return;

    mutex_lock (name_handler_mutex);

    name_handler.initialized = true;
    name_handler.last_name = 0;
    name_handler.used_names = NewHashTable(free);

    mutex_unlock (name_handler_mutex);
}

void
name_handler_alloc_names (GLsizei n,
                          GLuint *buffers)
{
    int i = 0;

    mutex_lock (name_handler_mutex);

    while (i < n) {
        int next_name = ++name_handler.last_name;

        if (next_name == UINT_MAX)
            name_handler.last_name = next_name = 0;

        if (!HashLookup (name_handler.used_names, next_name)) {
            buffers[i] = next_name;
            HashInsert (name_handler.used_names, next_name, dummyData);
            i++;
        }
    }

    mutex_unlock (name_handler_mutex);
}

void
name_handler_delete_names (GLsizei n,
                           const GLuint *buffers)
{
    int i;

    mutex_lock (name_handler_mutex);

    for (i = 0; i < n; i++)
        HashRemove (name_handler.used_names, buffers[i]);

    mutex_unlock (name_handler_mutex);
}
