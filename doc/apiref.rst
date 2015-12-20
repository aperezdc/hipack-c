


=============
API Reference
=============

Types
=====

.. c:type:: hipack_type_t


   Type of a value. This enumeration takes one of the following values:

   - ``HIPACK_INTEGER``: Integer value.
   - ``HIPACK_FLOAT``: Floating point value.
   - ``HIPACK_BOOL``: Boolean value.
   - ``HIPACK_STRING``: String value.
   - ``HIPACK_LIST``: List value.
   - ``HIPACK_DICT``: Dictionary value.

.. c:type:: hipack_value_t


   Represent any valid HiPack value.

   - :func:`hipack_value_type()` obtains the type of a value.

.. c:type:: hipack_string_t


   String value.

.. c:type:: hipack_list_t


   List value.

.. c:type:: hipack_dict_t


   Dictionary value.



Memory Allocation
=================

How ``hipack-c`` allocates memory can be customized by setting
:c:data:`hipack_alloc` to a custom allocation function.

.. c:var:: hipack_alloc


   Allocation function. By default it is set to :func:`hipack_alloc_stdlib()`,
   which uses the implementations of ``malloc()``, ``realloc()``, and
   ``free()`` provided by the C library.

   Allocation functions always have the following prototype:

   .. code-block:: c

      void* func (void *oldptr, size_t size);

   The behavior must be as follows:

   - When invoked with ``oldptr`` set to ``NULL``, and a non-zero ``size``,
     the function behaves like ``malloc()``: a memory block of at least
     ``size`` bytes is allocated and a pointer to it returned.
   - When ``oldptr`` is non-``NULL``, and a non-zero ``size``, the function
     behaves like ``realloc()``: the memory area pointed to by ``oldptr``
     is resized to be at least ``size`` bytes, or its contents moved to a
     new memory area of at least ``size`` bytes. The returned pointer may
     either be ``oldptr``, or a pointer to the new memory area if the data
     was relocated.
   - When ``oldptr`` is non-``NULL``, and ``size`` is zero, the function
     behaves like ``free()``.

.. c:function:: void* hipack_alloc_stdlib(void*, size_t)


   Default allocation function. It uses ``malloc()``, ``realloc()``, and
   ``free()`` from the C library. By default :any:`hipack_alloc` is set
   to use this function.

.. c:function:: void* hipack_alloc_array_extra (void *oldptr, size_t nmemb, size_t size, size_t extra)


   Allocates (if `oldptr` is ``NULL``) or reallocates (if `oldptr` is
   non-``NULL``) memory for an array which contains `nmemb` elements, each one
   of `size` bytes, plus an arbitrary amount of `extra` bytes.

   This function is used internally by the HiPack parser, and it is not likely
   to be needed by client code.

.. c:function:: void* hipack_alloc_array (void *optr, size_t nmemb, size_t size)


   Same as :c:func:`hipack_alloc_array_extra()`, without allowing to specify
   the amount of extra bytes. The following calls are both equivalent:

   .. code-block:: c

      void *a = hipack_alloc_array_extra (NULL, 10, 4, 0);
      void *b = hipack_alloc_array (NULL, 10, 4);

   See :c:func:`hipack_alloc_array_extra()` for details.

.. c:function:: void* hipack_alloc_bzero (size_t size)


   Allocates an area of memory of `size` bytes, and initializes it to zeroes.

.. c:function:: void hipack_alloc_free (void *pointer)


   Frees the memory area referenced by the given `pointer`.



String Functions
================

The following functions are provided as a convenience to operate on values
of type :c:type:`hipack_string_t`.

.. note:: The hash function used by :c:func:`hipack_string_hash()` is
   *not* guaranteed to be cryptographically safe. Please do avoid exposing
   values returned by this function to the attack surface of your
   applications, in particular *do not expose them to the network*.

.. c:function:: hipack_string_t* hipack_string_copy (const hipack_string_t *hstr)


   Returns a new copy of a string.

   The returned value must be freed using :c:func:`hipack_string_free()`.

.. c:function:: hipack_string_t* hipack_string_new_from_string (const char *str)


   Creates a new string from a C-style zero terminated string.

   The returned value must be freed using :c:func:`hipack_string_free()`.

.. c:function:: hipack_string_t* hipack_string_new_from_lstring (const char *str, uint32_t len)


   Creates a new string from a memory area and its length.

   The returned value must be freed using :c:func:`hipack_string_free()`.

.. c:function:: uint32_t hipack_string_hash (const hipack_string_t *hstr)


   Calculates a hash value for a string.

.. c:function:: bool hipack_string_equal (const hipack_string_t *hstr1, const hipack_string_t *hstr2)

   Compares two strings to check whether their contents are the same.

.. c:function:: void hipack_string_free (hipack_string_t *hstr)

   Frees the memory used by a string.



List Functions
==============

.. c:function:: hipack_list_t* hipack_list_new (uint32_t size)

   Creates a new list for ``size`` elements.

.. c:function:: void hipack_list_free (hipack_list_t *list)

   Frees the memory used by a list.

