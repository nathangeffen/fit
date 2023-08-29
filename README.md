# Function fitter

**WORK IN PROGRESS**

Fit is a GNU/Linux command line interface to optimize, minimize or fit
functions. It also provides a shared library and C++ header file.

Its primary purpose is for calibrating infectious disease models but hopefully
it is useful as a general function fitting/optimization program and library.

## Installation

Make sure these are installed on your system.
- Meson build system
- Boost C++ libraries

To build the executable and shared library, this will work on most systems:
- meson setup --buildtype=release release
- meson compile -C release
- cd release
- sudo ninja install

The executable is called *fit*.

The library is called *libfit.so*.


## Using the command line interface

TODO

## Using the C++ library

TODO

## License

The source code is licensed under the GNU General Public License version 3.


