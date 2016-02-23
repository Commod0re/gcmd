====
gcmd
====

a gawk extension providing an input parser that executes a command on the current file instead of reading it directly.


Installation
------------

To install gcmd, simply:

.. code-block:: bash

    $ make
    $ make install


Usage
-----

Using ``gcmd`` is simple. In your gawk script, you'll need to load ``gcmd``, and then set one or two variables, depending on how complex a command you'd like to invoke.

``gcmd`` uses two variables for control: ``GCMD`` and ``GCRS``.

``GCMD`` provides the commandline you'd like gawk to execute. The default value is "", which causes ``gcmd`` not to process anything.

``GCRS`` provides a replacement string. When getting ready to execute the command against the current file, the value of ``GCRS`` is replaced with the current ``FILENAME``. If the value of ``GCRS`` is not in ``GCMD``, then ``FILENAME`` will be appended to ``GCMD``.

A simple example::

    #!/usr/bin/gawk -f
    @load "gcmd"
    BEGIN {
        # the resulting commandline will be "sort FILENAME"
        GCMD = "sort"
    }

    # write the rest of your gawk script as normal
    ...

It's possible to change commands between files by specifying a new value for ``GCMD`` within the ``BEGINFILE`` block::

    #!/usr/bin/gawk -f
    @load "gcmd"
    BEGINFILE {
        GCMD = ""
        if (FILENAME == "Makefile") {
            GCMD = "make -Bnp : -f"
            RS=""
            FS="\n"
        }
    }
    # print a list of targets in the Makefile given
    /^# Files/,/^# files hash-table/ {
        if ($1 != "# Not a target:") {
            print $1
        }
    }
    ...

If you have a more complex commandline, making use of GCRS can be helpful::

    #!/usr/bin/gawk -f
    @load "gcmd"
    BEGIN {
        # GCRS = "{}" by default, 
        # so the resulting commandline will be "make -Bnp : -f <FILENAME> 2>/dev/null"
        GCMD = "make -Bnp : -f {} 2>/dev/null"
    }
    ...

Notes
-----

Some things to watch out for:

- ``GCMD`` is only processed when a file is being opened, so changing the value after gawk has started processing the file will have no effect until the next file is opened.

- Currently, ``gcmd`` cannot handle passing input from stdin to commands, so it will not even try.

Requirements
------------

- `gawk <http://savannah.gnu.org/projects/gawk/>`_ >= 4.1.0

License
-------

BSD 3-Clause licensed. See the bundled `LICENSE <blob/master/LICENSE.rst>`_ file for more details.