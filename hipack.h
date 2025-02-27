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

/**
 * =============
 * API Reference
 * =============
 *
 * Types
 * =====
 */

/*~t hipack_type_t
 *
 * Type of a value. This enumeration takes one of the following values:
 *
 * - ``HIPACK_INTEGER``: Integer value.
 * - ``HIPACK_FLOAT``: Floating point value.
 * - ``HIPACK_BOOL``: Boolean value.
 * - ``HIPACK_STRING``: String value.
 * - ``HIPACK_LIST``: List value.
 * - ``HIPACK_DICT``: Dictionary value.
 */
typedef enum {
    HIPACK_INTEGER,
    HIPACK_FLOAT,
    HIPACK_BOOL,
    HIPACK_STRING,
    HIPACK_LIST,
    HIPACK_DICT,
} hipack_type_t;


/* Forward declarations. */
typedef struct hipack_value     hipack_value_t;
typedef struct hipack_string    hipack_string_t;
typedef struct hipack_dict      hipack_dict_t;
typedef struct hipack_dict_node hipack_dict_node_t;
typedef struct hipack_list      hipack_list_t;


/*~t hipack_value_t
 *
 * Represent any valid HiPack value.
 *
 * - :func:`hipack_value_type()` obtains the type of a value.
 */
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


/*~t hipack_string_t
 *
 * String value.
 */
struct hipack_string {
    uint32_t size;
    uint8_t  data[]; /* C99 flexible array. */
};

/*~t hipack_list_t
 *
 * List value.
 */
struct hipack_list {
    uint32_t       size;
    hipack_value_t data[]; /* C99 flexible array. */
};

/*~t hipack_dict_t
 *
 * Dictionary value.
 */
struct hipack_dict {
    hipack_dict_node_t **nodes;
    hipack_dict_node_t  *first;
    uint32_t             count;
    uint32_t             size;
};


/**
 * Memory Allocation
 * =================
 *
 * How ``hipack-c`` allocates memory can be customized by setting
 * :c:data:`hipack_alloc` to a custom allocation function.
 */

/*~v hipack_alloc
 *
 * Allocation function. By default it is set to :func:`hipack_alloc_stdlib()`,
 * which uses the implementations of ``malloc()``, ``realloc()``, and
 * ``free()`` provided by the C library.
 *
 * Allocation functions always have the following prototype:
 *
 * .. code-block:: c
 *
 *    void* func (void *oldptr, size_t size);
 *
 * The behavior must be as follows:
 *
 * - When invoked with ``oldptr`` set to ``NULL``, and a non-zero ``size``,
 *   the function behaves like ``malloc()``: a memory block of at least
 *   ``size`` bytes is allocated and a pointer to it returned.
 * - When ``oldptr`` is non-``NULL``, and a non-zero ``size``, the function
 *   behaves like ``realloc()``: the memory area pointed to by ``oldptr``
 *   is resized to be at least ``size`` bytes, or its contents moved to a
 *   new memory area of at least ``size`` bytes. The returned pointer may
 *   either be ``oldptr``, or a pointer to the new memory area if the data
 *   was relocated.
 * - When ``oldptr`` is non-``NULL``, and ``size`` is zero, the function
 *   behaves like ``free()``.
 */
extern void* (*hipack_alloc) (void*, size_t);

/*~f void* hipack_alloc_stdlib(void*, size_t)
 *
 * Default allocation function. It uses ``malloc()``, ``realloc()``, and
 * ``free()`` from the C library. By default :any:`hipack_alloc` is set
 * to use this function.
 */
extern void* hipack_alloc_stdlib (void*, size_t);

/*~f void* hipack_alloc_array_extra (void *oldptr, size_t nmemb, size_t size, size_t extra)
 *
 * Allocates (if `oldptr` is ``NULL``) or reallocates (if `oldptr` is
 * non-``NULL``) memory for an array which contains `nmemb` elements, each one
 * of `size` bytes, plus an arbitrary amount of `extra` bytes.
 *
 * This function is used internally by the HiPack parser, and it is not likely
 * to be needed by client code.
 */