.. c:function:: bool hipack_list_equal (const hipack_list_t *a, const hipack_list_t *b)

   Checks whether two lists contains the same values.

.. c:function:: uint32_t hipack_list_size (const hipack_list_t *list)

   Obtains the number of elements in a list.

.. c:macro:: HIPACK_LIST_AT(list, index)


   Obtains a pointer to the element at a given `index` of a `list`.



.. _dict_funcs:

Dictionary Functions
====================

.. c:function:: uint32_t hipack_dict_size (const hipack_dict_t *dict)


   Obtains the number of elements in a dictionary.

.. c:function:: hipack_dict_t* hipack_dict_new (void)


   Creates a new, empty dictionary.

.. c:function:: void hipack_dict_free (hipack_dict_t *dict)


   Frees the memory used by a dictionary.

.. c:function:: bool hipack_dict_equal (const hipack_dict_t *a, const hipack_dict_t *b)


   Checks whether two dictinaries contain the same keys, and their associated
   values in each of the dictionaries are equal.

.. c:function:: void hipack_dict_set (hipack_dict_t *dict, const hipack_string_t *key, const hipack_value_t *value)


   Adds an association of a `key` to a `value`.

   Note that this function will copy the `key`. If you are not planning to
   continue reusing the `key`, it is recommended to use
   :c:func:`hipack_dict_set_adopt_key()` instead.

.. c:function:: void hipack_dict_set_adopt_key (hipack_dict_t *dict, hipack_string_t **key, const hipack_value_t *value)


   Adds an association of a `key` to a `value`, passing ownership of the
   memory using by the `key` to the dictionary (i.e. the string used as key
   will be freed by the dictionary).

   Use this function instead of :c:func:`hipack_dict_set()` when the `key`
   is not going to be used further afterwards.

.. c:function:: void hipack_dict_del (hipack_dict_t *dict, const hipack_string_t *key)


   Removes the element from a dictionary associated to a `key`.

.. c:function:: hipack_value_t* hipack_dict_get (const hipack_dict_t *dict, const hipack_string_t *key)


   Obtains the value associated to a `key` from a dictionary.

   The returned value points to memory owned by the dictionary. The value can
   be modified in-place, but it shall not be freed.

.. c:function:: hipack_value_t* hipack_dict_first (const hipack_dict_t *dict, const hipack_string_t **key)


   Obtains an a *(key, value)* pair, which is considered the *first* in
   iteration order. This can be used in combination with
   :c:func:`hipack_dict_next()` to enumerate all the *(key, value)* pairs
   stored in the dictionary:

   .. code-block:: c

      hipack_dict_t *d = get_dictionary ();
      hipack_value_t *v;
      hipack_string_t *k;

      for (v = hipack_dict_first (d, &k);
           v != NULL;
           v = hipack_dict_next (v, &k)) {
          // Use "k" and "v" normally.
      }

   As a shorthand, consider using :c:macro:`HIPACK_DICT_FOREACH()` instead.

.. c:function:: hipack_value_t* hipack_dict_next (hipack_value_t *value, const hipack_string_t **key)


   Iterates to the next *(key, value)* pair of a dictionary. For usage
   details, see :c:func:`hipack_dict_first()`.

.. c:macro:: HIPACK_DICT_FOREACH(dict, key, value)


   Convenience macro used to iterate over the *(key, value)* pairs contained
   in a dictionary. Internally this uses :c:func:`hipack_dict_first()` and
   :c:func:`hipack_dict_next()`.

   .. code-block:: c

      hipack_dict_t *d = get_dictionary ();
      hipack_string_t *k;
      hipack_value_t *v;
      HIPACK_DICT_FOREACH (d, k, v) {
          // Use "k" and "v"
      }

   Using this macro is the recommended way of writing a loop to enumerate
   elements from a dictionary.



Value Functions
===============

.. c:function:: hipack_type_t hipack_value_type (const hipack_value_t *value)


   Obtains the type of a value.

.. c:function:: bool hipack_value_is_integer (const hipack_value_t *value)

   Checks whether a value is an integer.

.. c:function:: bool hipack_value_is_float (const hipack_value_t *value)

   Checks whether a value is a floating point number.

.. c:function:: bool hipack_value_is_bool (const hipack_value_t *value)

   Checks whether a value is a boolean.

.. c:function:: bool hipack_value_is_string (const hipack_value_t *value)

   Checks whether a value is a string.

.. c:function:: bool hipack_value_is_list (const hipack_value_t *value)

   Checks whether a value is a list.

.. c:function:: bool hipack_value_is_dict (const hipack_value_t *value)

   Checks whether a value is a dictionary.

.. c:function:: const int32_t hipack_value_get_integer (const hipack_value_t *value)

   Obtains a numeric value as an ``int32_t``.

.. c:function:: const double hipack_value_get_float (const hipack_value_t *value)

   Obtains a floating point value as a ``double``.

.. c:function:: const bool hipack_value_get_bool (const hipack_value_t *value)

   Obtains a boolean value as a ``bool``.

.. c:function:: const hipack_string_t* hipack_value_get_string (const hipack_value_t *value)

   Obtains a numeric value as a :c:type:`hipack_string_t`.

