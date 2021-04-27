MLT FRAMEWORK README
--------------------

MLT is a LGPL multimedia framework designed for video editing.

This document provides a quick reference for the minimal configuration, build and
installation of MLT.

See the `docs/` directory for usage details.

See the [website](https://www.mltframework.org/docs/) for development details
and a [contributing](https://www.mltframework.org/docs/contributing/) guide.


Configuration
-------------

Configuration is triggered by running:

    cmake .

More information on usage is found by viewing `CMakeLists.txt` and the [cmake man page](https://cmake.org/cmake/help/latest/manual/cmake.1.html).

Compilation
-----------

Once configured, it should be sufficient to run:

    cmake --build

to compile the system. Alternatively, you can run `cmake` with `-G Ninja` and use ninja to compile.


Testing
-------

To execute the mlt tools without installation, or to test a new version
on a system with an already installed mlt version, you should run:

    . setenv

NB: This applies to your current shell only and it assumes a bash or
regular bourne shell is in use.


Installation
------------

The install is triggered by running:

    cmake --install


More Information
----------------

For more detailed information, please refer to https://mltframework.org/docs/install/.