extern void* hipack_alloc_array_extra (void*, size_t nmemb, size_t size, size_t extra);

/*~f void* hipack_alloc_array (void *optr, size_t nmemb, size_t size)
 *
 * Same as :c:func:`hipack_alloc_array_extra()`, without allowing to specify
 * the amount of extra bytes. The following calls are both equivalent:
 *
 * .. code-block:: c
 *
 *    void *a = hipack_alloc_array_extra (NULL, 10, 4, 0);
 *    void *b = hipack_alloc_array (NULL, 10, 4);
 *
 * See :c:func:`hipack_alloc_array_extra()` for details.
 */
static inline void*
hipack_alloc_array (void *optr, size_t nmemb, size_t size)
{
    return hipack_alloc_array_extra (optr, nmemb, size, 0);
}

/*~f void* hipack_alloc_bzero (size_t size)
 *
 * Allocates an area of memory of `size` bytes, and initializes it to zeroes.
 */
static inline void*
hipack_alloc_bzero (size_t size)
{
    assert (size > 0);
    return memset ((*hipack_alloc) (NULL, size), 0, size);
}

/*~f void hipack_alloc_free (void *pointer)
 *
 * Frees the memory area referenced by the given `pointer`.
 */
static inline void
hipack_alloc_free (void *optr)
{
    (*hipack_alloc) (optr, 0);
}


/**
 * String Functions
 * ================
 *
 * The following functions are provided as a convenience to operate on values
 * of type :c:type:`hipack_string_t`.
 *
 * .. note:: The hash function used by :c:func:`hipack_string_hash()` is
 *    *not* guaranteed to be cryptographically safe. Please do avoid exposing
 *    values returned by this function to the attack surface of your
 *    applications, in particular *do not expose them to the network*.
 */

/*~f hipack_string_t* hipack_string_copy (const hipack_string_t *hstr)
 *
 * Returns a new copy of a string.
 *
 * The returned value must be freed using :c:func:`hipack_string_free()`.
 */
extern hipack_string_t* hipack_string_copy (const hipack_string_t *hstr);

/*~f hipack_string_t* hipack_string_new_from_string (const char *str)
 *
 * Creates a new string from a C-style zero terminated string.
 *
 * The returned value must be freed using :c:func:`hipack_string_free()`.
 */
extern hipack_string_t* hipack_string_new_from_string (const char *str);

/*~f hipack_string_t* hipack_string_new_from_lstring (const char *str, uint32_t len)
 *
 * Creates a new string from a memory area and its length.
 *
 * The returned value must be freed using :c:func:`hipack_string_free()`.
 */
extern hipack_string_t* hipack_string_new_from_lstring (const char *str, uint32_t len);

/*~f uint32_t hipack_string_hash (const hipack_string_t *hstr)
 *
 * Calculates a hash value for a string.
 */
extern uint32_t hipack_string_hash (const hipack_string_t *hstr);

/*~f bool hipack_string_equal (const hipack_string_t *hstr1, const hipack_string_t *hstr2)
 * Compares two strings to check whether their contents are the same.
 */
extern bool hipack_string_equal (const hipack_string_t *hstr1,
                                 const hipack_string_t *hstr2);

/*~f void hipack_string_free (hipack_string_t *hstr)
 * Frees the memory used by a string.
 */
extern void hipack_string_free (hipack_string_t *hstr);


/**
 * List Functions
 * ==============
 */

/*~f hipack_list_t* hipack_list_new (uint32_t size)
 * Creates a new list for ``size`` elements.
 */
extern hipack_list_t* hipack_list_new (uint32_t size);

/*~f void hipack_list_free (hipack_list_t *list)
 * Frees the memory used by a list.
 */
extern void hipack_list_free (hipack_list_t *list);

/*~f bool hipack_list_equal (const hipack_list_t *a, const hipack_list_t *b)
 * Checks whether two lists contains the same values.
 */
