/*
 * hipack-dict.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "hipack.h"
#include <stdlib.h>
#include <string.h>


#ifndef HIPACK_DICT_DEFAULT_SIZE
#define HIPACK_DICT_DEFAULT_SIZE 16
#endif /* !HIPACK_DICT_DEFAULT_SIZE */

#ifndef HIPACK_DICT_RESIZE_FACTOR
#define HIPACK_DICT_RESIZE_FACTOR 3
#endif /* !HIPACK_DICT_RESIZE_FACTOR */

#ifndef HIPACK_DICT_COUNT_TO_SIZE_RATIO
#define HIPACK_DICT_COUNT_TO_SIZE_RATIO 1.2
#endif /* !HIPACK_DICT_COUNT_TO_SIZE_RATIO */


struct hipack_dict_node {
    hipack_value_t      value;
    hipack_string_t    *key;
    hipack_dict_node_t *next;
    hipack_dict_node_t *next_node;
    hipack_dict_node_t *prev_node;
};


static inline hipack_dict_node_t*
make_node (hipack_string_t *key,
           const hipack_value_t  *value)
{
    hipack_dict_node_t *node = (hipack_dict_node_t*)
        malloc (sizeof (hipack_dict_node_t));
    memcpy (&node->value, value, sizeof (hipack_value_t));
    node->key = key;
    return node;
}


static inline void
free_node (hipack_dict_node_t *node)
{
    hipack_string_free (node->key);
    hipack_value_free (&node->value);
    free (node);
}


static void
free_all_nodes (hipack_dict_t *dict)
{
    hipack_dict_node_t *next;

    for (hipack_dict_node_t *node = dict->first; node; node = next) {
        next = node->next_node;
        free_node (node);
    }
}


static inline void
rehash (hipack_dict_t *dict)
{
    for (hipack_dict_node_t *node = dict->first; node; node = node->next_node)
        node->next = NULL;

    dict->size *= HIPACK_DICT_RESIZE_FACTOR;
    dict->nodes = (hipack_dict_node_t**)
        realloc (dict->nodes, sizeof (hipack_dict_node_t*) * dict->size);
    memset (dict->nodes, 0x00, sizeof (hipack_dict_node_t*) * dict->size);

    for (hipack_dict_node_t *node = dict->first; node; node = node->next_node) {
        uint32_t hash_val = hipack_string_hash (node->key) % (dict->size - 1);
        hipack_dict_node_t *n = dict->nodes[hash_val];
        if (!n) {
            dict->nodes[hash_val] = node;
        } else {
            for (;; n = n->next) {
                if (!n->next) {
                    n->next = node;
                    break;
                }
            }
        }
    }
}


hipack_dict_t*
hipack_dict_new (void)
{
    hipack_dict_t *dict = (hipack_dict_t*)
        malloc (sizeof (hipack_dict_t));
    dict->count = 0;
    dict->size  = HIPACK_DICT_DEFAULT_SIZE;
    dict->nodes = (hipack_dict_node_t**)
        malloc (sizeof (hipack_dict_node_t*) * dict->size);
    return dict;
}


void
hipack_dict_free (hipack_dict_t *dict)
{
    if (dict) {
        free_all_nodes (dict);
        free (dict);
    }
}


void
hipack_dict_set (hipack_dict_t         *dict,
                 const hipack_string_t *key,
                 const hipack_value_t  *value)
{
    hipack_string_t *key_copy = hipack_string_copy (key);
    hipack_dict_set_adopt_key (dict, &key_copy, value);
}


void hipack_dict_set_adopt_key (hipack_dict_t        *dict,
                                hipack_string_t     **key,
                                const hipack_value_t *value)
{
    assert (dict);
    assert (key);
    assert (*key);
    assert (value);

    uint32_t hash_val = hipack_string_hash (*key) % (dict->size - 1);
    hipack_dict_node_t *node = dict->nodes[hash_val];

    for (; node; node = node->next) {
        if (hipack_string_equal (node->key, *key)) {
            hipack_value_free (&node->value);
            hipack_string_free (*key);
            memcpy (&node->value, value, sizeof (hipack_value_t));
            *key = NULL;
            return;
        }
    }

    node = make_node (*key, value);
    *key = NULL;

    if (dict->nodes[hash_val]) {
        node->next = dict->nodes[hash_val];
    }
    dict->nodes[hash_val] = node;

    node->next_node = dict->first;
    if (dict->first) {
        dict->first->prev_node = node;
    }
    dict->first = node;
    dict->count++;

    if (dict->count > (dict->size * HIPACK_DICT_COUNT_TO_SIZE_RATIO)) {
        rehash (dict);
    }
}


bool
hipack_dict_get (const hipack_dict_t   *dict,
                 const hipack_string_t *key,
                 hipack_value_t        *value)
{
    assert (dict);
    assert (key);
    /* TODO */
    return false;
}
