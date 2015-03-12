/*
 * hipack-alloc.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "hipack.h"
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>


void* (*hipack_alloc) (void*, size_t) = hipack_alloc_stdlib;


void*
hipack_alloc_stdlib (void *optr, size_t size)
{
    if (size) {
        optr = optr ?  realloc (optr, size) : malloc (size);
        if (!optr) {
            fprintf (stderr, "aborted: %s\n", strerror (errno));
            fflush (stderr);
            abort ();
        }
    } else {
        free (optr);
        optr = NULL;
    }
    return optr;
}


/*
 * This is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
 * if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
 */
#define MUL_NO_OVERFLOW	((size_t)1 << (sizeof(size_t) * 4))

void*
hipack_alloc_array_extra (void *optr, size_t nmemb, size_t size, size_t extra)
{
	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
        nmemb > 0 && SIZE_MAX / nmemb < size) {
        fprintf (stderr, "aborted: %s\n", strerror (ENOMEM));
        fflush (stderr);
        abort ();
    }
	return (*hipack_alloc) (optr, size * nmemb + extra);
}