extern bool hipack_list_equal (const hipack_list_t *a,
                               const hipack_list_t *b);

/*~f uint32_t hipack_list_size (const hipack_list_t *list)
 * Obtains the number of elements in a list.
 */
static inline uint32_t
hipack_list_size (const hipack_list_t *list)
{
    assert (list);
    return list->size;
}

/*~M HIPACK_LIST_AT(list, index)
 *
 * Obtains a pointer to the element at a given `index` of a `list`.
 */
#define HIPACK_LIST_AT(_list, _index) \
    (assert ((_index) < (_list)->size), &((_list)->data[_index]))


/**
 * .. _dict_funcs:
 *
 * Dictionary Functions
 * ====================
 */

/*~f uint32_t hipack_dict_size (const hipack_dict_t *dict)
 *
 * Obtains the number of elements in a dictionary.
 */
static inline uint32_t
hipack_dict_size (const hipack_dict_t *dict)
{
    assert (dict);
    return dict->count;
}

/*~f hipack_dict_t* hipack_dict_new (void)
 *
 * Creates a new, empty dictionary.
 */
extern hipack_dict_t* hipack_dict_new (void);

/*~f void hipack_dict_free (hipack_dict_t *dict)
 *
 * Frees the memory used by a dictionary.
 */
extern void hipack_dict_free (hipack_dict_t *dict);

/*~f bool hipack_dict_equal (const hipack_dict_t *a, const hipack_dict_t *b)
 *
 * Checks whether two dictinaries contain the same keys, and their associated
 * values in each of the dictionaries are equal.
 */
extern bool hipack_dict_equal (const hipack_dict_t *a,
                               const hipack_dict_t *b);

/*~f void hipack_dict_set (hipack_dict_t *dict, const hipack_string_t *key, const hipack_value_t *value)
 *
 * Adds an association of a `key` to a `value`.
 *
 * Note that this function will copy the `key`. If you are not planning to
 * continue reusing the `key`, it is recommended to use
 * :c:func:`hipack_dict_set_adopt_key()` instead.
 */
extern void hipack_dict_set (hipack_dict_t         *dict,
                             const hipack_string_t *key,
                             const hipack_value_t  *value);

/*~f void hipack_dict_set_adopt_key (hipack_dict_t *dict, hipack_string_t **key, const hipack_value_t *value)
 *
 * Adds an association of a `key` to a `value`, passing ownership of the
 * memory using by the `key` to the dictionary (i.e. the string used as key
 * will be freed by the dictionary).
 *
 * Use this function instead of :c:func:`hipack_dict_set()` when the `key`
 * is not going to be used further afterwards.
 */
extern void hipack_dict_set_adopt_key (hipack_dict_t        *dict,
                                       hipack_string_t     **key,
                                       const hipack_value_t *value);

/*~f void hipack_dict_del (hipack_dict_t *dict, const hipack_string_t *key)
 *
 * Removes the element from a dictionary associated to a `key`.
 */
extern void hipack_dict_del (hipack_dict_t         *dict,
                             const hipack_string_t *key);

/*~f hipack_value_t* hipack_dict_get (const hipack_dict_t *dict, const hipack_string_t *key)
 *
 * Obtains the value associated to a `key` from a dictionary.
 *
 * The returned value points to memory owned by the dictionary. The value can
 * be modified in-place, but it shall not be freed.
 */
extern hipack_value_t* hipack_dict_get (const hipack_dict_t   *dict,
                                        const hipack_string_t *key);

/*~f hipack_value_t* hipack_dict_first (const hipack_dict_t *dict, const hipack_string_t **key)
 *
 * Obtains an a *(key, value)* pair, which is considered the *first* in
 * iteration order. This can be used in combination with
 * :c:func:`hipack_dict_next()` to enumerate all the *(key, value)* pairs
 * stored in the dictionary:
 *
 * .. code-block:: c
 *
 *    hipack_dict_t *d = get_dictionary ();
 *    hipack_value_t *v;
 *    hipack_string_t *k;
 *
 *    for (v = hipack_dict_first (d, &k);
 *         v != NULL;
 *         v = hipack_dict_next (v, &k)) {
 *        // Use "k" and "v" normally.
 *    }
 *
 * As a shorthand, consider using :c:macro:`HIPACK_DICT_FOREACH()` instead.
 */
