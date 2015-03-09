/*
 * hipack-string.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "hipack.h"
#include <stdlib.h>


void
hipack_string_free (hipack_string_t *hstr)
{
    if (hstr)
        free (hstr);
}

