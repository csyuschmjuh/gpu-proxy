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
 * \file hash.c
 * Generic hash table.
 *
 * Used for display lists, texture objects, vertex/fragment programs,
 * buffer objects, etc.  The hash functions are thread-safe.
 *
 * Forked from mesa e00abb00f058f7476d21ee4be74df59a3a9782b9
 *
 * \note key=0 is illegal.
 *
 * \author Brian Paul
 */


#include "hash.h"

#include <stdio.h>
#include <stdlib.h>

#define HASH_FUNC(K)  ((K) % TABLE_SIZE)

/**
 * An entry in the hash table.
 */
struct HashEntry {
    GLuint Key;             /**< the entry's key */
    void *Data;             /**< the entry's data */
    struct HashEntry *Next; /**< pointer to next entry */
};

/**
 * Create a new hash table.
 *
 * \return pointer to a new, empty hash table.
 */
HashTable *
NewHashTable (void)
{
    HashTable *table = (HashTable *)
        calloc (1, sizeof (HashTable));
    if (table) {
        mutex_init (table->Mutex);
        mutex_init (table->WalkMutex);
    }

    return table;
}



/**
 * Delete a hash table.
 * Frees each entry on the hash table and then the hash table structure itself.
 * Note that the caller should have already traversed the table and deleted
 * the objects in the table (i.e. We don't free the entries' data pointer).
 *
 * \param table the hash table to delete.
 */
void
DeleteHashTable (HashTable *table)
{
    GLuint pos;
    assert (table);
    for (pos = 0; pos < TABLE_SIZE; pos++) {
        struct HashEntry *entry = table->Table[pos];
        while (entry) {
            struct HashEntry *next = entry->Next;
            free (entry);
            entry = next;
        }
    }
    mutex_destroy (table->Mutex);
    mutex_destroy (table->WalkMutex);
    free (table);
}



/**
 * Lookup an entry in the hash table, without locking.
 * \sa HashLookup
 */
static inline void *
HashLookup_unlocked (HashTable *table, GLuint key)
{
    GLuint pos;
    const struct HashEntry *entry;

    assert (table);
    assert (key);

    pos = HASH_FUNC (key);
    entry = table->Table[pos];
    while (entry) {
        if (entry->Key == key) {
            return entry->Data;
        }
        entry = entry->Next;
    }
    return NULL;
}


/**
 * Lookup an entry in the hash table.
 *
 * \param table the hash table.
 * \param key the key.
 *
 * \return pointer to user's data or NULL if key not in table
 */
void *
HashLookup (HashTable *table, GLuint key)
{
    void *res;
    assert (table);
    mutex_lock (table->Mutex);
    res = HashLookup_unlocked (table, key);
    mutex_unlock (table->Mutex);
    return res;
}


/**
 * Insert a key/pointer pair into the hash table.
 * If an entry with this key already exists we'll replace the existing entry.
 *
 * \param table the hash table.
 * \param key the key (not zero).
 * \param data pointer to user data.
 */
void
HashInsert (HashTable *table, GLuint key, void *data)
{
    /* search for existing entry with this key */
    GLuint pos;
    struct HashEntry *entry;

    assert (table);
    assert (key);

    mutex_lock (table->Mutex);

    if (key > table->MaxKey)
        table->MaxKey = key;

    pos = HASH_FUNC (key);

    /* Check if replacing an existing entry with same key. */
    for (entry = table->Table[pos]; entry; entry = entry->Next) {
        if (entry->Key == key) {
            /* Replace entry's data. */
#if 0 /* not sure this check is always valid */
            /* In DeleteHashTable, found non-freed data. */
            assert (!entry->Data)
#endif
                entry->Data = data;
            mutex_unlock (table->Mutex);
            return;
        }
    }

    /* Alloc and insert new table entry. */
    entry = (struct HashEntry *) malloc (sizeof (struct HashEntry));
    if (entry) {
        entry->Key = key;
        entry->Data = data;
        entry->Next = table->Table[pos];
        table->Table[pos] = entry;
    }

    mutex_unlock (table->Mutex);
}



/**
 * Remove an entry from the hash table.
 *
 * \param table the hash table.
 * \param key key of entry to remove.
 *
 * While holding the hash table's lock, searches the entry with the matching
 * key and unlinks it.
 */
