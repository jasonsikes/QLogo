# Unix terminal-signal tests. Sourced by test.sh.
#
# Required variables:
#   logo_path     Absolute path to the qlogo binary
#   console_dir   tests/console directory
#
# Optional variables:
#   stop_on_first_failure
#
# Mutates: test_count, reported_tests

run_signal_tests() {
    case "$(uname -s)" in
        Linux|Darwin)
            ;;
        *)
            return 0
            ;;
    esac

    if ! command -v python3 >/dev/null 2>&1; then
        echo "Skipping signal tests: python3 not found"
        return 0
    fi

    signals_dir="$console_dir/signals"
    echo "signals: running PTY cases"
    test_count=$((test_count + 12))

    tmp_out=$(mktemp /tmp/qlogo_signals_XXXXXX)
    python3 "$signals_dir/run_signals.py" "$logo_path" >"$tmp_out" 2>&1
    status=$?
    cat "$tmp_out"
    if [ "$status" -ne 0 ]; then
        while IFS= read -r line; do
            case $line in
                "signals: "*)
                    ;;
                "failed: "*)
                    names=${line#failed: }
                    old_ifs=$IFS
                    IFS=', '
                    for name in $names; do
                        [ -n "$name" ] || continue
                        reported_tests="$reported_tests$name
"
                    done
                    IFS=$old_ifs
                    ;;
            esac
        done <"$tmp_out"
        if [ -z "$reported_tests" ] || ! echo "$reported_tests" | grep -q .; then
            reported_tests="${reported_tests}signals
"
        fi
        if [ "$stop_on_first_failure" = true ]; then
            rm -f "$tmp_out"
            exit 1
        fi
    fi
    rm -f "$tmp_out"
}