extern hipack_value_t* hipack_dict_first (const hipack_dict_t    *dict,
                                          const hipack_string_t **key);

/*~f hipack_value_t* hipack_dict_next (hipack_value_t *value, const hipack_string_t **key)
 *
 * Iterates to the next *(key, value)* pair of a dictionary. For usage
 * details, see :c:func:`hipack_dict_first()`.
 */
extern hipack_value_t* hipack_dict_next (hipack_value_t         *value,
                                         const hipack_string_t **key);

/*~M HIPACK_DICT_FOREACH(dict, key, value)
 *
 * Convenience macro used to iterate over the *(key, value)* pairs contained
 * in a dictionary. Internally this uses :c:func:`hipack_dict_first()` and
 * :c:func:`hipack_dict_next()`.
 *
 * .. code-block:: c
 *
 *    hipack_dict_t *d = get_dictionary ();
 *    hipack_string_t *k;
 *    hipack_value_t *v;
 *    HIPACK_DICT_FOREACH (d, k, v) {
 *        // Use "k" and "v"
 *    }
 *
 * Using this macro is the recommended way of writing a loop to enumerate
 * elements from a dictionary.
 */
#define HIPACK_DICT_FOREACH(_d, _k, _v)          \
    for ((_v) = hipack_dict_first ((_d), &(_k)); \
         (_v) != NULL;                           \
         (_v) = hipack_dict_next ((_v), &(_k)))

/**
 * Value Functions
 * ===============
 */

/*~f hipack_type_t hipack_value_type (const hipack_value_t *value)
 *
 * Obtains the type of a value.
 */
static inline hipack_type_t
hipack_value_type (const hipack_value_t *value)
{
    return value->type;
}

/*~f hipack_value_t hipack_integer (int32_t value)
 * Creates a new integer value.
 */
/*~f hipack_value_t hipack_float (double value)
 * Creates a new floating point value.
 */
/*~f hipack_value_t hipack_bool (bool value)
 * Creates a new boolean value.
 */
/*~f hipack_value_t hipack_string (hipack_string_t *value)
 * Creates a new string value.
 */
/*~f hipack_value_t hipack_list (hipack_list_t *value)
 * Creates a new list value.
 */
/*~f hipack_value_t hipack_dict (hipack_dict_t *value)
 * Creates a new dictionary value.
 */

/*~f bool hipack_value_is_integer (const hipack_value_t *value)
 * Checks whether a value is an integer.
 */
/*~f bool hipack_value_is_float (const hipack_value_t *value)
 * Checks whether a value is a floating point number.
 */
/*~f bool hipack_value_is_bool (const hipack_value_t *value)
 * Checks whether a value is a boolean.
 */
/*~f bool hipack_value_is_string (const hipack_value_t *value)
 * Checks whether a value is a string.
 */
/*~f bool hipack_value_is_list (const hipack_value_t *value)
 * Checks whether a value is a list.
 */
/*~f bool hipack_value_is_dict (const hipack_value_t *value)
 * Checks whether a value is a dictionary.
 */

/*~f const int32_t hipack_value_get_integer (const hipack_value_t *value)
 * Obtains a numeric value as an ``int32_t``.
 */
/*~f const double hipack_value_get_float (const hipack_value_t *value)
 * Obtains a floating point value as a ``double``.
 */
/*~f const bool hipack_value_get_bool (const hipack_value_t *value)
 * Obtains a boolean value as a ``bool``.
 */
/*~f const hipack_string_t* hipack_value_get_string (const hipack_value_t *value)
 * Obtains a numeric value as a :c:type:`hipack_string_t`.
 */
