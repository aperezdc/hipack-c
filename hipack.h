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
#include <stdbool.h>


typedef enum {
    HIPACK_INTEGER,
    HIPACK_FLOAT,
    HIPACK_BOOL,
    HIPACK_STRING,
    HIPACK_LIST,
    HIPACK_DICT,
} hipack_type_t;


/* Forward declarations. */
typedef struct hipack_value  hipack_value_t;
typedef struct hipack_string hipack_string_t;
typedef struct hipack_dict   hipack_dict_t;
typedef struct hipack_list   hipack_list_t;


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


extern void hipack_string_free (hipack_string_t *hstr);


struct hipack_list {
    uint32_t       size;
    hipack_value_t data[]; /* C99 flexible array. */
};


extern hipack_list_t* hipack_list_new (void);
extern void hipack_list_free (hipack_list_t *list);

extern hipack_dict_t* hipack_dict_new (void);
extern void hipack_dict_free (hipack_dict_t *dict);


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


extern hipack_dict_t* hipack_read (FILE* fp);


#endif /* !HIPACK_H */
