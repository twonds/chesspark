/* utilhash.h
** libstrophe XMPP client library -- utilhash table interface
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

#ifndef __UTILHASH_H__
#define __UTILHASH_H__

typedef struct _utilhash_t utilhash_t;

typedef void (*utilhash_free_func)(void *p);

/** allocate and initialize a new utilhash table */
utilhash_t *utilhash_new(const int size, utilhash_free_func free);

/** allocate a new reference to an existing utilhash table */
utilhash_t *utilhash_clone(utilhash_t * const table);

/** release a utilhash table when no longer needed */
void utilhash_release(utilhash_t * const table);

/** add a key, value pair to a utilhash table.
 *  each key can appear only once; the value of any
 *  identical key will be replaced
 */
int utilhash_add(utilhash_t *table, const char * const key, void *data);

/** look up a key in a utilhash table */
void *utilhash_get(utilhash_t *table, const char *key);

/** delete a key from a utilhash table */
int utilhash_drop(utilhash_t *table, const char *key);

/** return the number of keys in a utilhash */
int utilhash_num_keys(utilhash_t *table);

/** utilhash key iterator functions */
typedef struct _utilhash_iterator_t utilhash_iterator_t;

/** allocate and initialize a new iterator */
utilhash_iterator_t *utilhash_iter_new(utilhash_t *table);

/** release an iterator that is no longer needed */
void utilhash_iter_release(utilhash_iterator_t *iter);

/** return the next utilhash table key from the iterator.
    the returned key should not be freed */
const char * utilhash_iter_next(utilhash_iterator_t *iter);

#endif /* __UTILHASH_H__ */
