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

First of all, we need to include the ``hipack.h`` header in the source of our
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

Note how objects cretated by us, like the ``key`` string, have to be freed
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


Writing (serialization)
=======================


Values
======


Annotations
===========

