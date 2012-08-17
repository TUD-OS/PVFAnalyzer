#!/bin/sh

echo "Creating new CFG processor '$1'"

if [ -z "$1" ]; then
	echo "Cannot use empty string as name.";
	exit 1;
fi

if [ -d "$1" ]; then
    echo "Subdirectory '$1' already exists.";
	exit 1;
fi

echo "Creating directory '$1'"
mkdir -p $1

echo "Creating main C++ file."
cat tools/cfgprocessor.cpp.skeleton >$1/$1.cpp

echo "Creating SConscript"
script=$1/SConscript

echo "files = \"\"\"" > $script
echo "$1" >> $script
echo "\"\"\"" >> $script
echo "" >> $script
echo "objects = [(\"%s.o\" % f) for f in files.split()]" >> $script
echo "" >> $script
echo "Import('env')" >> $script
echo "for f in files.split():" >> $script
echo "    env.Object('%s.cpp' % f)" >> $script
echo "" >> $script
echo "env.Append(LIBS=['boost_serialization'])" >> $script
echo "env.$1 = env.Program('$1', objects)" >> $script

echo "SConstruct line:"
echo "SConscript(\"$1/SConscript\", variant_dir=\"#/build/$1\", exports='env')"
