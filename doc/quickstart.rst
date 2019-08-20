==========
Quickstart
==========

This guide will walk you through the usage of the ``hipack-c`` C library.

.. note:: All the examples in this guide need a compiler which supports C99.


Reading (deserialization)
=========================

Let's start by parsing some data from the file which contains a HiPack message
like the following::

    title: "Quickstart"
    is-documentation? True

First, we need to include the ``hipack.h`` header in the source of our
program, and then use a :c:func:`hipack_read()` to parse it. In this example
we use the :c:func:`hipack_stdio_getchar()` function included in the library
which is used to read data from a ``FILE*``:

.. code-block:: c

   #include <hipack.h>
   #include <stdio.h>   /* Needed for FILE* streams */

   static void handle_message (hipack_dict_t *message);

   int main (int argc, const char *argv[]) {
       hipack_reader_t reader = {
           .getchar_data = fopen ("input.hipack", "rb"),
           .getchar = hipack_stdio_getchar,
       };
       hipack_dict_t *message = hipack_read (&reader);
       fclose (reader.getchar_data);

       handle_message (message); /* Use "message". */

       hipack_dict_free (message);
       return 0;
   }

Once the message was parsed, a dictionary (:c:type:`hipack_dict_t`) is
returned. The :ref:`dict_funcs` can be used to inspect the message. For
example, adding the following as the ``handle_message()`` function
prints the value of the ``title`` element:

.. code-block:: c

   void handle_message (hipack_dict_t *message) {
       hipack_string_t *key = hipack_string_new_from_string ("title");
       hipack_value_t *value = hipack_dict_get (message, key);

       printf ("Title is '%s'\n", hipack_value_get_string (value));

       hipack_string_free (key); // Free memory used by "key".
   }

Note how objects created by us, like the ``key`` string, have to be freed
by us. In general, the user of the library is responsible for freeing any
objects created by them. On the other hand, objects allocated by the library
are freed by library functions.

In our example, the memory area which contains the ``value`` is owned by the
message (more precisely: by the dictionary that represents the message), and
the call to :c:func:`hipack_value_get()` returns a pointer to the memory area
owned by the message. The same happens with the call to
:c:func:`hipack_value_get_string()`: it returns a pointer to a memory area
containing the bytes of the string value, which is owned by the
:c:type:`hipack_value_t` structure. This means that we *must not* free the
value structure or the string, because when the whole message is freed —by
calling :c:func:`hipack_dict_free()` at the end of our ``main()`` function—
the memory areas used by the value structure and the string will be freed as
well.


Values
======

Objects of type :c:type:`hipack_value_t` represent a single value of those
supported by HiPack: an integer number, a floating point number, a boolean,
a list, or a dictionary. Creating an object if a “basic” value, that is all
except lists and dictionary, can be done using the C99 designated initializer
syntax. For example, to create a floating point number value:

.. code-block:: c

   hipack_value_t flt_val = {
       .type = HIPACK_FLOAT,
       .v_float = 4.5e-1
   };

Alternatively, it is also possible to use provided utility functions to
create values. The example above is equivalent to:

.. code-block:: c

   hipack_value_t flt_val = hipack_float (4.5e-1);

When using the C99 initializer syntax directly, the name of the field
containing the value depends on the type. The following table summarizes the
equivalence between types, the field to use in :c:type:`hipack_value_t`, and
the utility function for constructing values:

================== ============== ==============================
Type               Field          Macro
================== ============== ==============================
``HIPACK_INTEGER`` ``.v_integer`` :c:func:`hipack_integer()`
``HIPACK_FLOAT``   ``.v_float``   :c:func:`hipack_float()`
``HIPACK_BOOL``    ``.v_bool``    :c:func:`hipack_bool()`
``HIPACK_STRING``  ``.v_string``  :c:func:`hipack_string()`
``HIPACK_LIST``    ``.v_list``    :c:func:`hipack_list()`
``HIPACK_DICT``    ``.v_dict``    :c:func:`hipack_dict()`
================== ============== ==============================

Note that strings, lists, and dictionaries may have additional memory
allocated. When wrapping them into a :c:type:`hipack_value_t` only the pointer
is stored, and it is still your responsibility to make sure the memory is
freed when the values are not used anymore:

.. code-block:: c

   hipack_string_t *str = hipack_string_new_from_string ("spam");
   hipack_value_t str_val = hipack_string (str);
   assert (str == hipack_value_get_string (&str_val)); // Always true

For convenience, a :c:func:`hipack_value_free()` function which will ensure
the memory allocated by values referenced by a :c:type:`hipack_value_t` will
be freed properly. This way, one can write code in which the value objects
(:c:type:`hipack_value_t`) are considered to be the owners of the memory which
has been dynamically allocated:

.. code-block:: c

   // Ownership of the hipack_string_t is passed to "str_val"
   hipack_value_t str_val =
       hipack_string (hipack_string_new_from_string ("spam"));
   // …
   hipack_value_free (&str_val);  // Free memory.

This behaviour is particularly handy when assembling complex values: all items
contained in lists and dictionaries can be just added to them, and when
freeing the container, all the values in them will be freed as well. Consider
the following function which creates a HiPack message with a few fields:

.. code-block:: c

   /*
    * Creates a message which would serialize as:
    *
    *    street: "Infinite Loop"
    *    lat: 37.332
    *    lon: -122.03
    *    number: 1
    */
   hipack_dict_t* make_address_message (void) {
       hipack_dict_t *message = hipack_dict_new ();
       hipack_dict_set_adopt_key (message,
           hipack_string_new_from_string ("lat"),
           hipack_float (37.332));
       hipack_dict_set_adopt_key (message,
           hipack_string_new_from_string ("lon"),
           hipack_float (-122.03));
       hipack_dict_set_adopt_key (message,
           hipack_string_new_from_string ("street"),
           hipack_string (
               hipack_string_new_from_string ("Infinite Loop")));
       hipack_dict_set_adopt_key (message,
           hipack_string_new_from_string ("number"),
           hipack_integer (1));
       return message;
   }

Note how it is not needed to free any of the values because they are “owned”
by the dictionary, which gets returned from the function. Also, we use
:c:func:`hipack_dict_set_adopt_key()` (instead of :c:func:`hipack_dict_set()`)
to pass ownership of the keys to the dictionary as well and, at the same time,
avoiding creating copies of the strings. The caller of this function would be
the owner of the memory allocated by the returned dictionary and all its
contained values.



Writing (serialization)
=======================

In order to serialize messages, we need a dictionary containing the values
which we want to have serialized — remember that any dictionary can be
considered a HiPack message. In order to serialize a message, we use
:c:func:`hipack_write()`, and in particular using
:c:func:`hipack_stdio_putchar()` we can directly write to a ``FILE*`` stream.
Given the ``make_address_message()`` function from the previous section:

.. code-block:: c

   int main (int argc, char *argv[]) {
       hipack_dict_t *message = make_address_message ();
       hipack_writer_t writer = {
           .putchar_data = fopen ("address.hipack", "wb"),
           .putchar = hipack_stdio_putchar,
       };
       hipack_write (&writer, message);
       hipack_dict_free (message);
       return 0;
   }

The resulting ``address.hipack`` file will have the following contents (the
order of the elements may vary)::

    number: 1
    street: "Infinite Loop"
    lat: 37.332
    lon: -122.03

