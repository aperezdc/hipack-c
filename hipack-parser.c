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
#include <stdlib.h>
#include <ctype.h>


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
    if (*status != kStatusOk) goto error; \
    ((void) 0
#define DUMMY ) /* Makes autoindentation work. */
#undef DUMMY

#define DUMMY_VALUE ((hipack_value_t) { .type = HIPACK_BOOL })


static hipack_value_t parse_value(P, S);
static hipack_dict_t* parse_keyval_items (P, int eos, S);

static hipack_string_t empty_string = { .size = 0 };


static inline bool
is_hipack_whitespace (int ch)
{
    switch (ch) {
        case 0x09: /* Horizontal tab. */
        case 0x0A: /* New line. */
        case 0x0D: /* Carriage return. */
        case 0x20: /* Space. */
            return true;
        default:
            return false;
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

    if (p->look == EOF && ferror (p->fp))
        *status = kStatusIoError;
}


static inline void
skipwhite (P, S)
{
    while (p->look != EOF && is_hipack_whitespace (p->look))
        nextchar (p, status);

    if (p->look == EOF && ferror (p->fp))
        *status = kStatusIoError;
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

error:
    return;
}


#ifndef HIPACK_STRING_CHUNK_SIZE
#define HIPACK_STRING_CHUNK_SIZE 32
#endif /* !HIPACK_STRING_CHUNK_SIZE */

#ifndef HIPACK_STRING_POW_SIZE
#define HIPACK_STRING_POW_SIZE 512
#endif /* !HIPACK_STRING_POW_SIZE */


static hipack_string_t*
string_resize (hipack_string_t *hstr, uint32_t *alloc, uint32_t size)
{
    if (size) {
        uint32_t new_size = HIPACK_STRING_CHUNK_SIZE *
            ((size / HIPACK_STRING_CHUNK_SIZE) + 1);
        if (new_size < size) {
            new_size = size;
        }
        if (new_size != *alloc) {
            *alloc = new_size;
            new_size = sizeof (hipack_string_t) + new_size * sizeof (uint8_t);
            hstr = (hipack_string_t*)
                (hstr ? realloc (hstr, new_size) : malloc (new_size));
        }
        hstr->size = size;
    } else {
        *alloc = 0;
        free (hstr);
        hstr = NULL;
    }
    return hstr;
}


static hipack_string_t*
parse_key (P, S)
{
    hipack_string_t *hstr = NULL;
    uint32_t alloc_size = 0;
    uint32_t size = 0;

    while (p->look != EOF &&
           !is_hipack_whitespace (p->look) &&
           p->look != ':') {
        hstr = string_resize (hstr, &alloc_size, size + 1);
        hstr->data[size++] = p->look;
        nextchar (p, CHECK_OK);
    }

    return hstr ? hstr : &empty_string;

error:
    hipack_string_free (hstr);
    return NULL;
}


static hipack_value_t
parse_string (P, S)
{
    int ch, extra;
    hipack_string_t *hstr = NULL;
    uint32_t alloc_size = 0;
    uint32_t size = 0;

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
                        goto error;
                    }
                    ch = (xdigit_to_int (ch) * 16) + xdigit_to_int (extra);
                    break;
            }
        }

        hstr = string_resize (hstr, &alloc_size, size + 1);
        hstr->data[size++] = ch;
    }

    matchchar (p, '"', "unterminated  string value", CHECK_OK);
    return (hipack_value_t) {
        .type = HIPACK_STRING,
        .v_string = hstr ? hstr : &empty_string
    };

error:
    hipack_string_free (hstr);
    return DUMMY_VALUE;
}