/*~f const hipack_list_t* hipack_value_get_list (const hipack_value_t *value)
 * Obtains a numeric value as a :c:type:`hipack_list_t`.
 */
/*~f const hipack_dict_t* hipack_value_get_dict (const hipack_value_t *value)
 * Obtains a numeric value as a :c:type:`hipack_dict_t`.
 */

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

#define HIPACK_DEFINE_MAKE_VALUE(_type, name, type_tag)                 \
    static inline hipack_value_t                                        \
    hipack_ ## name (_type value) {                                     \
        hipack_value_t v = { type_tag, NULL, { .v_ ## name = value } }; \
        return v;                                                       \
    }

HIPACK_TYPES (HIPACK_DEFINE_IS_TYPE)
HIPACK_TYPES (HIPACK_DEFINE_GET_VALUE)
HIPACK_TYPES (HIPACK_DEFINE_MAKE_VALUE)

#undef HIPACK_DEFINE_IS_TYPE
#undef HIPACK_DEFINE_GET_VALUE
#undef HIPACK_DEFINE_MAKE_VALUE


/*~f bool hipack_value_equal (const hipack_value_t *a, const hipack_value_t *b)
 *
 * Checks whether two values are equal.
 */
extern bool hipack_value_equal (const hipack_value_t *a,
                                const hipack_value_t *b);

/*~f void hipack_value_free (hipack_value_t *value)
 *
 * Frees the memory used by a value.
 */
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

/*~f void hipack_value_add_annot (hipack_value_t *value, const char *annot)
 *
 * Adds an annotation to a value. If the value already had the annotation,
 * this function is a no-op.
 */
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

/*~f bool hipack_value_has_annot (const hipack_value_t *value, const char *annot)
 *
 * Checks whether a value has a given annotation.
 */
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

/*~f void hipack_value_del_annot (hipack_value_t *value, const char *annot)
 *
 * Removes an annotation from a value. If the annotation was not present, this
 * function is a no-op.
 */
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


/**
 * Reader Interface
 * ================
 */

/*~t hipack_reader_t
 *
 * Allows communicating with the parser, instructing it how to read text
 * input data, and provides a way for the parser to report errors back.
 *
 * The following members of the structure are to be used by client code:
 */
typedef struct {
    /*~m int (*getchar)(void *data)
     * Reader callback function. The function will be called every time the
     * next character of input is needed. It must return it as an integer,
     * :any:`HIPACK_IO_EOF` when trying to read pas the end of the input,
     * or :any:`HIPACK_IO_ERROR` if an input error occurs.
     */
    int (*getchar) (void*);

    /*m void *getchar_data
     * Data passed to the reader callback function.
     */
    void *getchar_data;

    /*~m const char *error
     * On error, a string describing the issue, suitable to be displayed to
     * the user.
     */
    const char *error;

    /*~m unsigned error_line
     * On error, the line number where parsing was stopped.
     */
    unsigned error_line;

    /*~m unsigned error_column
     * On error, the column where parsing was stopped.
     */
    unsigned error_column;
} hipack_reader_t;


enum {
/*~M HIPACK_IO_EOF
 * Constant returned by reader functions when trying to read past the end of
 * the input.
 */
    HIPACK_IO_EOF   = -1,

/*~M HIPACK_IO_ERROR
 * Constant returned by reader functions on input errors.
 */
    HIPACK_IO_ERROR = -2,
};

/*~M HIPACK_READ_ERROR
 *
 * Constant value used to signal an underlying input error.
 *
 * The `error` field of :c:type:`hipack_reader_t` is set to this value when
 * the reader function returns :any:`HIPACK_IO_ERROR`. This is provided to
 * allow client code to detect this condition and further query for the
 * nature of the input error.
 */
extern const char* HIPACK_READ_ERROR;

/*~f hipack_dict_t* hipack_read (hipack_reader_t *reader)
 *
 * Reads a HiPack message from a stream `reader` and returns a dictionary.
 *
 * On error, ``NULL`` is returned, and the members `error`, `error_line`,
 * and `error_column` (see :c:type:`hipack_reader_t`) are set accordingly
 * in the `reader`.
 */
extern hipack_dict_t* hipack_read (hipack_reader_t *reader);

/*~f int hipack_stdio_getchar (void* fp)
 *
 * Reader function which uses ``FILE*`` objects from the standard C library.
 *
 * To use this function to read from a ``FILE*``, first open a file, and
 * then create a reader using this function and the open file as data to
 * be passed to it, and then use :c:func:`hipack_read()`:
 *
 * .. code-block:: c
 *
 *    FILE* stream = fopen (HIPACK_FILE_PATH, "rb")
 *    hipack_reader_t reader = {
 *        .getchar = hipack_stdio_getchar,
 *        .getchar_data = stream,
 *    };
 *    hipack_dict_t *message = hipack_read (&reader);
 *
 * The user is responsible for closing the ``FILE*`` after using it.
 */
extern int hipack_stdio_getchar (void* fp);


/**
 * Writer Interface
 * ================
 */

/*~t hipack_writer_t
 *
 * Allows specifying how to write text output data, and configuring how
 * the produced HiPack output looks like.
 *
 * The following members of the structure are to be used by client code:
 */
typedef struct {
    /*~m int (*putchar)(void *data, int ch)
     * Writer callback function. The function will be called every time a
     * character is produced as output. It must return :any:`HIPACK_IO_ERROR`
     * if an output error occurs, and it is invalid for the callback to
     * return :any:`HIPACK_IO_EOF`. Any other value is interpreted as
     * indication of success.
     */
    int (*putchar) (void*, int);

    /*~m void* putchar_data
     * Data passed to the writer callback function.
     */
    void *putchar_data;

    /*~m int32_t indent
     * Either :any:`HIPACK_WRITER_COMPACT` or :any:`HIPACK_WRITER_INDENTED`.
     */
    int32_t indent;
} hipack_writer_t;


enum {
/*~M HIPACK_WRITER_COMPACT
 * Flag to generate output HiPack messages in their compact representation.
 */
    HIPACK_WRITER_COMPACT = -1,
/*~M HIPACK_WRITER_INDENTED
 * Flag to generate output HiPack messages in “indented” (pretty-printed)
 * representation.
 */
    HIPACK_WRITER_INDENTED = 0,
};

#define HIPACK_DEFINE_WRITE_VALUE(_type, name, type_tag) \
    extern bool hipack_write_ ## name (hipack_writer_t *writer, \
                                       const _type value);

