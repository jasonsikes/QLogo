# This is README for QLogo.

![QLogo Icon](qlogo_logo.png)


## Downloading

I am not providing compiled versions of QLogo at this time. 

***

QLogo is an interpreter for the Logo language written in C++ using
Qt. It mimics (as much as I find reasonable) the UCBLogo
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

The help and library texts included in this package derive from UCBLogo under
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

Building QLogo requires Qt6.5 and CMake.

### To build in MacOS and Windows:

Simply open the CMakeLists.txt file in QtCreator and build within there. 

### To build in Linux:

If you have qtcreator, you can use qtcreator in Linux in the same manner as
in Windows and MacOS described above.

Otherwise, you can follow the standard CMake build procedure. First, create a
build directory somewhere outside of the QLogo source directory.

*In my case, since I keep my projects in a "Projects" directory under my home directory,
my QLogo directory can be found at "\~/Projects/QLogo". Thus, one place to create
my "build" would be adjacent to my QLogo directory, "\~/Projects/build".*

Then have CMake create the build structure.


```
$ cd ~/Projects
$ mkdir build
$ cmake -S QLogo -B build

```

Then enter into your build directory and issue ```make```, and, optionally, if
all goes well (it will) you can run ```make install```

```
$ cd build
$ make
$ sudo make install
```

This will give you two executables and supporting files:

1. "qlogo": this is the Logo interpreter that can be run from a script or command line.

2. "QLogo-GUI": this is the graphical user interface that will run qlogo and provides
the turtle and editor.

3. "qlogo_library.db": this is the SQLite database that stores the standard library.

4. "qlogo_help.db": this is the SQLite database that stores the help texts.

***

## Here are the nuances (they're mostly insignificant, I think):


* Colors can be specified in one of five ways:

   1. as a palette index (0 to 100), same as UCBLogo

   2. as a list of three percentages, one for each of red, green, blue `[0 0 0]` is
   black, `[100 100 100]` is white, also same as UCBLogo.
   
   3. as a list of **four** percentages, similar to Option 2 above, with the fourth
   value being "alpha". `100` is fully opaque, and `0` means fully transparent.

   4. as a named color from the X Color Database, e.g. `white` or `lemonchiffon`.
   The list of color names can be retrieved using the `ALLCOLORS` command or
   from the X Color database found here:
   https://en.wikipedia.org/wiki/X11_color_names
   
   5. as a hex triplet, preceded by "#"


* Changes in font properties (size, color, family) do not affect characters
  already printed. This enables multiple colors and fonts on the same console.
  

* QLogo does not look for nor automatically load `STARTUP.LG`.


* `COMMANDLINE` contains **ALL** of the parameters used to start qlogo instead
  of just the ones that appear after a hyphen. This is because I use QCommandLineParser
  to handle all of the command line arguments that begin with a hyphen.


* If `ERRACT` is set and its size is greater than zero, then any errors execute
  `PAUSE`. This was necessary because I couldn't find a reliable way to prevent
  infinite loops during error handling.
  

* Garbage collection is on-the-fly, meaning that memory is freed the moment a
  word/list/array is no longer needed. `GC` and `.SETSEGMENTSIZE` are no-ops.


* No scunching. UCBLogo provided a scrunch to compensate for older CRT screens
  with non-square pixels. This enabled turtle operations to maintain consistent
  physical height-width. The drawback is that some orientation queries are
  inaccurate. `SCRUNCH` and `SETSCRUNCH` are no-ops.


* `SAVEPICT` saves a copy of the canvas in the format given by the filename's
  extension. For example: `SAVEPICT "MY_PICTURE.PNG` will save in PNG
  format. QLogo can save an image in the following formats: BMP, JPG/JPEG,
  PNG, PPM, XBM, and XPM

* There is no facility yet for translation/internationalization.


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
algorithm can slow down window resizing. `FILLED` is still
available.

`EPSPICT`:

This is replaced by `SVGPICT`. See below.

`CSLSLOAD`:

Not implemented yet.

`SETCSLSLOC`:

Not implemented yet.

`SETEDITOR`:

The QLogo GUI has its own built-in editor. If you run the logo program from a
command line, such as in a terminal, no editor is available.

`SETLIBLOC`:

Not implemented. QLogo uses a SQLite database to store its standard library.
You can use the `setlibloc` command line parameter to tell qlogo where to find
the SQLite database if it is in a different location than where qlogo expects it.

`SETHELPLOC`:

Not implemented. QLogo uses a SQLite database to store its help text.
You can use the `sethelploc` command line parameter to tell qlogo where to find
the SQLite database if it is in a different location than where qlogo expects it.

`SETTEMPLOC`:

QLogo doesn't create temporary files.


### The following variables have no special meaning:

`REDEFP`:

Qt has strong support for internationalization, but in QLogo it is only partially implemented.
Internationalization will be supported soon.

`USEALTERNATENAMES`:

Qt has strong support for internationalization, but in QLogo it is only partially implemented.
Internationalization will be supported soon.


### The following commands are NEW:

`SVGPICT` has been added and is a replacement for `EPSPICT`. `SVGPICT` will
save the image on the canvas in Scalable Vector
Graphics format.

`ALLFONTS`:

Returns a list of all the fonts available on your system.

`ALLCOLORS`:

Returns a list of all the named colors that QLogo knows about.

`TIME`:

This is mostly for my own curiosity and for debugging. `TIME` will take one parameter,
a list, which it will execute. A timer will start when the list is executed and
then stop when the list is finished. The total running time of the list will be
printed. The output will be whatever the list outputs, if anything.

`MARK`:

This is for debugging memory management. `MARK` will take one parameter, set a
flag on it, and output that parameter. At the moment it is marked, a debugging
message will be printed out. Later, if/when the item is deleted, another
debugging message will be printed.

`CURSORINSERT`:

Sets cursor to insert mode in QLogo. This is the default.

`CURSOROVERWRITE`:

Sets cursor to overwrite mode in QLogo.

`CURSORMODE`:

Outputs either `INSERT` or `OVERWRITE`.

`STANDOUT`:

This works in the QLogo GUI by switching the font's foreground and background
colors. It isn't implemented for text terminals.

`SETBOUNDS`:

The drawing canvas in the QLogo GUI is designed to be device and resolution
independent. The user can stretch and resize the GUI window and its components
without needing interaction or permission from the QLogo program. Therefore,
the best way for the programmer to have control and the GUI to have responsiveness
is to set the bounds programatically. The GUI then can squeeze or stretch the
canvas to fit the window as needed.

The coordinate system of the drawing canvas is Cartesian: the Origin `[0,0]` is
always in the center. The range of the X-coordinate is between `-boundX` and
`boundX`. The range of the Y-coordinate is between `-boundY` and `boundY`. For
example, a bound set at `[350 150]` means that the turtle is visible if its
X-coordinate is between -350 and 350 and its Y-coordinate is between -150 and
150. See also `BOUNDS`.

`BOUNDS`:

Outputs a list of two numbers giving the maximum bounds (x,y)
of the canvas.
