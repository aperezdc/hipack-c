/*
 * hipack-parser.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "hipack.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>


enum status {
    kStatusOk = 0,
    kStatusEof,
    kStatusError,
    kStatusIoError,
};
typedef enum status status_t;


struct parser {
    FILE       *fp;
    int         look;
    unsigned    line;
    unsigned    column;
    const char *error;
};

#define P struct parser* p
#define S status_t *status

#define CHECK_OK status); \
    if (*status != kStatusOk) return; \
    ((void) 0
#define DUMMY ) /* Makes autoindentation work. */
#undef DUMMY


static void parse_value(P, S);
static void parse_keyval_items (P, int eos, S);


static inline int
is_hipack_whitespace (int ch)
{
    switch (ch) {
        case 0x09: /* Horizontal tab. */
        case 0x0A: /* New line. */
        case 0x0D: /* Carriage return. */
        case 0x20: /* Space. */
            return 1;
        default:
            return 0;
    }
}



static inline int
xdigit_to_int (int xdigit)
{
    assert ((xdigit >= '0' && xdigit <= '9') ||
            (xdigit >= 'A' && xdigit <= 'F') ||
            (xdigit >= 'a' && xdigit <= 'f'));

    switch (xdigit) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a': case 'A': return 0xA;
        case 'b': case 'B': return 0xB;
        case 'c': case 'C': return 0xC;
        case 'd': case 'D': return 0xD;
        case 'e': case 'E': return 0xE;
        case 'f': case 'F': return 0xF;
        default: abort ();
    }
}



static inline void
nextchar (P, S)
{
    do {
        p->look = fgetc (p->fp);

        if (p->look == '\n') {
            p->column = 0;
            p->line++;
        }
        p->column++;

        if (p->look == '#') {
            while ((p->look = fgetc (p->fp)) != '\n' &&
                   (p->look != EOF)) /* noop */;

            if (p->look != EOF) {
                ungetc ('\n', p->fp);
            }
        }
    } while (p->look != EOF && p->look == '#');

    *status = (p->look == EOF)
        ? (ferror (p->fp) ? kStatusIoError : kStatusEof)
        : kStatusOk;
}


static inline void
skipwhite (P, S)
{
    while (p->look != EOF && is_hipack_whitespace (p->look)) {
        nextchar (p, CHECK_OK);
    }
    *status = (p->look == EOF)
        ? (ferror (p->fp) ? kStatusIoError : kStatusEof)
        : kStatusOk;
}


static inline void
matchchars (P, const char *chars, S)
{
    assert (chars != NULL);

    while (*chars) {
        if (p->look == EOF) {
            *status = ferror (p->fp) ? kStatusIoError : kStatusEof;
            return;
        }
        if (p->look != *chars) {
            *status = kStatusError;
            return;
        }
        p->look = fgetc (p->fp);
        chars++;
    }
}


static inline void
matchchar (P, int ch, const char *errmsg, S)
{
    if (p->look == ch) {
        nextchar (p, CHECK_OK);
        return;
    }

    p->error = errmsg ? errmsg : "unexpected input";
    *status = kStatusError;
}


static void
parse_key (P, S)
{
    while (p->look != EOF &&
           !is_hipack_whitespace (p->look) &&
           p->look != ':') {
        nextchar (p, CHECK_OK);
    }
}


static void
parse_string (P, S)
{
    int ch, extra;

    matchchar (p, '"', NULL, CHECK_OK);

    for (ch = fgetc (p->fp); ch != EOF; ch = fgetc (p->fp)) {
        /* Handle escapes. */
        if (ch == '\\') {
            switch ((ch = fgetc (p->fp))) {
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;
                default: /* Hex number. */
                    extra = fgetc (p->fp);
                    if (!isxdigit (extra) || !isxdigit (ch)) {
                        if (extra == EOF && ferror (p->fp)) {
                            *status = kStatusIoError;
                        } else {
                            p->error = "invalid escape sequence";
                            *status = kStatusError;
                        }
                        return;
                    }
                    ch = (xdigit_to_int (ch) * 16) + xdigit_to_int (extra);
                    break;
            }
        }
        /* TODO: Save "ch". */
    }

    matchchar (p, '"', "unterminated  string value", CHECK_OK);
}


static void
parse_list (P, S)
{
    matchchar (p, '[', NULL, CHECK_OK);
    skipwhite (p, CHECK_OK);

    while (p->look != ']') {
        parse_value (p, CHECK_OK);
        bool got_whitespace = is_hipack_whitespace (p->look);
        skipwhite (p, CHECK_OK);

        /* There must either a comma or whitespace after the value. */
        if (p->look == ',') {
            nextchar (p, CHECK_OK);
        } else if (!got_whitespace && !is_hipack_whitespace (p->look)) {
            break;
        }
        skipwhite (p, CHECK_OK);
    }

    matchchar (p, ']', "unterminated list value", CHECK_OK);
}


static void
parse_dict (P, S)
{
    matchchar (p, '{', NULL, CHECK_OK);
    parse_keyval_items (p, '}', CHECK_OK);
    matchchar (p, '}', "unterminated dict value", CHECK_OK);
}


static int
parse_bool (P, S)
{
    const char *match = NULL;
    int retval = 0;

    switch (p->look) {
        case 'T': match = "True"; retval = 1; break;
        case 't': match = "true"; retval = 1; break;
        case 'F': match = "False"; break;
        case 'f': match = "false"; break;
    }

    if (match) {
        matchchars (p, match, status);
        if (*status == kStatusOk)
            return retval;
    }

    p->error = "boolean value expected (true/false)";
    return 0;
}


static void
parse_number (P, S)
{
    /* TODO */
}


static void
parse_value (P, S)
{
    switch (p->look) {
        case '"': /* String */
            parse_string (p, CHECK_OK);
            break;

        case '[': /* List */
            parse_list (p, CHECK_OK);
            break;

        case '{': /* Dict */
            parse_dict (p, CHECK_OK);
            break;

        case 'T': /* Bool */
        case 't':
        case 'F':
        case 'f':
            parse_bool (p, CHECK_OK);
            break;

        default: /* Integer or Float */
            parse_number (p, CHECK_OK);
            break;
    }
}


static void
parse_keyval_items (P, int eos, S)
{
    while (p->look != eos) {
        parse_key (p, CHECK_OK);

        /* There must either a colon or whitespace before the value. */
        if (p->look == ':') {
            nextchar (p, CHECK_OK);
        } else if (!is_hipack_whitespace (p->look)) {
            p->error = "whitespace or colon expected";
            *status = kStatusError;
        }
        skipwhite (p, CHECK_OK);
        parse_value (p, CHECK_OK);
    }
}


static void
parse_message (P, S)
{
    nextchar (p, CHECK_OK);
    skipwhite (p, CHECK_OK);

    if (p->look == '{') {
        /* Input starts with a Dict marker. */
        nextchar (p, CHECK_OK);
        skipwhite (p, CHECK_OK);
        parse_keyval_items (p, '}', CHECK_OK);
        matchchar (p, '}', "unterminated message", CHECK_OK);
    } else {
        parse_keyval_items (p, EOF, CHECK_OK);
    }
}


int
hipack_read (FILE *fp)
{
    assert (fp);

    status_t status = kStatusOk;
    struct parser p = {
        .fp = fp,
        .line = 1,
        0,
    };

    parse_message (&p, &status);
    p.fp = NULL;

    return 0;
}


