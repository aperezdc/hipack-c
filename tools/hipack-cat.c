/*
 * hipack-cat.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#define _POSIX_C_SOURCE 2
#include "../hipack.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


static void
usage (const char *argv0, int code)
{
    FILE *output = (code == EXIT_FAILURE) ? stderr : stdout;
    fprintf (output, "Usage: %s [-c] PATH\n", argv0);
    exit (code);
}


int
main (int argc, char *argv[])
{
    bool compact = false;
    int opt;

    while ((opt = getopt (argc, argv, "hc")) != -1) {
        switch (opt) {
            case 'c':
                compact = true;
                break;
            case 'h':
                usage (argv[0], EXIT_SUCCESS);
                break;
            default:
                usage (argv[0], EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        usage (argv[0], EXIT_FAILURE);
    }

    FILE *fp = fopen (argv[optind], "rb");
    if (!fp) {
        fprintf (stderr, "%s: Cannot open '%s' (%s)\n",
                 argv[0], argv[optind], strerror (errno));
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
        goto cleanup;
    }

    hipack_writer_t writer = {
        .putchar = hipack_stdio_putchar,
        .putchar_data = stdout,
        .indent = compact ? HIPACK_WRITER_COMPACT : HIPACK_WRITER_INDENTED,
    };
    hipack_write (&writer, message);

cleanup:
    fclose (fp);
    hipack_dict_free (message);
    return retcode;
}

