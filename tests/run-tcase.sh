#!/bin/sh

set -x

buildDir="$1"
tcase="$2"

# defaults
# TODO: rename to $input, remove default, and require that it is set
inputFilter=cat
outputFilter=cat
finalInput="in-`basename $tcase`.tmp"
rawOutput="middle-`basename $tcase`.tmp"
finalOutput="out-`basename $tcase`.tmp"

. "$tcase"

# XXX: remove debugging
set | egrep '^(srcdir|tcase|output|testProgram|expectedOutput)='

# xxd is used by many test cases
command -v xxd > /dev/null || { echo 'Test cases require xxd' 1>&2; exit 99; }

eval "$inputFilter > $finalInput" || exit 99
eval "\"$buildDir\"/$testProgram < $finalInput > $rawOutput" || exit $?
eval "$outputFilter < $rawOutput > $finalOutput" || exit 99

diff -u $expectedOutput $finalOutput || exit $?

rm $finalInput $rawOutput $finalOutput || exit 99
