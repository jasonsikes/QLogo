#! /bin/zsh -v

# This script was developed on MacOS, but will probably work in Linux.
# It will generate the set of icons for GitHub README, MacOS,
# Windows, and common Linux desktops.

# Get the directory containing this script. We will base everything else relative to it.
exePath=${0:a:h}

# Our work will be in the icons directory.
cd $exePath/../icons

# The icon source file:

source=qlogo_icon_1024.png


# Create the icon to include in the GitHub README:

convert $source -resize 192x192   -depth 32 "../qlogo_logo.png"

# Create the icon file for MacOS:

destdir="Psychi.iconset"

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
rm -rf "$destdir"


# Create the icon file for Windows:

# Windows icon goes into its 'resources' directory.
dest="../resources/Psychi.ico"

convert $source -resize 16x16   -depth 32 q16-32.png
convert $source -resize 32x32   -depth 32 q32-32.png
convert $source -resize 48x48   -depth 32 q48-32.png
convert $source -resize 256x256 -depth 32 q256-32.png

convert q16-32.png q32-32.png q48-32.png q256-32.png $dest
rm q16-32.png q32-32.png q48-32.png q256-32.png


# Create the icons for Linux:

dest="hicolor"

# The name of the icon files we are producing:
name="Psychi.png"

rm -rf "$dest"
mkdir -p "$dest"

for w in 128 16 24 32 48; do
    dims="${w}x${w}"
    dir="$dest/$dims/apps"
    p="$dir/$name"
    mkdir -p "$dir"
    convert "$source" -resize $dims -depth 32 "$p"
done

