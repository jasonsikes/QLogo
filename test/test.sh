#!/usr/bin/env sh

# Run this while in the test directory.
# ./test.sh [FILENAME]
#
# Run test(s) using FILENAME as input. Compare the output to the file
# with the same filename except with the ".out" extension.
#
# FILENAME is the optional name of the logo script WITH the .lg extension.
#
# If FILENAME is not specified then this will go through all the .lg files
# in this directory.

filename="$1"

run_test() {
    f="$1"
    ../logo < $f | diff "${f%.lg}.out" -
}

if [ -n "$filename" ]
then
    run_test $filename
    exit 0
fi

for a in *.lg; do
    echo $a
    run_test $a
done
