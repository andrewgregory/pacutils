#!/bin/bash

gdb=0
valgrind=0
cmd=()

extend() {
    if which "$1" &>/dev/null; then
        cmd+=("$@")
    else
        # bailing out would be counted as a failure by test harnesses,
        # ignore missing programs so that tests can be gracefully skipped
        printf "warning: command '$1' not found\n" >&2
    fi
}

usage() {
    printf "runtest.sh - run an executable pacman test\n"
    printf "usage: runtest.sh [options] <testfile> [test-options]\n"
    printf "\n"
    printf "Options:\n"
    printf "   -g   gdb\n"
    printf "   -h   display help\n"
    printf "   -v   valgrind\n"
}

while getopts cdghlrsv name; do
  case $name in
    g) gdb=1;;
    h) usage; exit;;
    v) valgrind=1;;
  esac
done

[ $gdb -eq 1 ] && extend gdb
[ $valgrind -eq 1 ] && extend valgrind --quiet --leak-check=full \
    --gen-suppressions=no --error-exitcode=123

shift $(($OPTIND - 1)) # remove our options from the stack

prog="$(realpath "$1")"; shift
"${cmd[@]}" "$prog" "$@"
