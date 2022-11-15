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
# in current directory.

filename="$1"

logo_binary=logo
logo_path="../$logo_binary"

failed_tests=()

run_test() {
    f="$1"
    ../logo < $f | diff "${f%.lg}.result" -
    if [ $? -eq 1 ]
    then
	failed_tests+=($f)
    fi
}

if [ ! -f "$logo_path" ]
then
    echo "Error: could not find '$logo_binary' in parent directory."
    echo "There should be a logo executable or a symbolic link in my parent diectory."
    exit 0
fi

if [ -n "$filename" ]
then
    run_test $filename
else
    for a in *.lg; do
	echo $a
	run_test $a
    done
fi

if (( ${#failed_tests[@]} )); then
    echo
    echo ============================
    echo ==== FAILED TESTS:
    echo ====
    for f in ${failed_tests[@]}
    do
	echo ==== $f
	echo ====
    done
    echo ============================
    exit 1
fi

exit 0
