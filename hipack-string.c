/*
 * hipack-string.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "hipack.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


static hipack_string_t s_empty_string = { .size = 0 };


hipack_string_t*
hipack_string_new_from_string (const char *str)
{
    assert (str);
    return hipack_string_new_from_lstring (str, strlen (str));
}


hipack_string_t*
hipack_string_new_from_lstring (const char *str, uint32_t len)
{
    assert (str);
    if (len > 0) {
        hipack_string_t *hstr = (hipack_string_t*) malloc (
            sizeof (hipack_list_t) + len * sizeof (uint8_t));
        memcpy (hstr->data, str, len);
        hstr->size = len;
        return hstr;
    } else {
        return &s_empty_string;
    }
}


hipack_string_t*
hipack_string_copy (const hipack_string_t *hstr)
{
    assert (hstr);

    if (hstr == &s_empty_string)
        return &s_empty_string;

    return hipack_string_new_from_lstring ((const char*) hstr->data,
                                           hstr->size);
}


void
hipack_string_free (hipack_string_t *hstr)
{
    if (hstr && hstr != &s_empty_string)
        free (hstr);
}


uint32_t
hipack_string_hash (const hipack_string_t *hstr)
{
    assert (hstr);

    uint32_t ret = 0;
    uint32_t ctr = 0;

    for (uint32_t i = 0; i < hstr->size; i++) {
        ret ^= hstr->data[i] << ctr;
        ctr  = (ctr + 1) % sizeof (void*);
    }

    return ret;
}


bool
hipack_string_equal (const hipack_string_t *hstr1,
                     const hipack_string_t *hstr2)
{
    assert (hstr1);
    assert (hstr2);

    return (hstr1->size == hstr2->size) &&
        memcmp (hstr1->data, hstr2->data, hstr1->size) == 0;
}
