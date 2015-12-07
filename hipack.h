/*
 * hipack.h
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef HIPACK_H
#define HIPACK_H

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>


typedef enum {
    HIPACK_INTEGER,
    HIPACK_FLOAT,
    HIPACK_BOOL,
    HIPACK_STRING,
    HIPACK_LIST,
    HIPACK_DICT,
} hipack_type_t;


extern void* (*hipack_alloc) (void*, size_t);
extern void* hipack_alloc_stdlib (void*, size_t);
extern void* hipack_alloc_array_extra (void*, size_t nmemb, size_t size, size_t extra);


static inline void*
hipack_alloc_bzero (size_t size)
{
    assert (size > 0);
    return memset ((*hipack_alloc) (NULL, size), 0, size);
}


static inline void*
hipack_alloc_array (void *optr, size_t nmemb, size_t size)
{
    return hipack_alloc_array_extra (optr, nmemb, size, 0);
}


static inline void
hipack_alloc_free (void *optr)
{
    (*hipack_alloc) (optr, 0);
}


/* Forward declarations. */
typedef struct hipack_value     hipack_value_t;
typedef struct hipack_string    hipack_string_t;
typedef struct hipack_dict      hipack_dict_t;
typedef struct hipack_dict_node hipack_dict_node_t;
typedef struct hipack_list      hipack_list_t;


struct hipack_value {
    hipack_type_t  type;
    hipack_dict_t *annot;
    union {
        int32_t          v_integer;
        double           v_float;
        bool             v_bool;
        hipack_string_t *v_string;
        hipack_list_t   *v_list;
        hipack_dict_t   *v_dict;
    };
};


struct hipack_string {
    uint32_t size;
    uint8_t  data[]; /* C99 flexible array. */
};


extern hipack_string_t* hipack_string_copy (const hipack_string_t *hstr);
extern hipack_string_t* hipack_string_new_from_string (const char *str);
extern hipack_string_t* hipack_string_new_from_lstring (const char *str, uint32_t len);
extern uint32_t hipack_string_hash (const hipack_string_t *hstr);
extern bool hipack_string_equal (const hipack_string_t *hstr1,
                                 const hipack_string_t *hstr2);
extern void hipack_string_free (hipack_string_t *hstr);


struct hipack_list {
    uint32_t       size;
    hipack_value_t data[]; /* C99 flexible array. */
};


extern hipack_list_t* hipack_list_new (uint32_t size);
extern void hipack_list_free (hipack_list_t *list);
extern bool hipack_list_equal (const hipack_list_t *a,
                               const hipack_list_t *b);

static inline uint32_t
hipack_list_size (const hipack_list_t *list)
{
    assert (list);
    return list->size;
}

#define HIPACK_LIST_AT(_list, _index) \
    (assert ((_index) < (_list)->size), &((_list)->data[_index]))


struct hipack_dict {
    hipack_dict_node_t **nodes;
    hipack_dict_node_t  *first;
    uint32_t             count;
    uint32_t             size;
};


static inline uint32_t
hipack_dict_size (const hipack_dict_t *dict)
{
    assert (dict);
    return dict->count;
}


extern hipack_dict_t* hipack_dict_new (void);
extern void hipack_dict_free (hipack_dict_t *dict);

extern bool hipack_dict_equal (const hipack_dict_t *a,
                               const hipack_dict_t *b);


extern void hipack_dict_set_adopt_key (hipack_dict_t        *dict,
                                       hipack_string_t     **key,
                                       const hipack_value_t *value);

extern void hipack_dict_set (hipack_dict_t         *dict,
                             const hipack_string_t *key,
                             const hipack_value_t  *value);

extern void hipack_dict_del (hipack_dict_t         *dict,
                             const hipack_string_t *key);

extern hipack_value_t* hipack_dict_get (const hipack_dict_t   *dict,
                                        const hipack_string_t *key);

extern hipack_value_t* hipack_dict_first (const hipack_dict_t    *dict,
                                          const hipack_string_t **key);

extern hipack_value_t* hipack_dict_next (hipack_value_t         *value,
                                         const hipack_string_t **key);

#define HIPACK_DICT_FOREACH(_d, _k, _v)          \
    for ((_v) = hipack_dict_first ((_d), &(_k)); \
         (_v) != NULL;                           \
         (_v) = hipack_dict_next ((_v), &(_k)))

static inline hipack_type_t
hipack_value_type (const hipack_value_t *value)
{
    return value->type;
}


