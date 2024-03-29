# This is README for QLogo.

![QLogo Icon](qlogo_logo.png)


## Downloading

I am not providing compiled versions of QLogo at this time. Windows
and Macos systems require me to sign the binaries. This is a
good thing. I still have yet to learn how to do that.

***

QLogo is an interpreter for the Logo language written in C++ using
Qt and OpenGL. It mimics (as much as I find reasonable) the UCBLogo
interpreter developed by Brian Harvey at U.C. Berkeley.

Copyright (C) 2017-2024 Jason Sikes

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

***

The help files included in this package derive from UCBLogo under
terms of the GNU General Public License as published by the Free
Software Foundation, version 3.

You should have received a copy of the GNU General Public License along
with this program. If not, see https://www.gnu.org/licenses/.

***

You can find the UCBLogo Manual here:

http://people.eecs.berkeley.edu/~bh/usermanual

The differences between QLogo and UCBLogo are described in the
**nuances** section below. There aren't many.

***


## Compiling

Building QLogo requires Qt6 and CMake.

### To build in MacOS and Windows:

Simply open the CMakeLists.txt file in QtCreator and build within there. 

### To build in Linux:

If you have qtcreator, you can use qtcreator in Linux in the same mannor as
in Windows and MacOS described above.

Otherwise, you can follow the standard CMake build procedure. First, create a
build directory somewhere outside of the QLogo source directory.

*In my case, since I keep my projects in a "Projects" directory under my home,
my QLogo directory can be found at "\~/Projects/QLogo". Thus, one place to create
my "build" would be adjacent to my QLogo directory, "\~/Projects/build".*

Then have CMake create the build structure.


```

$ cd ~/Projects

$ mkdir build

$ cmake -S QLogo -B build

```

Then enter into your build directory and issue ```make```, and if all goes well,
you can run ```make install```

```

$ cd build

$ make

$ sudo make install
```

This will give you two executables:

1. "qlogo": this is the Logo interpreter that can be run from a script or command line.

2. "QLogo-GUI": this is the graphical user interface that will run qlogo and provides
the turtle.

***

## Here are the nuances (they're mostly insignificant, I think):


* Colors can be specified in one of three ways:

   1. as a palette index (0 to 100), same as UCBLogo

   2. as a list of 3 percentages, one for each of red, green, blue `[0 0 0]` is
   black, `[100 100 100]` is white, also same as UCBLogo

   3. as a named color from the X Color Database, e.g. `white` or `lemonchiffon`.
   The X Color database can be found here:
   https://en.wikipedia.org/wiki/X11_color_names


* Changes in font properties (size, color, family) do not affect characters
  already printed. This enables multiple colors and fonts on the same console.
  

* The entire Logo standard library is loaded internally and buried. There is
  no library directory at this time.


* QLogo does not look for nor automatically load `STARTUP.LG`.


* If `ERRACT` is set and its size is greater than zero, then any errors execute
  `PAUSE`. This was necessary because I couldn't find a reliable way to prevent
  infinite loops during error handling.
  

* Garbage collection is on-the-fly, meaning that memory is freed the moment a
  word/list/array is no longer needed. `GC` and `.SETSEGMENTSIZE` are no-ops.


* No scunching. UCBLogo provided a scrunch to compensate for older CRT screens
  with non-square pixels. This enabled turtle operations to maintain consistent
  physical height-width. `SCRUNCH` and `SETSCRUNCH` are no-ops.


* `SAVEPICT` saves a copy of the canvas in the format given by the filename's
  extension. For example: `SAVEPICT "MY_PICTURE.PNG` will save in PNG
  format. QLogo can save an image in the following formats: BMP, JPG/JPEG,
  PNG, PPM, XBM, and XPM


* There is no facility yet for translation/internationalization. Yet.


### The following commands are not implemented:

`SETMARGINS`:

The original purpose of the command was to enable text to
be visible on projectors which cut off outer boundaries of
a computer screen. Projectors and monitors produced in the
last few years show all of the computer screen. In addition,
QLogo is a windowed application so an instructor or presentor
can move the window to a different position.


`FILL`:

Two reasons: One of the user interface principles for QLogo is that
the canvas should be device resolution-independent. That
means when the QLogo window is resized or the separator
between the text and the graphics is moved the graphics
will be redrawn with the new dimensions. The Flood Fill
algorithm depends on specific pixels which means that the
display can change dramatically depending on the size of
the canvas. The other reason is that the Flood Fill
algorithm can slow down window resizing. FILLED is still
available.

`LOADPICT`:

This will be implemented soon.

`EPSPICT`:

This will be replaced by SVGPICT.

`CSLSLOAD`:

Not implemented yet.

`SETCSLSLOC`:

Not implemented yet.

`SETEDITOR`:

QLogo uses a built-in editor. If you run the logo program on the
command line, no editor is available.

`SETLIBLOC`:

The QLogo library is stored internally.

`SETHELPLOC`:

Not implemented. QLogo uses a SQLite database to store help text.

`SETTEMPLOC`:

QLogo doesn't create temporary files at this time.


### The following variables have no special meaning:

`COMMANDLINE`:

Not implemented yet.

`REDEFP`:

Qt has strong support for internationalization, but QLogo
is not designed at this time to take advantage of
that. Internationalization will be supported soon.

`USEALTERNATENAMES`:

Ditto.


### The following commands are NEW:

`ALLFONTS`:

Returns a list of all the fonts available on your system.

`CURSORINSERT`:

Sets cursor to insert mode in QLogo. This is the default.

`CURSOROVERWRITE`:

Sets cursor to overwrite mode in QLogo.

`CURSORMODE`:

Outputs either `INSERT` or `OVERWRITE`.

`STANDOUT`:

This works in QLogo by switching the font's foreground and background
colors. It isn't implemented for text terminals.

`BOUNDS`:

Outputs a list of two numbers giving the maximum bounds (x,y)
of the canvas.  e.g. bounds of [350 150] means that the
turtle is visible if its X-coordinate is between -350 and 350
and its Y-coordinate is between -150 and 150. The coordinate
[0, 0] is always in the center. See `SETBOUNDS`

`SETBOUNDS`:

Takes two integers and sets outer bounds of the canvas.
See `BOUNDS`.

`MATRIX`:

This exists for debugging and may be removed. Outputs a
list of four lists, each sublist contains four numbers. This
is the matrix that represents the current turtle state. In
row-major order.