HIPACK_TYPES (HIPACK_DEFINE_WRITE_VALUE)

#undef HIPACK_DEFINE_WRITE_VALUE

extern bool hipack_write_value (hipack_writer_t      *writer,
                                const hipack_value_t *value);

/*~f bool hipack_write (hipack_writer_t *writer, const hipack_dict_t *message)
 *
 * Writes a HiPack `message` to a stream `writer`, and returns whether writing
 * the message was successful.
 */
extern bool hipack_write (hipack_writer_t     *writer,
                          const hipack_dict_t *message);

/*~f int hipack_stdio_putchar (void* data, int ch)
 *
 * Writer function which uses ``FILE*`` objects from the standard C library.
 *
 * To use this function to write a message to a ``FILE*``, first open a file,
 * then create a writer using this function, and then use
 * :c:func:`hipack_write()`:
 *
 * .. code-block:: c
 *
 *    FILE* stream = fopen (HIPACK_FILE_PATH, "wb");
 *    hipack_writer_t writer = {
 *        .putchar = hipack_stdio_putchar,
 *        .putchar_data = stream,
 *    };
 *    hipack_write (&writer, message);
 *
 * The user is responsible for closing the ``FILE*`` after using it.
 */
extern int hipack_stdio_putchar (void* fp, int ch);

#endif /* !HIPACK_H */
