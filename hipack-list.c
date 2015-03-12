/*
 * hipack-list.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "hipack.h"
#include <stdlib.h>


static hipack_list_t s_empty_list = { .size = 0 };


hipack_list_t*
hipack_list_new (uint32_t size)
{
    hipack_list_t *list;

    if (size) {
        list = hipack_alloc_array_extra (NULL, size,
                                         sizeof (hipack_value_t),
                                         sizeof (hipack_list_t));
        list->size = size;
    } else {
        list = &s_empty_list;
    }
    return list;
}


void
hipack_list_free (hipack_list_t *list)
{
    if (list && list != &s_empty_list) {
        for (uint32_t i = 0; i < list->size; i++)
            hipack_value_free (&list->data[i]);
        hipack_alloc_free (list);
    }
}

