#!/bin/sh

# generate fresh configure and Makefile.ins
# only developers might need to run this script

autoreconf --force --install || exit $?

# leave grep output on the console to supply macro details.
if grep -E 'AX_PREFIX_CONFIG_H' configure; then
    echo "Error: Macros from Autoconf Macro Archive not expanded."
    echo "       Do you need to install the Archive?"
    echo "       Moving configire script to configure.invalid for now."
    mv -i configure configure.invalid
    exit 1
fi
