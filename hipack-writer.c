/*
 * hipack-writer.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "hipack.h"

/*
 * Define FPCONV_H to avoid fpconv/src/fpconv.h being included.
 * By making our own definition of the function here, it can be
 * marked as "static inline".
 */
#define FPCONV_H 1
static inline int fpconv_dtoa (double fp, char dest[24]);
#include "fpconv/src/fpconv.c"


static inline bool
writechar (hipack_writer_t *writer, int ch)
{
    assert (ch != HIPACK_IO_ERROR);
    assert (ch != HIPACK_IO_EOF);

    assert (writer->putchar);

    int ret = (*writer->putchar) (writer->putchar_data, ch);
    assert (ret != HIPACK_IO_EOF);
    return ret == HIPACK_IO_ERROR;
}


#define CHECK_IO(statement)         \
    do {                            \
        if (statement) return true; \
    } while (0)


static inline void
moreindent (hipack_writer_t *writer)
{
    if (writer->indent != HIPACK_WRITER_COMPACT)
        writer->indent++;
}


static inline void
lessindent (hipack_writer_t *writer)
{
    if (writer->indent != HIPACK_WRITER_COMPACT)
        writer->indent--;
}


static inline bool
writeindent (hipack_writer_t *writer)
{
    int32_t num_spaces = (writer->indent != HIPACK_WRITER_COMPACT)
        ? writer->indent * 2 : 0;
    while (num_spaces--) {
        CHECK_IO (writechar (writer, ' '));
    }
    return false;
}


static inline bool
writedata (hipack_writer_t *writer,
           const char      *data,
           uint32_t         length)
{
    if (length) {
        assert (data);
        while (length--) {
            CHECK_IO (writechar (writer, *data++));
        }
    }
    return false;
}


static inline int
mapdigit (unsigned n)
{
    if (n < 10) {
        return '0' + n;
    }
    if (n < 36) {
        return 'A' + (n - 10);
    }
    assert (false);
    return '?';
}


static inline bool
formatint (hipack_writer_t *writer,
           int              value,
           uint8_t          base)
{
    assert (writer);
    if (value >= base) {
        CHECK_IO (formatint (writer, value / base, base));
    }
    CHECK_IO (writechar (writer, mapdigit (value % base)));
    return false;
}


bool
hipack_write_bool (hipack_writer_t *writer,
                   const bool       value)
{
    assert (writer);
    if (value) {
        return writedata (writer, "True", 4);
    } else {
        return writedata (writer, "False", 5);
    }
}


bool
hipack_write_integer (hipack_writer_t *writer,
                      const int32_t    value)
{
    assert (writer);
    if (value < 0) {
        CHECK_IO (writechar (writer, '-'));
        return formatint (writer, -value, 10);
    } else {
        return formatint (writer, value, 10);
    }
}


bool
hipack_write_float (hipack_writer_t *writer,
                    const double     value)
{
    assert (writer);

    char buf[24];
    int nchars = fpconv_dtoa (value, buf);
    bool need_dot = true;
    for (int i = 0; i < nchars; i++) {
        CHECK_IO (writechar (writer, buf[i]));
        if (buf[i] == '.' || buf[i] == 'e' || buf[i] == 'E') {
            need_dot = false;
        }
    }
    if (need_dot) {
        CHECK_IO (writechar (writer, '.'));
        CHECK_IO (writechar (writer, '0'));
    }
    return false;
}


bool
hipack_write_string (hipack_writer_t       *writer,
                     const hipack_string_t *hstr)
{
    assert (writer);
    assert (hstr);
    CHECK_IO (writechar (writer, '"'));
    for (uint32_t i = 0; i < hstr->size; i++) {
        switch (hstr->data[i]) {
            case 0x09: /* Horizontal tab. */
                CHECK_IO (writechar (writer, '\\'));
                CHECK_IO (writechar (writer, 't'));
                break;
            case 0x0A: /* New line. */
                CHECK_IO (writechar (writer, '\\'));
                CHECK_IO (writechar (writer, 'n'));
                break;
            case 0x0D: /* Carriage return. */
                CHECK_IO (writechar (writer, '\\'));
                CHECK_IO (writechar (writer, 'r'));
                break;
            case 0x22: /* Double quote. */
                CHECK_IO (writechar (writer, '\\'));
                CHECK_IO (writechar (writer, '"'));
                break;
            case 0x5C: /* Backslash. */
                CHECK_IO (writechar (writer, '\\'));
                CHECK_IO (writechar (writer, '\\'));
                break;
            default:
                if (hstr->data[i] < 0x20) {
                    /* ASCII non-printable character. */
                    CHECK_IO (writechar (writer, '\\'));
                    if ((uint8_t) hstr->data[i] < 16) {
                        /* Leading zero. */
                        CHECK_IO (writechar (writer, '0'));
                    }
                    CHECK_IO (formatint (writer, (uint8_t) hstr->data[i], 16));
                } else {
                    CHECK_IO (writechar (writer, hstr->data[i]));
                }
        }
    }
    CHECK_IO (writechar (writer, '"'));
    return false;
}


