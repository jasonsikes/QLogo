#! /bin/zsh

# This script is intended to run on MacOS, but will probably work in Linux.
# It will generate the icon file suitable for Windows.

# In order to prevent accidentally cobbering something, the path to the
# icons directory needs to be specified.
argn=$#

if [ $argn -ne 1 ] || ! [ -d "$1" ]; then
    echo Usage: $0 DIRECTORY
    exit 1
fi

cd "$1"

# It is assumed that the source file is:
source=qlogo_icon_1024.png

dest="QLogo-GUI.ico"

convert $source -resize 16x16   -depth 32 q16-32.png
convert $source -resize 32x32   -depth 32 q32-32.png
convert $source -resize 48x48   -depth 32 q48-32.png
convert $source -resize 256x256 -depth 32 q256-32.png

convert q16-32.png q32-32.png q48-32.png q256-32.png $dest
rm q16-32.png q32-32.png q48-32.png q256-32.png

