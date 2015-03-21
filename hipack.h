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
    hipack_type_t type;
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


struct hipack_dict {
    hipack_dict_node_t **nodes;
    hipack_dict_node_t  *first;
    uint32_t             count;
    uint32_t             size;
};


extern hipack_dict_t* hipack_dict_new (void);
extern void hipack_dict_free (hipack_dict_t *dict);

extern void hipack_dict_set_adopt_key (hipack_dict_t        *dict,
                                       hipack_string_t     **key,
                                       const hipack_value_t *value);

extern void hipack_dict_set (hipack_dict_t         *dict,
                             const hipack_string_t *key,
                             const hipack_value_t  *value);

extern bool hipack_dict_get (const hipack_dict_t   *dict,
                             const hipack_string_t *key,
                             hipack_value_t        *value);



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

static inline void
hipack_value_free (hipack_value_t *value)
{
    assert (value);

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


#endif /* !HIPACK_H */
