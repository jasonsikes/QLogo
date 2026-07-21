#!/bin/sh

# Serial test runner for Windows (Git Bash). Compare each .lg script output to its
# .expected file. No parallel make — slower, but reliable on Windows CI.
#
# Usage (from this directory):
#   ./windows_test.sh [FILENAMES...]
#
# If FILENAMES is omitted, all *.lg files in the tests directory are run.

test_dir=$(dirname "$0")
tests_dir=$(cd "$test_dir/tests" && pwd)
cd "$tests_dir" || exit 1

logo_path=""
for candidate in \
    "../../../qlogo/qlogo.exe" \
    "../../../qlogo/qlogo" \
    "../../../build/qlogo/qlogo.exe" \
    "../../../build/qlogo/qlogo"
do
    # Git Bash: .exe may exist but fail -x; only require -f.
    if [ -f "$candidate" ]; then
        logo_path="$candidate"
        break
    fi
done

if [ -z "$logo_path" ]; then
    if command -v qlogo.exe >/dev/null 2>&1; then
        logo_path=$(command -v qlogo.exe)
    elif command -v qlogo >/dev/null 2>&1; then
        logo_path=$(command -v qlogo)
    fi
fi

if [ -n "$logo_path" ]; then
    case $logo_path in
        /* | [A-Za-z]:*)
            ;;
        *)
            logo_dir=$(dirname "$logo_path")
            logo_base=$(basename "$logo_path")
            logo_path=$(cd "$logo_dir" && pwd)/$logo_base
            ;;
    esac
fi

if [ -z "$logo_path" ]; then
    echo "Error: could not find executable 'qlogo'."
    exit 1
fi

logo_dir=$(dirname "$logo_path")
echo "Using qlogo: $logo_path"

reported_tests=""
test_count=0

run_test() {
    f="$1"
    case $f in
        *.lg)
            echo "$f"
            test_count=$((test_count + 1))
            if [ ! -f "${f%.lg}.expected" ]; then
                echo "  missing expected file: ${f%.lg}.expected"
                reported_tests="$reported_tests$f
"
                return
            fi
            abs_lg="$tests_dir/$f"
            tmp_out=$(mktemp /tmp/qlogo_test_XXXXXX)
            (
                cd "$logo_dir" || exit 1
                "$logo_path" <"$abs_lg" >"$tmp_out" 2>&1
            )
            if ! diff "${f%.lg}.expected" "$tmp_out" >/dev/null 2>&1; then
                reported_tests="$reported_tests$f
"
                diff -u "${f%.lg}.expected" "$tmp_out" || true
            fi
            rm -f "$tmp_out"
            ;;
    esac
}

if [ $# -gt 0 ]; then
    for filename in "$@"; do
        run_test "$filename"
    done
else
    for a in *.lg; do
        [ -f "$a" ] || continue
        run_test "$a"
    done
fi

if [ -n "$reported_tests" ]; then
    echo
    echo "============================"
    echo "==== FAILED TESTS:"
    echo "===="
    echo "$reported_tests" | while IFS= read -r f; do
        [ -n "$f" ] || continue
        echo "==== $f"
        echo "===="
    done
    echo "============================"
fi

echo "$test_count tests."

if [ -n "$reported_tests" ]; then
    exit 1
fi
exit 0
