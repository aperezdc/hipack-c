/*
 * hipack-parse.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "../hipack.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int
main (int argc, const char *argv[])
{
    if (argc != 2) {
        fprintf (stderr, "Usage: %s PATH\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *fp = fopen (argv[1], "rb");
    if (!fp) {
        fprintf (stderr, "%s: Cannot open '%s' (%s)\n",
                 argv[0], argv[1], strerror (errno));
        return EXIT_FAILURE;
    }

    int retcode = EXIT_SUCCESS;
    hipack_reader_t reader = {
        .getchar = hipack_stdio_getchar,
        .getchar_data = fp,
    };
    hipack_dict_t *message = hipack_read (&reader);
    if (!message) {
        assert (reader.error);
        fprintf (stderr, "line %u, column %u: %s\n",
                 reader.error_line, reader.error_column,
                 (reader.error == HIPACK_READ_ERROR)
                    ? strerror (errno) : reader.error);
        retcode = EXIT_FAILURE;
    }
    hipack_dict_free (message);

    fclose (fp);
    return retcode;
}

