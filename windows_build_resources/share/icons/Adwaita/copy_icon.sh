#!/bin/bash

BASEPATH=/usr/share/icons/Adwaita

if [ $# -ne 1 ]; then
	echo "This script copies all versions (sizes) of specified icons from the host system to the project directories."
	echo
	echo "Syntax: ./copy_icon.sh category/filename.png"
	echo
	echo "  Example: ./copy_icon.sh actions/list-add.png"
	exit 1
fi

NAME=$1

if [ ! -f $BASEPATH/16x16/$NAME ]; then
	echo $BASEPATH/16x16/$NAME not found
	exit 1
fi

for size in `echo 16x16 22x22 24x24 32x32 48x48 256x256`
do
	cp $BASEPATH/$size/$NAME -v ./$size/$NAME
done