static bool
write_keyval (hipack_writer_t     *writer,
              const hipack_dict_t *dict)
{
    const hipack_string_t *key;
    hipack_value_t *value;
    HIPACK_DICT_FOREACH (dict, key, value) {
        writeindent (writer);
        /* Key */
        for (uint32_t i = 0; i < key->size; i++) {
            CHECK_IO (writechar (writer, key->data[i]));
        }

        switch (value->type) {
            case HIPACK_INTEGER:
            case HIPACK_FLOAT:
            case HIPACK_BOOL:
            case HIPACK_STRING:
                CHECK_IO (writechar (writer, ':'));
                break;
            case HIPACK_DICT:
            case HIPACK_LIST:
                /* No colon. */
                break;
            default:
                assert (false);
        }

        if (writer->indent != HIPACK_WRITER_COMPACT) {
            CHECK_IO (writechar (writer, ' '));
        }

        CHECK_IO (hipack_write_value (writer, value));
        CHECK_IO (writechar (writer, ','));

        if (writer->indent != HIPACK_WRITER_COMPACT) {
            CHECK_IO (writechar (writer, '\n'));
        }
    }

    return false;
}


bool
hipack_write_list (hipack_writer_t     *writer,
                   const hipack_list_t *list)
{
    assert (writer);
    assert (list);

    CHECK_IO (writechar (writer, '['));

    if (hipack_list_size (list)) {
        if (writer->indent != HIPACK_WRITER_COMPACT) {
            CHECK_IO (writechar (writer, '\n'));
        }

        moreindent (writer);
        for (uint32_t i = 0; i < list->size; i++) {
            CHECK_IO (writeindent (writer));
            CHECK_IO (hipack_write_value (writer, &list->data[i]));
            CHECK_IO (writechar (writer, ','));
            if (writer->indent != HIPACK_WRITER_COMPACT) {
                CHECK_IO (writechar (writer, '\n'));
            }
        }
        lessindent (writer);
        CHECK_IO (writeindent (writer));
    }

    CHECK_IO (writechar (writer, ']'));
    return false;
}


bool
hipack_write_dict (hipack_writer_t     *writer,
                   const hipack_dict_t *dict)
{
    CHECK_IO (writechar (writer, '{'));

    if (hipack_dict_size (dict)) {
        if (writer->indent != HIPACK_WRITER_COMPACT) {
            CHECK_IO (writechar (writer, '\n'));
        }

        moreindent (writer);
        CHECK_IO (write_keyval (writer, dict));
        lessindent (writer);
        CHECK_IO (writeindent (writer));
    }
    CHECK_IO (writechar (writer, '}'));
    return false;
}


bool
hipack_write_value (hipack_writer_t      *writer,
                    const hipack_value_t *value)
{
    assert (writer);
    assert (value);

    switch (value->type) {
        case HIPACK_INTEGER:
            return hipack_write_integer (writer, value->v_integer);
        case HIPACK_FLOAT:
            return hipack_write_float (writer, value->v_float);
        case HIPACK_BOOL:
            return hipack_write_bool (writer, value->v_bool);
        case HIPACK_STRING:
            return hipack_write_string (writer, value->v_string);
        case HIPACK_LIST:
            return hipack_write_list (writer, value->v_list);
        case HIPACK_DICT:
            return hipack_write_dict (writer, value->v_dict);
    }

    assert (false); /* Never reached. */
    return false;
}


bool
hipack_write (hipack_writer_t     *writer,
              const hipack_dict_t *message)
{
    assert (writer);
    assert (message);
    if (writer->indent != HIPACK_WRITER_COMPACT) {
        writer->indent = HIPACK_WRITER_INDENTED;
    }
    return write_keyval (writer, message);
}


int
hipack_stdio_putchar (void* fp, int ch)
{
    assert (fp);
    int ret = fputc (ch, (FILE*) fp);
    return (ret == EOF) ? HIPACK_IO_ERROR : ch;
}
