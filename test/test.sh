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

failed_tests=()

run_test() {
    f="$1"
    ../logo < $f | diff "${f%.lg}.out" -
    if [ $? -eq 1 ]
    then
	failed_tests+=($f)
    fi
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

if (( ${#failed_tests[@]} )); then
    echo
    echo FAILED TESTS:
    for f in ${failed_tests[@]}
    do
	echo $f
    done
    exit 1
fi

exit 0