#define HIPACK_TYPES(F) \
    F (int32_t,          integer, HIPACK_INTEGER) \
    F (double,           float,   HIPACK_FLOAT  ) \
    F (bool,             bool,    HIPACK_BOOL   ) \
    F (hipack_string_t*, string,  HIPACK_STRING ) \
    F (hipack_list_t*,   list,    HIPACK_LIST   ) \
    F (hipack_dict_t*,   dict,    HIPACK_DICT   )

#define HIPACK_DEFINE_IS_TYPE(_type, name, type_tag)          \
    static inline bool                                        \
    hipack_value_is_ ## name (const hipack_value_t *value) {  \
        return value->type == type_tag;                       \
    }

#define HIPACK_DEFINE_GET_VALUE(_type, name, type_tag)        \
    static inline const _type                                 \
    hipack_value_get_ ## name (const hipack_value_t *value) { \
        assert (value->type == type_tag);                     \
        return value->v_ ## name;                             \
    }

HIPACK_TYPES (HIPACK_DEFINE_IS_TYPE)
HIPACK_TYPES (HIPACK_DEFINE_GET_VALUE)

#undef HIPACK_DEFINE_IS_TYPE
#undef HIPACK_DEFINE_GET_VALUE

extern bool hipack_value_equal (const hipack_value_t *a,
                                const hipack_value_t *b);

static inline void
hipack_value_free (hipack_value_t *value)
{
    assert (value);

    if (value->annot)
        hipack_dict_free (value->annot);

    switch (value->type) {
        case HIPACK_INTEGER:
        case HIPACK_FLOAT:
        case HIPACK_BOOL:
            /* Nothing to free. */
            break;

        case HIPACK_STRING:
            hipack_string_free (value->v_string);
            break;

        case HIPACK_LIST:
            hipack_list_free (value->v_list);
            break;

        case HIPACK_DICT:
            hipack_dict_free (value->v_dict);
            break;
    }
}


static inline void
hipack_value_add_annot (hipack_value_t *value,
                        const char     *annot)
{
    assert (value);
    assert (annot);

    if (!value->annot) {
        value->annot = hipack_dict_new ();
    }

    static const hipack_value_t bool_true = {
        .type   = HIPACK_BOOL,
        .v_bool = true,
    };
    hipack_string_t *key = hipack_string_new_from_string (annot);
    hipack_dict_set_adopt_key (value->annot, &key, &bool_true);
}

static inline bool
hipack_value_has_annot (const hipack_value_t *value,
                        const char           *annot)
{
    assert (value);
    assert (annot);

    /* TODO: Try to avoid the string copy. */
    hipack_string_t *key = hipack_string_new_from_string (annot);
    bool result = (value->annot) && hipack_dict_get (value->annot, key);
    hipack_string_free (key);
    return result;
}

static inline void
hipack_value_del_annot (hipack_value_t *value,
                        const char     *annot)
{
    assert (value);
    assert (annot);

    if (value->annot) {
        hipack_string_t *key = hipack_string_new_from_string (annot);
        hipack_dict_del (value->annot, key);
    }
}


typedef struct {
    int       (*getchar) (void*);
    void       *getchar_data;
    const char *error;
    unsigned    error_line;
    unsigned    error_column;
} hipack_reader_t;


enum {
    HIPACK_IO_EOF   = -1,
    HIPACK_IO_ERROR = -2,
};

extern const char* HIPACK_READ_ERROR;


extern int hipack_stdio_getchar (void* fp);
extern hipack_dict_t* hipack_read (hipack_reader_t *reader);


enum {
    HIPACK_WRITER_COMPACT = -1,
    HIPACK_WRITER_INDENTED = 0,
};

typedef struct {
    int   (*putchar) (void*, int);
    void   *putchar_data;
    int32_t indent;
} hipack_writer_t;


#define HIPACK_DEFINE_WRITE_VALUE(_type, name, type_tag) \
    extern bool hipack_write_ ## name (hipack_writer_t *writer, \
                                       const _type value);

HIPACK_TYPES (HIPACK_DEFINE_WRITE_VALUE)

#undef HIPACK_DEFINE_WRITE_VALUE

extern bool hipack_write_value (hipack_writer_t      *writer,
                                const hipack_value_t *value);

extern bool hipack_write (hipack_writer_t     *writer,
                          const hipack_dict_t *message);

extern int hipack_stdio_putchar (void* fp, int ch);

#endif /* !HIPACK_H */
