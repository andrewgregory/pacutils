#!/bin/sh
ret=0
for t in "$@"; do "$(realpath "$t")" || ret=$?; done
exit $ret
