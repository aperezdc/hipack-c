/*
 * hipack-dict.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "hipack.h"


hipack_dict_t*
hipack_dict_new (void)
{
    /* TODO */
    return (hipack_dict_t*) 0x1;
}


void
hipack_dict_free (hipack_dict_t *dict)
{
    /* TODO */
}


void
hipack_dict_set (hipack_type_t         *dict,
                 const hipack_string_t *key,
                 const hipack_value_t  *value)
{
    assert (dict);
    assert (key);
    assert (value);
    /* TODO */
}


void hipack_dict_set_adopt_key (hipack_dict_t        *dict,
                                hipack_string_t     **key,
                                const hipack_value_t *value)
{
    assert (dict);
    assert (key);
    assert (*key);
    assert (value);

    /* TODO */
    hipack_string_free (*key);
    *key = NULL;
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
