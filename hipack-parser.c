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
#include <errno.h>


const char* HIPACK_READ_ERROR = "Error reading from input";


enum status {
    kStatusOk = 0,
    kStatusEof,
    kStatusError,
    kStatusIoError,
};
typedef enum status status_t;


struct parser {
    int       (*getchar) (void*);
    void       *getchar_data;
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

#define DUMMY_VALUE ((hipack_value_t) { .type = HIPACK_BOOL, .annot = NULL })


static hipack_value_t parse_value (P, S);
static void parse_keyval_items (P, hipack_dict_t *result, int eos, S);


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


static inline bool
is_hipack_key_character (int ch)
{
    switch (ch) {
        /* Keys do not contain whitespace */
        case 0x09: /* Horizontal tab. */
        case 0x0A: /* New line. */
        case 0x0D: /* Carriage return. */
        case 0x20: /* Space. */
        /* Characters are forbidden in keys by the spec. */
        case '[':
        case ']':
        case '{':
        case '}':
        case ':':
        case ',':
            return false;
        default:
            return true;
    }
}


static inline bool
is_number_char (int ch)
{
    switch (ch) {
        case '.': return true;
        case '+': return true;
        case '-': return true;
        case '0': return true;
        case '1': return true;
        case '2': return true;
        case '3': return true;
        case '4': return true;
        case '5': return true;
        case '6': return true;
        case '7': return true;
        case '8': return true;
        case '9': return true;
        case 'a': case 'A': return true;
        case 'b': case 'B': return true;
        case 'c': case 'C': return true;
        case 'd': case 'D': return true;
        case 'e': case 'E': return true;
        case 'f': case 'F': return true;
        default:
            return false;
    }
}


static inline bool
is_octal_nonzero_digit (int ch)
{
    return (ch > '0') && (ch < '8');
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


static inline int
nextchar_raw (P, S)
{
    int ch = (*p->getchar) (p->getchar_data);
    switch (ch) {
        case HIPACK_IO_ERROR:
            *status = kStatusIoError;
            /* fall-through */
        case HIPACK_IO_EOF:
            break;

        case '\n':
            p->column = 0;
            p->line++;
            /* fall-through */
        default:
            p->column++;
    }
    return ch;
}


static inline void
nextchar (P, S)
{
    do {
        p->look = nextchar_raw (p, CHECK_OK);

        if (p->look == '#') {
            while (p->look != '\n' && p->look != HIPACK_IO_EOF) {
                p->look = nextchar_raw (p, CHECK_OK);
            }
        }
    } while (p->look != HIPACK_IO_EOF && p->look == '#');

error:
    /* noop */;
}


static inline void
skipwhite (P, S)
{
    while (p->look != HIPACK_IO_EOF && is_hipack_whitespace (p->look))
        nextchar (p, status);
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

#ifndef HIPACK_LIST_CHUNK_SIZE
#define HIPACK_LIST_CHUNK_SIZE HIPACK_STRING_CHUNK_SIZE
#endif /* !HIPACK_LIST_CHUNK_SIZE */

#ifndef HIPACK_LIST_POW_SIZE
#define HIPACK_LIST_POW_SIZE HIPACK_STRING_POW_SIZE
#endif /* !HIPACK_LIST_POW_SIZE */


static hipack_string_t*
string_resize (hipack_string_t *hstr, uint32_t *alloc, uint32_t size)
{
    /* TODO: Use HIPACK_STRING_POW_SIZE. */
    if (size) {
        uint32_t new_size = HIPACK_STRING_CHUNK_SIZE *
            ((size / HIPACK_STRING_CHUNK_SIZE) + 1);
        if (new_size < size) {
            new_size = size;
        }
        if (new_size != *alloc) {
            *alloc = new_size;
            new_size = sizeof (hipack_string_t) + new_size * sizeof (uint8_t);
            hstr = hipack_alloc_array_extra (hstr, new_size,
                                             sizeof (uint8_t),
                                             sizeof (hipack_string_t));
        }
        hstr->size = size;
    } else {
        hipack_alloc_free (hstr);
        hstr = NULL;
        *alloc = 0;
    }
    return hstr;
}


static hipack_list_t*
list_resize (hipack_list_t *list, uint32_t *alloc, uint32_t size)
{
    /* TODO: Use HIPACK_LIST_POW_SIZE. */
    if (size) {
        uint32_t new_size = HIPACK_LIST_CHUNK_SIZE *
            ((size / HIPACK_LIST_CHUNK_SIZE) + 1);
        if (new_size < size) {
            new_size = size;
        }
        if (new_size != *alloc) {
            *alloc = new_size;
            list = hipack_alloc_array_extra (list, new_size,
                                             sizeof (hipack_value_t),
                                             sizeof (hipack_list_t));
        }
        list->size = size;
    } else {
        hipack_alloc_free (list);
        list = NULL;
        *alloc = 0;
    }
    return list;
}


/* On empty (missing) keys, NULL is returned. */
static hipack_string_t*
parse_key (P, S)
{
    hipack_string_t *hstr = NULL;
    uint32_t alloc_size = 0;
    uint32_t size = 0;

    while (p->look != HIPACK_IO_EOF && is_hipack_key_character (p->look)) {
        hstr = string_resize (hstr, &alloc_size, size + 1);
        hstr->data[size++] = p->look;
        nextchar (p, CHECK_OK);
    }

    return hstr;

error:
    hipack_string_free (hstr);
    return NULL;
}


static void
parse_string (P, hipack_value_t *result, S)
{
    hipack_string_t *hstr = NULL;
    uint32_t alloc_size = 0;
    uint32_t size = 0;

    matchchar (p, '"', NULL, CHECK_OK);

    while (p->look != '"' && p->look != HIPACK_IO_EOF) {
        /* Handle escapes. */
        if (p->look == '\\') {
            int extra;

            p->look = nextchar_raw (p, CHECK_OK);
            switch (p->look) {
                case '"' : p->look = '"' ; break;
                case 'n' : p->look = '\n'; break;
                case 'r' : p->look = '\r'; break;
                case 't' : p->look = '\t'; break;
                case '\\': p->look = '\\'; break;
                default:
                    /* Hex number. */
                    extra = nextchar_raw (p, CHECK_OK);
                    if (!isxdigit (extra) || !isxdigit (p->look)) {
                        p->error = "invalid escape sequence";
                        *status = kStatusError;
                        goto error;
                    }
                    p->look = (xdigit_to_int (p->look) * 16) +
                        xdigit_to_int (extra);
                    break;
            }
        }

        hstr = string_resize (hstr, &alloc_size, size + 1);
        hstr->data[size++] = p->look;

        /* Read next character from the string. */
        p->look = nextchar_raw (p, CHECK_OK);
    }

    matchchar (p, '"', "unterminated string value", CHECK_OK);
    result->type = HIPACK_STRING;
    result->v_string = hstr ? hstr : hipack_string_new_from_lstring ("", 0);
    return;

error:
    hipack_string_free (hstr);
    return;
}


static void
parse_list (P, hipack_value_t *result, S)
{
    hipack_list_t *list = NULL;
    uint32_t alloc_size = 0;
    uint32_t size = 0;

    matchchar (p, '[', NULL, CHECK_OK);
    skipwhite (p, CHECK_OK);

    while (p->look != ']') {
        hipack_value_t value = parse_value (p, CHECK_OK);
        list = list_resize (list, &alloc_size, size + 1);
        list->data[size++] = value;

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
    result->type = HIPACK_LIST;
    result->v_list = list;
    return;

error:
    hipack_list_free (list);
    return;
}


static void
parse_dict (P, hipack_value_t *result, S)
{
    hipack_dict_t *dict = hipack_dict_new ();
    matchchar (p, '{', NULL, CHECK_OK);
    skipwhite (p, CHECK_OK);
    parse_keyval_items (p, dict, '}', CHECK_OK);
    matchchar (p, '}', "unterminated dict value", CHECK_OK);
    result->type = HIPACK_DICT;
    result->v_dict = dict;
    return;

error:
    hipack_dict_free (dict);
    return;
}


static void
parse_bool (P, hipack_value_t *result, S)
{
    result->type = HIPACK_BOOL;
    if (p->look == 'T' || p->look == 't') {
        nextchar (p, CHECK_OK);
        matchchar (p, 'r', NULL, CHECK_OK);
        matchchar (p, 'u', NULL, CHECK_OK);
        matchchar (p, 'e', NULL, CHECK_OK);
        result->v_bool = true;
    } else if (p->look == 'F' || p->look == 'f') {
        nextchar (p, CHECK_OK);
        matchchar (p, 'a', NULL, CHECK_OK);
        matchchar (p, 'l', NULL, CHECK_OK);
        matchchar (p, 's', NULL, CHECK_OK);
        matchchar (p, 'e', NULL, CHECK_OK);
        result->v_bool = false;
    }
    return;

error:
    p->error = "invalid boolean value";
}


static void
parse_number (P, hipack_value_t *result, S)
{
    hipack_string_t *hstr = NULL;
    uint32_t alloc_size = 0;
    uint32_t size = 0;

#define SAVE_LOOK( ) \
    hstr = string_resize (hstr, &alloc_size, size + 1); \
    hstr->data[size++] = p->look

    /* Optional sign. */
    bool has_sign = false;
    if (p->look == '-' || p->look == '+') {
        SAVE_LOOK ();
        has_sign = true;
        nextchar (p, CHECK_OK);
    }

    /* Octal/hexadecimal numbers. */
    bool is_octal = false;
    bool is_hex = false;
    if (p->look == '0') {
        SAVE_LOOK ();
        nextchar (p, CHECK_OK);
        if (p->look == 'x' || p->look == 'X') {
            SAVE_LOOK ();
            nextchar (p, CHECK_OK);
            is_hex = true;
        } else if (is_octal_nonzero_digit (p->look)) {
            is_octal = true;
        }
    }

    /* Read the rest of the number. */
    bool dot_seen = false;
    bool exp_seen = false;
    while (p->look != HIPACK_IO_EOF && is_number_char (p->look)) {
        if (!is_hex && (p->look == 'e' || p->look == 'E')) {
            if (exp_seen || is_octal) {
                *status = kStatusError;
                goto error;
            }
            exp_seen = true;
            /* Handle the optional sign of the exponent. */
            SAVE_LOOK ();
            nextchar (p, CHECK_OK);
            if (p->look == '-' || p->look == '+') {
                SAVE_LOOK ();
                nextchar (p, CHECK_OK);
            }
        } else {
            if (p->look == '.') {
                if (dot_seen || is_hex || is_octal) {
                    *status = kStatusError;
                    goto error;
                }
                dot_seen = true;
            }
            if (p->look == '-' || p->look == '+') {
                *status = kStatusError;
                goto error;
            }
            SAVE_LOOK ();
            nextchar (p, CHECK_OK);
        }
    }

    if (!size) {
        *status = kStatusError;
        goto error;
    }

    /* Zero-terminate, to use with the libc conversion functions. */
    hstr = string_resize (hstr, &alloc_size, size + 1);
    hstr->data[size++] = '\0';

    char *endptr = NULL;
    if (is_hex) {
        assert (!is_octal);
        assert (!exp_seen);
        assert (!dot_seen);
        char *endptr = NULL;
        long v = strtol ((const char*) hstr->data, &endptr, 16);
        /* TODO: Check for overflow. */
        result->type = HIPACK_INTEGER;
        result->v_integer = (int32_t) v;
    } else if (is_octal) {
        assert (!is_hex);
        assert (!exp_seen);
        assert (!dot_seen);
        long v = strtol ((const char*) hstr->data, &endptr, 8);
        /* TODO: Check for overflow. */
        result->type = HIPACK_INTEGER;
        result->v_integer = (int32_t) v;
    } else if (dot_seen || exp_seen) {
        assert (!is_hex);
        assert (!is_octal);
        result->type = HIPACK_FLOAT;
        result->v_float = strtod ((const char*) hstr->data, &endptr);
    } else {
        assert (!is_hex);
        assert (!is_octal);
        assert (!exp_seen);
        assert (!dot_seen);
        long v = strtol ((const char*) hstr->data, &endptr, 10);
        /* TODO: Check for overflow. */
        result->type = HIPACK_INTEGER;
        result->v_integer = (int32_t) v;
    }

    if (endptr && *endptr != '\0') {
        *status = kStatusError;
        goto error;
    }

    hipack_string_free (hstr);
    return;

error:
    p->error = "invalid numeric value";
    hipack_string_free (hstr);
}


static void
parse_annotations (P, hipack_value_t *result, S)
{
    hipack_string_t *key = NULL;
    while (p->look == ':') {
        p->look = nextchar_raw (p, CHECK_OK);
        key = parse_key (p, CHECK_OK);
        skipwhite (p, CHECK_OK);

        /* Check if the annotation is already in the set. */
        if (result->annot && hipack_dict_get (result->annot, key)) {
            p->error = "duplicate annotation";
            *status = kStatusError;
            goto error;
        }
        /* Add the annotation to the set. */
        if (!result->annot)
            result->annot = hipack_dict_new ();

        static const hipack_value_t annot_present = {
            .type   = HIPACK_BOOL,
            .v_bool = true,
        };
        hipack_dict_set_adopt_key (result->annot, &key, &annot_present);
    }

error:
    if (key)
        hipack_string_free (key);
}


static hipack_value_t
parse_value (P, S)
{
    hipack_value_t result = DUMMY_VALUE;

    parse_annotations (p, &result, CHECK_OK);

    switch (p->look) {
        case '"': /* String */
            parse_string (p, &result, CHECK_OK);
            break;

        case '[': /* List */
            parse_list (p, &result, CHECK_OK);
            break;

        case '{': /* Dict */
            parse_dict (p, &result, CHECK_OK);
            break;

        case 'T': /* Bool */
        case 't':
        case 'F':
        case 'f':
            parse_bool (p, &result, CHECK_OK);
            break;

        default: /* Integer or Float */
            parse_number (p, &result, CHECK_OK);
            break;
    }

error:
    hipack_value_free (&result);
    return DUMMY_VALUE;
}


static void
parse_keyval_items (P, hipack_dict_t *result, int eos, S)
{
    hipack_value_t value = DUMMY_VALUE;
    hipack_string_t *key = NULL;

    while (p->look != eos) {
        key = parse_key (p, CHECK_OK);
        if (!key) {
            p->error = "missing dictionary key";
            *status = kStatusError;
            goto error;
        }

        bool got_separator = false;

        if (is_hipack_whitespace (p->look)) {
            got_separator = true;
            skipwhite (p, CHECK_OK);
        } else switch (p->look) {
            case ':':
                nextchar (p, CHECK_OK);
                skipwhite (p, CHECK_OK);
                /* fall-through */
            case '{':
            case '[':
                got_separator = true;
                break;
        }

        if (!got_separator) {
            p->error = "missing separator";
            *status = kStatusError;
            goto error;
        }

        value = parse_value (p, CHECK_OK);
        hipack_dict_set_adopt_key (result, &key, &value);

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
    return;

error:
    hipack_string_free (key);
    hipack_value_free (&value);
}


static hipack_dict_t*
parse_message (P, S)
{
    hipack_dict_t *result = hipack_dict_new ();

    nextchar (p, CHECK_OK);
    skipwhite (p, CHECK_OK);

    if (p->look == HIPACK_IO_ERROR) {
        *status = kStatusIoError;
    } else if (p->look == '{') {
        /* Input starts with a Dict marker. */
        nextchar (p, CHECK_OK);
        skipwhite (p, CHECK_OK);
        parse_keyval_items (p, result, '}', CHECK_OK);
        matchchar (p, '}', "unterminated message", CHECK_OK);
    } else {
        parse_keyval_items (p, result, HIPACK_IO_EOF, CHECK_OK);
    }
    return result;

error:
    hipack_dict_free (result);
    return NULL;
}


hipack_dict_t*
hipack_read (hipack_reader_t *reader)
{
    assert (reader);

    /*
     * Copy the reader function (and its data pointer) into the parser
     * structure. The rest of the fields are used as results, so the
     * reader structure can be cleaned up right after.
     */
    struct parser p = {
        .getchar      = reader->getchar,
        .getchar_data = reader->getchar_data,
        .line         = 1,
        0,
    };
    memset (reader, 0x00, sizeof (hipack_reader_t));

    status_t status = kStatusOk;
    hipack_dict_t *result = parse_message (&p, &status);
    switch (status) {
        case kStatusOk:
            assert (result);
            break;
        case kStatusError:
            assert (!result);
            assert (p.error);
            break;
        case kStatusIoError:
            p.error = HIPACK_READ_ERROR;
            hipack_dict_free (result);
            result = NULL;
            break;
        case kStatusEof:
            break;
    }

    reader->error        = p.error;
    reader->error_line   = p.line;
    reader->error_column = p.column;

    return result;
}


int
hipack_stdio_getchar (void *fp)
{
    assert (fp);
    int ch = fgetc ((FILE*) fp);
    if (ch == EOF) {
        return ferror ((FILE*) fp) ? HIPACK_IO_ERROR : HIPACK_IO_EOF;
    }
    return ch;
}

