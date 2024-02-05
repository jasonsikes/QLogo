#! /bin/zsh

# This script is intended to run on MacOS, but will probably work in Linux.
# It will generate the set of icons for common Linux desktops.

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

dest="hicolor"

# The name of the icon files we are producing:
name="QLogo-GUI.png"

rm -rf "$dest"
mkdir -p "$dest"

for w in 128 16 24 32 48; do
    dims="${w}x${w}"
    dir="$dest/$dims/apps"
    p="$dir/$name"
    echo writing "$p"
    mkdir -p "$dir"
    convert "$source" -resize $dims -depth 32 "$p"
done

