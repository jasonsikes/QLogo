#!/usr/bin/env zsh

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

filenames=( "$@" )
argc="$#"

logo_binary=qlogo
logo_path="../qlogo/$logo_binary"
log_file=~/Documents/QLogo_test_runs.csv

failed_tests=()

run_test() {
    f="$1"
    if [[ $f == *.lg ]]
    then
	echo $f
	$logo_path < $f  2>&1 | diff "${f%.lg}.result" -
	if [ $? -eq 1 ]
	then
            failed_tests+=($f)
	fi
    fi
}

if [ ! -f "$logo_path" ]
then
    echo "Error: could not find '$logo_binary' in parent directory."
    echo "There should be a logo executable or a symbolic link in my parent diectory."
    exit 0
fi

if (( $argc > 0 ))
then
    for filename in ${filenames[*]}
    do
	run_test $filename
    done
else
    log_date=`date "+%Y-%m-%d %H:%M:%S,"`
    start_time=`date +%s`
    test_count=0
    for a in *.lg; do
	run_test $a
	((++test_count))
    done
    end_time=`date +%s`
    trt=$((end_time-start_time))
    echo ${log_date}${trt}, $test_count >> $log_file
fi

if (( ${#failed_tests[@]} )); then
    echo
    echo "============================"
    echo "====" FAILED TESTS:
    echo "===="
    for f in ${failed_tests[@]}
    do
	echo "====" $f
	echo "===="
    done
    echo "============================"
    exit 1
fi

exit 0
