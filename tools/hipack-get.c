/*
 * hipack-get.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "../hipack.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>


/* Make sure that strtonum() is defined as "static". */
static long long strtonum (const char*, long long, long long, const char**);
#include "strtonum.c"


int
main (int argc, const char *argv[])
{
    if (argc < 2) {
        fprintf (stderr, "Usage: %s <-|PATH> [key...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    bool use_stdin = argv[1][0] == '-' && argv[1][1] == '\0';
    FILE *fp = use_stdin ? stdin : fopen (argv[1], "rb");
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
    fclose (fp);

    if (retcode != EXIT_SUCCESS)
        return retcode;

    hipack_value_t *value = &((hipack_value_t) {
        .type   = HIPACK_DICT,
        .v_dict = message
    });

    for (unsigned i = 2; i < argc && value; i++) {
        switch (hipack_value_type (value)) {
            case HIPACK_DICT: {
                /* Use argv[i] as dictionary key. */
                hipack_string_t *key = hipack_string_new_from_string (argv[i]);
                value = hipack_dict_get (value->v_dict, key);
                hipack_string_free (key);
                break;
            }
            case HIPACK_LIST: {
                /* Use argv[i] as a list index. */
                const char *error = NULL;
                long long index = strtonum (argv[i],
                                            0,
                                            hipack_list_size (value->v_list),
                                            &error);
                if (error) {
                    fprintf (stderr, "%s: number '%s' is %s\n",
                             argv[0], argv[i], error);
                    retcode = EXIT_FAILURE;
                    goto cleanup_dict;
                }
                value = HIPACK_LIST_AT (value->v_list, index);
                break;
            }
            default:
                fprintf (stderr, "%s: value is not a list or dictionary\n",
                         argv[0]);
                retcode = EXIT_FAILURE;
                goto cleanup_dict;
        }
    }

    if (value) {
        hipack_writer_t writer = {
            .putchar = hipack_stdio_putchar,
            .putchar_data = stdout,
        };
        if (hipack_write_value (&writer, value)) {
            fprintf (stderr, "%s: write error (%s)\n",
                     argv[0], strerror (errno));
            retcode = EXIT_FAILURE;
        }
        putchar ('\n');
    } else {
        fprintf (stderr, "%s: No value for the specified key.\n", argv[0]);
        retcode = EXIT_FAILURE;
    }

cleanup_dict:
    hipack_dict_free (message);
    return retcode;
}
