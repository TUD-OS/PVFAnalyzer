#!/bin/bash

find . | grep -v build | grep -v .git | \
		 xargs grep -iIsn \#include | \
		 grep -v \" | \
		 cut -d: -f3 | \
		 cut -d\  -f 2 | \
		 cut -d\< -f2 | \
		 cut -d\> -f1 | \
		 sort | uniq 