.. c:function:: const hipack_list_t* hipack_value_get_list (const hipack_value_t *value)

   Obtains a numeric value as a :c:type:`hipack_list_t`.

.. c:function:: const hipack_dict_t* hipack_value_get_dict (const hipack_value_t *value)

   Obtains a numeric value as a :c:type:`hipack_dict_t`.

.. c:function:: bool hipack_value_equal (const hipack_value_t *a, const hipack_value_t *b)


   Checks whether two values are equal.

.. c:function:: void hipack_value_free (hipack_value_t *value)


   Frees the memory used by a value.

.. c:function:: void hipack_value_add_annot (hipack_value_t *value, const char *annot)


   Adds an annotation to a value. If the value already had the annotation,
   this function is a no-op.

.. c:function:: bool hipack_value_has_annot (const hipack_value_t *value, const char *annot)


   Checks whether a value has a given annotation.

.. c:function:: void hipack_value_del_annot (hipack_value_t *value, const char *annot)


   Removes an annotation from a value. If the annotation was not present, this
   function is a no-op.



Reader Interface
================

.. c:type:: hipack_reader_t


   Allows communicating with the parser, instructing it how to read text
   input data, and provides a way for the parser to report errors back.

   The following members of the structure are to be used by client code:

   .. c:member:: int (*getchar)(void *data)

      Reader callback function. The function will be called every time the
      next character of input is needed. It must return it as an integer,
      :any:`HIPACK_IO_EOF` when trying to read pas the end of the input,
      or :any:`HIPACK_IO_ERROR` if an input error occurs.


   .. c:member:: const char *error

      On error, a string describing the issue, suitable to be displayed to
      the user.

   .. c:member:: unsigned error_line

      On error, the line number where parsing was stopped.

   .. c:member:: unsigned error_column

      On error, the column where parsing was stopped.

.. c:macro:: HIPACK_IO_EOF

   Constant returned by reader functions when trying to read past the end of
   the input.

.. c:macro:: HIPACK_IO_ERROR

   Constant returned by reader functions on input errors.

.. c:macro:: HIPACK_READ_ERROR


   Constant value used to signal an underlying input error.

   The `error` field of :c:type:`hipack_reader_t` is set to this value when
   the reader function returns :any:`HIPACK_IO_ERROR`. This is provided to
   allow client code to detect this condition and further query for the
   nature of the input error.

.. c:function:: hipack_dict_t* hipack_read (hipack_reader_t *reader)


   Reads a HiPack message from a stream `reader` and returns a dictionary.

   On error, ``NULL`` is returned, and the members `error`, `error_line`,
   and `error_column` (see :c:type:`hipack_reader_t`) are set accordingly
   in the `reader`.

.. c:function:: int hipack_stdio_getchar (void* fp)


   Reader function which uses ``FILE*`` objects from the standard C library.

   To use this function to read from a ``FILE*``, first open a file, and
   then create a reader using this function and the open file as data to
   be passed to it, and then use :c:func:`hipack_read()`:

   .. code-block:: c

      FILE* stream = fopen (HIPACK_FILE_PATH, "rb")
      hipack_reader_t reader = {
          .getchar = hipack_stdio_getchar,
          .getchar_data = stream,
      };
      hipack_dict_t *message = hipack_read (&reader);

   The user is responsible for closing the ``FILE*`` after using it.



Writer Interface
================

.. c:type:: hipack_writer_t


   Allows specifying how to write text output data, and configuring how
   the produced HiPack output looks like.

   The following members of the structure are to be used by client code:

   .. c:member:: int (*putchar)(void *data, int ch)

      Writer callback function. The function will be called every time a
      character is produced as output. It must return :any:`HIPACK_IO_ERROR`
      if an output error occurs, and it is invalid for the callback to
      return :any:`HIPACK_IO_EOF`. Any other value is interpreted as
      indication of success.

   .. c:member:: void* putchar_data

      Data passed to the writer callback function.

   .. c:member:: int32_t indent

      Either :any:`HIPACK_WRITER_COMPACT` or :any:`HIPACK_WRITER_INDENTED`.

.. c:macro:: HIPACK_WRITER_COMPACT

   Flag to generate output HiPack messages in their compact representation.

.. c:macro:: HIPACK_WRITER_INDENTED

   Flag to generate output HiPack messages in “indented” (pretty-printed)
   representation.

.. c:function:: bool hipack_write (hipack_writer_t *writer, const hipack_dict_t *message)


   Writes a HiPack `message` to a stream `writer`, and returns whether writing
   the message was successful.

.. c:function:: int hipack_stdio_putchar (void* data, int ch)


   Writer function which uses ``FILE*`` objects from the standard C library.

   To use this function to write a message to a ``FILE*``, first open a file,
   then create a writer using this function, and then use
   :c:func:`hipack_write()`:

   .. code-block:: c

      FILE* stream = fopen (HIPACK_FILE_PATH, "wb");
      hipack_writer_t writer = {
          .putchar = hipack_stdio_putchar,
          .putchar_data = stream,
      };
      hipack_write (&writer, message);

   The user is responsible for closing the ``FILE*`` after using it.