void
HashRemove (HashTable *table, GLuint key)
{
    GLuint pos;
    struct HashEntry *entry, *prev;

    assert (table);
    assert (key);

    /* Have to check this outside of mutex lock. */
    /* HashRemove illegally called from HashDeleteAll callback function */
    assert (!table->InDeleteAll);

    mutex_lock (table->Mutex);

    pos = HASH_FUNC (key);
    prev = NULL;
    entry = table->Table[pos];
    while (entry) {
        if (entry->Key == key) {
            /* found it! */
            if (prev) {
                prev->Next = entry->Next;
            }
            else {
                table->Table[pos] = entry->Next;
            }
            free (entry);
            mutex_unlock (table->Mutex);
            return;
        }
        prev = entry;
        entry = entry->Next;
    }

    mutex_unlock (table->Mutex);
}



/**
 * Delete all entries in a hash table, but don't delete the table itself.
 * Invoke the given callback function for each table entry.
 *
 * \param table  the hash table to delete
 * \param callback  the callback function
 * \param userData  arbitrary pointer to pass along to the callback
 *                  (this is typically a struct gl_context pointer)
 */
void
HashDeleteAll (HashTable *table,
               void (*callback)(GLuint key, void *data, void *userData),
               void *userData)
{
    GLuint pos;
    assert (table);
    assert (callback);
    mutex_lock (table->Mutex);
    table->InDeleteAll = GL_TRUE;
    for (pos = 0; pos < TABLE_SIZE; pos++) {
        struct HashEntry *entry, *next;
        for (entry = table->Table[pos]; entry; entry = next) {
            callback (entry->Key, entry->Data, userData);
            next = entry->Next;
            free (entry);
        }
        table->Table[pos] = NULL;
    }
    table->InDeleteAll = GL_FALSE;
    mutex_unlock (table->Mutex);
}


/**
 * Walk over all entries in a hash table, calling callback function for each.
 * Note: we use a separate mutex in this function to avoid a recursive
 * locking deadlock (in case the callback calls HashRemove ()) and to
 * prevent multiple threads/contexts from getting tangled up.
 * A lock-less version of this function could be used when the table will
 * not be modified.
 * \param table  the hash table to walk
 * \param callback  the callback function
 * \param userData  arbitrary pointer to pass along to the callback
 *                  (this is typically a struct gl_context pointer)
 */
void
HashWalk (const HashTable *table,
          void (*callback)(GLuint key, void *data, void *userData),
          void *userData)
{
    /* cast-away const */
    HashTable *table2 = (HashTable *) table;
    GLuint pos;
    assert (table);
    assert (callback);
    mutex_lock (table2->WalkMutex);
    for (pos = 0; pos < TABLE_SIZE; pos++) {
        struct HashEntry *entry, *next;
        for (entry = table->Table[pos]; entry; entry = next) {
            /* save 'next' pointer now in case the callback deletes the entry */
            next = entry->Next;
            callback (entry->Key, entry->Data, userData);
        }
    }
    mutex_unlock (table2->WalkMutex);
}


/**
 * Return the key of the "first" entry in the hash table.
 * While holding the lock, walks through all table positions until finding
 * the first entry of the first non-empty one.
 *
 * \param table  the hash table
 * \return key for the "first" entry in the hash table.
 */
GLuint
HashFirstEntry (HashTable *table)
{
    GLuint pos;
    assert (table);
    mutex_lock (table->Mutex);
    for (pos = 0; pos < TABLE_SIZE; pos++) {
        if (table->Table[pos]) {
            mutex_unlock (table->Mutex);
            return table->Table[pos]->Key;
        }
    }
    mutex_unlock (table->Mutex);
    return 0;
}


/**
 * Given a hash table key, return the next key.  This is used to walk
 * over all entries in the table.  Note that the keys returned during
 * walking won't be in any particular order.
 * \return next hash key or 0 if end of table.
 */
