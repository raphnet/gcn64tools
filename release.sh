#!/bin/bash

VERSION_INC=./src/version.inc
VERSION=`cat $VERSION_INC | cut -d '=' -f 2` # VERSION=x.x.x
PREFIX=raphnet-tech_adapter_manager
HEXFILE=$PREFIX.hex
TAG=v$VERSION

echo "Release script for $PREFIX"

if [ $# -ne 1 ]; then
	echo "Syntax: ./release.sh version releasedir"
	echo
	echo "ex: './release.sh ../releases' will produce $PREFIX-$VERSION.tar.gz in releasedir out of git HEAD,"
	echo "and place it in the ../releases directory."
	echo
	echo "It will also create a tag named $TAG"
	exit
fi

RELEASEDIR=$1
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

git tag $TAG -f
git archive --format=tar --prefix=$DIRNAME/ HEAD | gzip > $RELEASEDIR/$FILENAME
