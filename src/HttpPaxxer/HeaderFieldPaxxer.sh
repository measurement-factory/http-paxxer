#!/bin/sh

# A temporary script to test HTTP header parsing and packing.
# TODO: replace with proper .tcases.

echo "82 83 84 85 86 42 02 76 31 44 02 76 32" | \
	xxd -r -p | \
	./HeaderFieldPaxxer --parse > /tmp/StatTab.txt
echo -n ":method: GET
:method: POST
:path: /
:path: /index.html
:scheme: http
:method: v1
:path: v2
" > /tmp/StatTabOrig.txt
if cmp -s /tmp/StatTabOrig.txt /tmp/StatTab.txt; then
	echo "OK: parsed static names and values"
else
	echo "Error: cannot parse static names and values"
	exit 1
fi

echo "40 02 6e 31 02 76 31 7e 02 76 32 40 02 6e 32 02 76 33 c0" | \
	xxd -r -p | \
	./HeaderFieldPaxxer --parse > /tmp/DynamTab.txt
echo -n "n1: v1
n1: v2
n2: v3
n1: v1
" > /tmp/DynamTabOrig.txt
if cmp -s /tmp/DynamTabOrig.txt /tmp/DynamTab.txt; then
	echo "OK: parsed dynamic names and values"
else
	echo "Error: cannot parse dynamic names and values"
	exit 1
fi

echo -n ":method: GET
:method: POST
:path: /
:path: /index.html
:scheme: http
:method: v1
:path: v2
" | ./HeaderFieldPaxxer --pack | xxd -p > /tmp/StatTab.hex
echo "82838485864302763145027632" > /tmp/StatTabOrig.hex
if cmp -s /tmp/StatTabOrig.hex /tmp/StatTab.hex; then
	echo "OK: packed static names and values"
else
	echo "Error: cannot pack static names and values"
	exit 1
fi

echo -n "n1: v1
n1: v2
n2: v3
n1: v1
" | ./HeaderFieldPaxxer --pack | xxd -p > /tmp/DynamTab.hex
echo "40026e310276317e02763240026e32027633c0" > /tmp/DynamTabOrig.hex
if cmp -s /tmp/DynamTabOrig.hex /tmp/DynamTab.hex; then
	echo "OK: packed dynamic names and values"
else
	echo "Error: cannot pack dynamic names and values"
	exit 1
fi
