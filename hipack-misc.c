/*
 * hipack-misc.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "hipack.h"
#include <math.h>


bool
hipack_value_equal (const hipack_value_t *a,
                    const hipack_value_t *b)
{
    assert (a);
    assert (b);

    if (a->type != b->type)
        return false;

    switch (a->type) {
        case HIPACK_INTEGER:
            return a->v_integer == b->v_integer;
        case HIPACK_BOOL:
            return a->v_bool == b->v_bool;
        case HIPACK_FLOAT:
            return fabs(b->v_float - a->v_float) < 1e-15;
        case HIPACK_STRING:
            return hipack_string_equal (a->v_string, b->v_string);
        case HIPACK_LIST:
            return hipack_list_equal (a->v_list, b->v_list);
        case HIPACK_DICT:
            return hipack_dict_equal (a->v_dict, b->v_dict);
        default:
            assert (false); // Unreachable.
    }
}


bool
hipack_list_equal (const hipack_list_t *a,
                   const hipack_list_t *b)
{
    assert (a);
    assert (b);

    if (a->size != b->size)
        return false;

    for (uint32_t i = 0; i < a->size; i++)
        if (!hipack_value_equal (&a->data[i], &b->data[i]))
            return false;

    return true;
}


bool
hipack_dict_equal (const hipack_dict_t *a,
                   const hipack_dict_t *b)
{
    assert (a);
    assert (b);

    if (a->count != b->count)
        return false;

    const hipack_string_t *key;
    hipack_value_t *a_value;
    HIPACK_DICT_FOREACH (a, key, a_value) {
        hipack_value_t *b_value = hipack_dict_get (b, key);
        if (!b_value || !hipack_value_equal (a_value, b_value)) {
            return false;
        }
    }

    return true;
}