GLuint
HashNextEntry (const HashTable *table, GLuint key)
{
    const struct HashEntry *entry;
    GLuint pos;

    assert (table);
    assert (key);

    /* Find the entry with given key */
    pos = HASH_FUNC (key);
    for (entry = table->Table[pos]; entry ; entry = entry->Next) {
        if (entry->Key == key) {
            break;
        }
    }

    if (!entry) {
        /* the given key was not found, so we can't find the next entry */
        return 0;
    }

    if (entry->Next) {
        /* return next in linked list */
        return entry->Next->Key;
    }
    else {
        /* look for next non-empty table slot */
        pos++;
        while (pos < TABLE_SIZE) {
            if (table->Table[pos]) {
                return table->Table[pos]->Key;
            }
            pos++;
        }
        return 0;
    }
}


/**
 * Dump contents of hash table for debugging.
 *
 * \param table the hash table.
 */
void
HashPrint (const HashTable *table)
{
    GLuint pos;
    assert (table);
    for (pos = 0; pos < TABLE_SIZE; pos++) {
        const struct HashEntry *entry = table->Table[pos];
        while (entry) {
            fprintf (stderr, "%u %p\n", entry->Key, entry->Data);
            entry = entry->Next;
        }
    }
}



/**
 * Find a block of adjacent unused hash keys.
 *
 * \param table the hash table.
 * \param numKeys number of keys needed.
 *
 * \return Starting key of free block or 0 if failure.
 *
 * If there are enough free keys between the maximum key existing in the table
 * (HashTable::MaxKey) and the maximum key possible, then simply return
 * the adjacent key. Otherwise do a full search for a free key block in the
 * allowable key range.
 */
GLuint
HashFindFreeKeyBlock (HashTable *table, GLuint numKeys)
{
    const GLuint maxKey = ~((GLuint) 0);
    mutex_lock (table->Mutex);
    if (maxKey - numKeys > table->MaxKey) {
        /* the quick solution */
        mutex_unlock (table->Mutex);
        return table->MaxKey + 1;
    }
    else {
        /* the slow solution */
        GLuint freeCount = 0;
        GLuint freeStart = 1;
        GLuint key;
        for (key = 1; key != maxKey; key++) {
            if (HashLookup_unlocked (table, key)) {
                /* darn, this key is already in use */
                freeCount = 0;
                freeStart = key+1;
            }
            else {
                /* this key not in use, check if we've found enough */
                freeCount++;
                if (freeCount == numKeys) {
                    mutex_unlock (table->Mutex);
                    return freeStart;
                }
            }
        }
        /* cannot allocate a block of numKeys consecutive keys */
        mutex_unlock (table->Mutex);
        return 0;
    }
}


/**
 * Return the number of entries in the hash table.
 */
GLuint
HashNumEntries (const HashTable *table)
{
    GLuint pos, count = 0;

    for (pos = 0; pos < TABLE_SIZE; pos++) {
        const struct HashEntry *entry;
        for (entry = table->Table[pos]; entry; entry = entry->Next) {
            count++;
        }
    }

    return count;
}



#if 1 /* debug only */

/**
 * Test walking over all the entries in a hash table.
 */
static void
test_hash_walking (void)
{
    HashTable *t = NewHashTable ();
    const GLuint limit = 50000;
    GLuint i;

    /* create some entries */
    for (i = 0; i < limit; i++) {
        GLuint dummy;
        GLuint k = (rand () % (limit * 10)) + 1;
        while (HashLookup (t, k)) {
            /* id already in use, try another */
            k = (rand () % (limit * 10)) + 1;
        }
        HashInsert (t, k, &dummy);
    }

    /* walk over all entries */
    {
        GLuint k = HashFirstEntry (t);
        GLuint count = 0;
        while (k) {
            GLuint knext = HashNextEntry (t, k);
            assert (knext != k);
            HashRemove (t, k);
            count++;
            k = knext;
        }
        assert (count == limit);
        k = HashFirstEntry (t);
        assert (k==0);
    }

    DeleteHashTable (t);
}


void
test_hash_functions (void)
{
    int a, b, c;
    HashTable *t;

    a = b = c = 403;

    t = NewHashTable ();
    HashInsert (t, 501, &a);
    HashInsert (t, 10, &c);
    HashInsert (t, 0xfffffff8, &b);
    HashPrint (t);

    assert (HashLookup (t,501));
    assert (!HashLookup (t,1313));
    assert (HashFindFreeKeyBlock (t, 100));

    DeleteHashTable (t);

    test_hash_walking ();
}

#endif
