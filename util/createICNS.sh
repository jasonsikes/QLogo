#! /bin/zsh

# This script is intended to run on MacOS.
# It will generate the icon file suitable for MacOS.

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

destdir="QLogo-GUI.iconset"

mkdir "$destdir"
sips -z 16 16     "$source" --out "$destdir/icon_16x16.png"
sips -z 32 32     "$source" --out "$destdir/icon_16x16@2x.png"
sips -z 32 32     "$source" --out "$destdir/icon_32x32.png"
sips -z 64 64     "$source" --out "$destdir/icon_32x32@2x.png"
sips -z 128 128   "$source" --out "$destdir/icon_128x128.png"
sips -z 256 256   "$source" --out "$destdir/icon_128x128@2x.png"
sips -z 256 256   "$source" --out "$destdir/icon_256x256.png"
sips -z 512 512   "$source" --out "$destdir/icon_256x256@2x.png"
sips -z 512 512   "$source" --out "$destdir/icon_512x512.png"
cp "$source" "$destdir/icon_512x512@2x.png"
iconutil -c icns "$destdir"
rm -R "$destdir"
