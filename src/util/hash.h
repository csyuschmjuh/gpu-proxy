/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 * Copyright (C) 2012 Samsung   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file hash.h
 * Generic hash table.
 */

#ifndef HASH_H
#define HASH_H

#include "compiler_private.h"
#include "thread_private.h"
#include "types_private.h"

#include <GLES2/gl2.h>

#define TABLE_SIZE 1023  /**< Size of lookup table/array */

typedef void (*hash_delete_function_t)(void *data);

/**
 * The hash table data structure.
 */
typedef struct _HashTable {
    struct HashEntry *Table[TABLE_SIZE];  /**< the lookup table */
    GLuint MaxKey;                        /**< highest key inserted so far */
    GLboolean InDeleteAll;                /**< Debug check */
    hash_delete_function_t delete_function;
} HashTable;

HashTable *new_hash_table (hash_delete_function_t delete_function);

private void
delete_hash_table (HashTable *table);

private void *
hash_lookup (HashTable *table,
             GLuint key);

private void
hash_insert (HashTable *table,
             GLuint key,
             void *data);

private void
hash_remove (HashTable *table,
             GLuint key);

private void *
hash_take (HashTable *table,
           GLuint key);

private void
hash_delete_all (HashTable *table,
                 void (*callback) (GLuint key, void *data, void *userData),
                 void *userData);

private void
hash_walk (const HashTable *table,
          void (*callback) (GLuint key, void *data, void *userData),
          void *userData);

bool
hash_has_element (const HashTable *table,
                  const void *userData,
                  bool (*callback)(const void *data, const void *userData));

private GLuint
hash_first_entry (HashTable *table);

private GLuint
hash_next_entry (const HashTable *table,
                 GLuint key);

private void
hash_print (const HashTable *table);

private GLuint
hash_find_free_key_block (HashTable *table,
                          GLuint numKeys);

private GLuint
has_num_entries (const HashTable *table);

private void
free_data_callback (GLuint key,
                    void *data,
                    void *userData);

private GLuint
hash_str (const void *v);

private GLuint
hash_int (const void *v);

private void
test_hash_functions (void);

#endif
