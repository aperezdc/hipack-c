/*
 * hipack-roundtrip.c
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
    hipack_dict_t *message1 = hipack_read (&reader);
    hipack_dict_t *message2 = NULL;
    if (!message1) {
        assert (reader.error);
        fprintf (stderr, "[pass 1] line %u, column %u: %s\n",
                 reader.error_line, reader.error_column,
                 (reader.error == HIPACK_READ_ERROR)
                    ? strerror (errno) : reader.error);
        retcode = EXIT_FAILURE;
        goto cleanup;
    }

    fclose (fp);

    /* Create a temporary file and write the message to it. */
    fp = tmpfile ();
    hipack_writer_t writer = {
        .putchar = hipack_stdio_putchar,
        .putchar_data = fp,
        .indent = compact ? HIPACK_WRITER_COMPACT : HIPACK_WRITER_INDENTED,
    };

    if (hipack_write (&writer, message1)) {
        fprintf (stderr, "write error: %s\n", strerror (errno));
        retcode = EXIT_FAILURE;
        goto cleanup;
    }

    /* Re-read the dumped file. */
    rewind (fp);
    reader = (hipack_reader_t) {
        .getchar = hipack_stdio_getchar,
        .getchar_data = fp,
    };
    message2 = hipack_read (&reader);
    if (!message2) {
        assert (reader.error);
        fprintf (stderr, "[pass 2] line %u, column %u: %s\n",
                 reader.error_line, reader.error_column,
                 (reader.error == HIPACK_READ_ERROR)
                    ? strerror (errno) : reader.error);
        retcode = EXIT_FAILURE;
        goto cleanup;
    }

    if (!hipack_dict_equal (message1, message2)) {
        fprintf (stderr, "Messages are different\n");
        retcode = EXIT_FAILURE;
    }

cleanup:
    hipack_dict_free (message1);
    hipack_dict_free (message2);
    if (fp) fclose (fp);
    return retcode;
}

