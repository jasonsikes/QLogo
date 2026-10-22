# Command-line startup script tests. Sourced by test.sh and windows_test.sh.
#
# Required variables:
#   logo_path     Absolute path to the qlogo binary
#   cmdline_dir   Directory containing the cmdline fixtures
#
# Optional variables:
#   qlogo_run_dir If set, qlogo is invoked with this as cwd (Windows DLL search)
#   exe_opts      Extra options prepended for most cases (not COMMAND.LINE checks)
#   stop_on_first_failure
#
# Mutates: test_count, reported_tests

cmdline_script_arg() {
    if [ -n "$qlogo_run_dir" ]; then
        echo "$cmdline_dir/$1"
    else
        echo "$1"
    fi
}

cmdline_invoke() {
    if [ -n "$cmdline_home" ]; then
        if [ -n "$qlogo_run_dir" ]; then
            (cd "$qlogo_run_dir" && HOME="$cmdline_home" USERPROFILE="$cmdline_home" "$logo_path" "$@")
        else
            HOME="$cmdline_home" USERPROFILE="$cmdline_home" "$logo_path" "$@"
        fi
    elif [ -n "$qlogo_run_dir" ]; then
        (cd "$qlogo_run_dir" && "$logo_path" "$@")
    else
        "$logo_path" "$@"
    fi
}

cmdline_record_failure() {
    reported_tests="$reported_tests$1
"
    if [ "$stop_on_first_failure" = true ]; then
        exit 1
    fi
}

# Compare qlogo output to an expected file. Remaining args are passed to qlogo.
# Stdin is /dev/null unless cmdline_stdin is set. cmdline_expect_status, if set,
# is compared to qlogo's exit code (nonzero means any non-zero is accepted).
cmdline_run_case() {
    name="$1"
    expected="$2"
    shift 2

    echo "cmdline: $name"
    test_count=$((test_count + 1))

    tmp_out=$(mktemp /tmp/qlogo_cmdline_XXXXXX)
    if [ -n "$cmdline_stdin" ]; then
        cmdline_invoke "$@" <"$cmdline_stdin" >"$tmp_out" 2>&1
        qlogo_status=$?
    else
        cmdline_invoke "$@" </dev/null >"$tmp_out" 2>&1
        qlogo_status=$?
    fi

    failed=false
    if ! diff "$expected" "$tmp_out" >/dev/null 2>&1; then
        echo "  output mismatch:"
        diff -u "$expected" "$tmp_out" || true
        failed=true
    fi
    if [ -n "$cmdline_expect_status" ]; then
        if [ "$cmdline_expect_status" = "nonzero" ]; then
            if [ "$qlogo_status" -eq 0 ]; then
                echo "  expected non-zero exit, got $qlogo_status"
                failed=true
            fi
        elif [ "$qlogo_status" -ne "$cmdline_expect_status" ]; then
            echo "  expected exit $cmdline_expect_status, got $qlogo_status"
            failed=true
        fi
    fi
    rm -f "$tmp_out"
    cmdline_stdin=""
    cmdline_expect_status=""
    cmdline_home=""
    if [ "$failed" = true ]; then
        cmdline_record_failure "$name"
    fi
}

run_cmdline_tests() {
    orig_dir=$(pwd)
    if [ -z "$qlogo_run_dir" ]; then
        cd "$cmdline_dir" || exit 1
    fi

    cmdline_run_case "print.lg" "$cmdline_dir/print.expected" \
        $exe_opts "$(cmdline_script_arg print.lg)"

    cmdline_run_case "define_and_call.lg" "$cmdline_dir/define_and_call.expected" \
        $exe_opts "$(cmdline_script_arg define_and_call.lg)"

    cmdline_run_case "comments_and_blanks.lg" "$cmdline_dir/comments_and_blanks.expected" \
        $exe_opts "$(cmdline_script_arg comments_and_blanks.lg)"

    cmdline_run_case "runtime_error.lg" "$cmdline_dir/runtime_error.expected" \
        $exe_opts "$(cmdline_script_arg runtime_error.lg)"

    bye_stdin=$(mktemp /tmp/qlogo_cmdline_stdin_XXXXXX)
    printf 'print [fromprompt]\n' >"$bye_stdin"
    cmdline_stdin="$bye_stdin"
    cmdline_run_case "bye.lg" "$cmdline_dir/bye.expected" \
        $exe_opts "$(cmdline_script_arg bye.lg)"
    rm -f "$bye_stdin"

    # With no hyphen, COMMAND.LINE is empty. ARGV still has the binary and script.
    cmdline_run_case "commandline.lg" "$cmdline_dir/commandline.expected" \
        "$(cmdline_script_arg commandline.lg)"

    cmdline_run_case "extra_args.lg" "$cmdline_dir/extra_args.expected" \
        "$(cmdline_script_arg extra_args.lg)" leftover

    cmdline_run_case "hyphen_args.lg" "$cmdline_dir/hyphen_args.expected" \
        "$(cmdline_script_arg hyphen_args.lg)" - leftover more

    cmdline_run_case "option_before.lg" "$cmdline_dir/print.expected" \
        --verifyIR "$(cmdline_script_arg print.lg)"

    cmdline_expect_status=nonzero
    cmdline_run_case "missing.lg" "$cmdline_dir/missing.expected" \
        this_file_does_not_exist.lg

    home_dir=$(mktemp -d /tmp/qlogo_testhome_XXXXXX)
    printf 'print [fromhome]\n' >"$home_dir/startup.lg"

    cmdline_home="$home_dir"
    cmdline_run_case "home_startup.lg" "$cmdline_dir/home_startup.expected"

    cmdline_home="$home_dir"
    cmdline_run_case "home_then_script.lg" "$cmdline_dir/home_then_script.expected" \
        $exe_opts "$(cmdline_script_arg print.lg)"

    rm -rf "$home_dir"

    cd "$orig_dir" || exit 1
}