static hipack_value_t
parse_list (P, S)
{
    hipack_list_t *result = hipack_list_new ();
    hipack_value_t value = DUMMY_VALUE;

    matchchar (p, '[', NULL, CHECK_OK);
    skipwhite (p, CHECK_OK);

    while (p->look != ']') {
        value = parse_value (p, CHECK_OK);
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
    return (hipack_value_t) { .type = HIPACK_LIST, .v_list = result };

error:
    hipack_value_free (&value);
    hipack_list_free (result);
    return DUMMY_VALUE;
}


static hipack_value_t
parse_dict (P, S)
{
    hipack_dict_t *dict = NULL;
    matchchar (p, '{', NULL, CHECK_OK);
    dict = parse_keyval_items (p, '}', CHECK_OK);
    matchchar (p, '}', "unterminated dict value", CHECK_OK);
    return (hipack_value_t) { .type = HIPACK_DICT, .v_dict = dict };

error:
    hipack_dict_free (dict);
    return DUMMY_VALUE;
}


static hipack_value_t
parse_bool (P, S)
{
    const char *match = NULL;
    bool retval = false;

    switch (p->look) {
        case 'T': match = "True"; retval = true; break;
        case 't': match = "true"; retval = true; break;
        case 'F': match = "False"; break;
        case 'f': match = "false"; break;
    }

    if (match) {
        matchchars (p, match, status);
        if (*status == kStatusOk) {
            return (hipack_value_t) { .type = HIPACK_BOOL, .v_bool = retval };
        }
    }

    p->error = "boolean value expected (true/false)";
    return DUMMY_VALUE;
}


static hipack_value_t
parse_number (P, S)
{
    /* TODO */
    return DUMMY_VALUE;
}


static hipack_value_t
parse_value (P, S)
{
    hipack_value_t result = DUMMY_VALUE;

    switch (p->look) {
        case '"': /* String */
            result = parse_string (p, CHECK_OK);
            break;

        case '[': /* List */
            result = parse_list (p, CHECK_OK);
            break;

        case '{': /* Dict */
            result = parse_dict (p, CHECK_OK);
            break;

        case 'T': /* Bool */
        case 't':
        case 'F':
        case 'f':
            result = parse_bool (p, CHECK_OK);
            break;

        default: /* Integer or Float */
            result = parse_number (p, CHECK_OK);
            break;
    }

error:
    return result;
}


static hipack_dict_t*
parse_keyval_items (P, int eos, S)
{
    hipack_dict_t *result = hipack_dict_new ();
    hipack_value_t value = DUMMY_VALUE;
    hipack_string_t *key = NULL;

    while (p->look != eos) {
        key = parse_key (p, CHECK_OK);

        /* There must either a colon or whitespace before the value. */
        if (p->look == ':') {
            nextchar (p, CHECK_OK);
        } else if (!is_hipack_whitespace (p->look)) {
            p->error = "whitespace or colon expected";
            *status = kStatusError;
        }
        skipwhite (p, CHECK_OK);
        value = parse_value (p, CHECK_OK);

        /* TODO: Put key/value in dictionary. */
        hipack_string_free (key); /* TODO: Put key/value in dictionary. */
        key = NULL;

        /*
         * There must be either a comma or a whitespace after the value,
         * or the end-of-sequence character.
         */
        if (p->look == ',') {
            nextchar (p, CHECK_OK);
        } else if (p->look != eos && !is_hipack_whitespace (p->look)) {
            break;
        }
        skipwhite (p, CHECK_OK);
    }

    return result;

error:
    hipack_string_free (key);
    hipack_value_free (&value);
    hipack_dict_free (result);
    return NULL;
}


static hipack_dict_t*
parse_message (P, S)
{
    hipack_dict_t *result = NULL;

    nextchar (p, CHECK_OK);
    skipwhite (p, CHECK_OK);

    if (p->look == EOF) {
        result = hipack_dict_new ();
    } else if (p->look == '{') {
        /* Input starts with a Dict marker. */
        nextchar (p, CHECK_OK);
        skipwhite (p, CHECK_OK);
        result = parse_keyval_items (p, '}', CHECK_OK);
        matchchar (p, '}', "unterminated message", CHECK_OK);
    } else {
        result = parse_keyval_items (p, EOF, CHECK_OK);
    }
    return result;

error:
    hipack_dict_free (result);
    return NULL;
}


hipack_dict_t*
hipack_read (FILE *fp)
{
    assert (fp);

    status_t status = kStatusOk;
    struct parser p = {
        .fp = fp,
        .line = 1,
        0,
    };

    hipack_dict_t *result = parse_message (&p, &status);
    /* TODO: Some post-processing of "status" */
    return result;
}


