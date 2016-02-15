#!/bin/bash

PREFIX=gcn64tools
HEXFILE=$PREFIX.hex

echo "Release script for $PREFIX"

if [ $# -ne 2 ]; then
	echo "Syntax: ./release.sh version releasedir"
	echo
	echo "ex: './release 1.0' will produce $PREFIX-1.0.tar.gz in releasedir out of git HEAD,"
	echo "and place it in the release directory."
	exit
fi

VERSION=$1
RELEASEDIR=$2
DIRNAME=$PREFIX-$VERSION
FILENAME=$PREFIX-$VERSION.tar.gz

echo "Version: $VERSION"
echo "Filename: $FILENAME"
echo "Release directory: $RELEASEDIR"
echo "--------"
echo "Ready? Press ENTER to go ahead (or CTRL+C to cancel)"

read

if [ -f $RELEASEDIR/$FILENAME ]; then
	echo "Release file already exists!"
	exit 1
fi

git archive --format=tar --prefix=$DIRNAME/ HEAD | gzip > $RELEASEDIR/$FILENAME

#cd $RELEASEDIR
#tar zxf $FILENAME
#cd $DIRNAME
#make
#cp $HEXFILE ../$PREFIX-$VERSION.hex
#cd ..
#echo
#echo
#echo
#ls -l $PREFIX-$VERSION.*
