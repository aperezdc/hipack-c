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


void
hipack_string_free (hipack_string_t *hstr)
{
    if (hstr && hstr != &s_empty_string)
        free (hstr);
}

