/*
 * hipack-writer.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "hipack.h"


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
                    CHECK_IO (formatint (writer, (uint8_t) hstr->data[i], 16));
                } else {
                    CHECK_IO (writechar (writer, hstr->data[i]));
                }
        }
    }
    CHECK_IO (writechar (writer, '"'));
    return false;
}


bool
hipack_write (hipack_writer_t     *writer,
              const hipack_dict_t *message)
{
    assert (writer);
    assert (message);
}


int
hipack_stdio_putchar (void* fp, int ch)
{
    assert (fp);
    int ret = fputc (ch, (FILE*) fp);
    return (ret == EOF) ? HIPACK_IO_ERROR : ch;
}
