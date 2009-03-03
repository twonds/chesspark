/* utilhash.c
** libstrophe XMPP client library -- utilhash table implementation
** 
** Copyright (C) 2005 OGG, LCC. All rights reserved.
**
**  This software is provided AS-IS with no warranty, either express
**  or implied.
**
**  This software is distributed under license and may not be copied,
**  modified or distributed except as expressly authorized under the
**  terms of the license contained in the file LICENSE.txt in this
**  distribution.
*/

#include <stdlib.h>
#include <string.h>

#include "utilhash.h"

/* private types */
typedef struct _utilhashentry_t utilhashentry_t;

struct _utilhashentry_t {
    utilhashentry_t *next;
    char *key;
    void *value;
};

struct _utilhash_t {
    unsigned int ref;
    utilhash_free_func free;
    int length;
    int num_keys;
    utilhashentry_t **entries;
};

struct _utilhash_iterator_t {
    unsigned int ref;
    utilhash_t *table;
    utilhashentry_t *entry;
    int index;
};
   
/** allocate and initialize a new utilhash table */
utilhash_t *utilhash_new(const int size, utilhash_free_func free)
{
    utilhash_t *result = NULL;

    result = malloc(sizeof(utilhash_t));
    if (result != NULL) {
	result->entries = malloc(size * sizeof(utilhashentry_t *));
	if (result->entries == NULL) {
	    free(result);
	    return NULL;
	}
	memset(result->entries, 0, size * sizeof(utilhashentry_t *));
	result->length = size;

	result->free = free;
	result->num_keys = 0;
	/* give the caller a reference */
	result->ref = 1;
    }
    
    return result;
}

/** obtain a new reference to an existing utilhash table */
utilhash_t *utilhash_clone(utilhash_t * const table)
{
    table->ref++;
    return table;
}

/** release a utilhash table that is no longer needed */
void utilhash_release(utilhash_t * const table)
{
    utilhashentry_t *entry, *next;
    int i;
    
    if (table->ref > 1)
	table->ref--;
    else {
	for (i = 0; i < table->length; i++) {
	    entry = table->entries[i];
	    while (entry != NULL) {
		next = entry->next;
		free(entry->key);
		if (table->free) table->free(entry->value);
		free(entry);
		entry = next;
	    }
	}
	free(table->entries);
	free(table);
    }
}

/** utilhash a key for our table lookup */
static int _utilhash_key(utilhash_t *table, const char *key)
{
   int utilhash = 0;
   int shift = 0;
   const char *c = key;

   while (*c != '\0') {
	/* assume 32 bit ints */
	utilhash ^= ((int)*c++ << shift);
	shift += 8;
	if (shift > 24) shift = 0;
   }

   return utilhash % table->length;
}

/** add a key, value pair to a utilhash table.
 *  each key can appear only once; the value of any
 *  identical key will be replaced
 */
int utilhash_add(utilhash_t *table, const char * const key, void *data)
{
   utilhashentry_t *entry = NULL;
   int index = _utilhash_key(table, key);

   /* allocate and fill a new entry */
   entry = malloc(sizeof(utilhashentry_t));
   if (!entry) return -1;
   entry->key = strdup(key);
   if (!entry->key) {
       free(entry);
       return -1;
   }
   entry->value = data;
   /* insert ourselves in the linked list */
   /* TODO: this leaks duplicate keys */
   entry->next = table->entries[index];
   table->entries[index] = entry;
   table->num_keys++;

   return 0;
}

/** look up a key in a utilhash table */
void *utilhash_get(utilhash_t *table, const char *key)
{
   utilhashentry_t *entry;
   int index = _utilhash_key(table, key);
   void *result = NULL;

   /* look up the utilhash entry */
   entry = table->entries[index];
   while (entry != NULL) {
	/* traverse the linked list looking for the key */
	if (!strcmp(key, entry->key)) {
	  /* match */
	  result = entry->value;
	  return result;
	}
	entry = entry->next;
   }
   /* no match */
   return result;
}

/** delete a key from a utilhash table */
int utilhash_drop(utilhash_t *table, const char *key)
{
   utilhashentry_t *entry, *prev;
   int index = _utilhash_key(table, key);

   /* look up the utilhash entry */
   entry = table->entries[index];
   prev = NULL;
   while (entry != NULL) {
	/* traverse the linked list looking for the key */
	if (!strcmp(key, entry->key)) {
	  /* match, remove the entry */
	  free(entry->key);
	  if (table->free) table->free(entry->value);
	  if (prev == NULL) {
	    table->entries[index] = entry->next;
	  } else {
	    prev->next = entry->next;
	  }
	  free(entry);
	  table->num_keys--;
	  return 0;
	}
	prev = entry;
	entry = entry->next;
   }
   /* no match */
   return -1;
}

int utilhash_num_keys(utilhash_t *table)
{
    return table->num_keys;
}

/** allocate and initialize a new iterator */
utilhash_iterator_t *utilhash_iter_new(utilhash_t *table)
{
    utilhash_iterator_t *iter;

    iter = malloc(sizeof(*iter));
    if (iter != NULL) {
	iter->ref = 1;
	iter->table = utilhash_clone(table);
	iter->entry = NULL;
	iter->index = -1;
    }

    return iter;
}


/** release an iterator that is no longer needed */
void utilhash_iter_release(utilhash_iterator_t *iter)
{
    iter->ref--;

    if (iter->ref <= 0) {
	utilhash_release(iter->table);
	free(iter);
    }
}

/** return the next utilhash table key from the iterator.
    the returned key should not be freed */
const char * utilhash_iter_next(utilhash_iterator_t *iter)
{
    utilhash_t *table = iter->table;
    utilhashentry_t *entry = iter->entry;
    int i = iter->index + 1;

    /* advance until we find the next entry */
    if (entry != NULL) entry = entry->next;
    if (entry == NULL) {
	/* we're off the end of list, search for a new entry */
	while (i < iter->table->length) {
	    entry = table->entries[i];
	    if (entry != NULL) {
		iter->index = i;
		break;
	    }
	    i++;
	}
    }

    if ((entry == NULL) || (i >= table->length)) {
	/* no more keys! */
	return NULL;
    }

    /* remember our current match */
    iter->entry = entry;
    return entry->key;
}

/* Wrapper functions, so we can treat a utilhash like a namedlist */
#if 0
utilhash_t **NamedHash_Add(utilhash_t **hash, char *name, void *data, void (*destroydata)(void *))
{
}

utilhash_t **NamedHash_AddToTop(utilhash_t **hash, char *name, void *data, void (*destroydata)(void *))
{
	NamedHash_Add(hash, name, data, destroydata);
}

utilhash_t **NamedHash_GetByName(utilhash_t **hash, char *name)
{
}

void NamedHash_Remove(utilhash_t **hash)
{
}

void NamedHash_Destroy(utilhash_t **hash)
{
}
#endif