#!/bin/sh

set -x

buildDir="$1"
tcase="$2"

# defaults
inputFilter=cat
outputFilter=cat
output="`basename $tcase`.out"

. "$tcase"

# XXX: remove debugging
set | egrep '^(srcdir|tcase|output|testProgram|expectedOutput)='

# xxd is used by many test cases
command -v xxd > /dev/null || { echo 'Test cases require xxd' 1>&2; exit 99; }

$inputFilter | "$buildDir"/$testProgram | $outputFilter > $output \
	|| exit $?

diff -u $expectedOutput $output && rm $output
