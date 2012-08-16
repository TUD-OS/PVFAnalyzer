#!/bin/bash

DIRS="cfg_reader cfg_printer common testing"

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
