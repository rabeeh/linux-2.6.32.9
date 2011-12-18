#!/bin/bash
# Create a Makefile in a given directory for a given list of objects.
# $1 - Dir name.
# $2... - List of files.
# <dir name> [cond <compilation condition>] <file 1> [cond <compilation condition>] <file 2> ... <file n> [, <options>]

OUTMAKE=$1/Makefile

# Create a new makefile

echo "include \$(srctree)/\$(subst \",,\$(CONFIG_MV_HAL_RULES_PATH))" > $OUTMAKE

echo "# Objects list" >> $OUTMAKE

OBJ_TYPE="y"

shift
while [ "$1" ]; do
	if [ "$1" = "cond" ]; then
		shift
		OBJ_TYPE=\$\($1\)
		shift
	fi
	if [ "$1" = "opt" ]; then
		shift
		break;
	fi
	echo "obj-$OBJ_TYPE += $1" >> $OUTMAKE
        shift
done
echo "" >> $OUTMAKE

while [ "$1" ]; do
	echo "$1" >> $OUTMAKE
        shift
done


