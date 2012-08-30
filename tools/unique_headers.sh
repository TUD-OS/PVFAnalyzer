#!/bin/bash

DIRS="reader printer common testing unroll dynrun"

function find_includes ()
{
	find $@ | \
		xargs grep -iIsn \#include | \
		grep -v \"
}

function get_headers ()
{
	for w in $(find_includes $DIRS); do
		echo $w | sed -e 's/^[^<].*//g' | sed -e 's/[<>]//g'
	done
}

get_headers | sort | uniq | grep -v "^$"
