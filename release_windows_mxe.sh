#!/bin/bash -x

MXE_ROOT=$HOME/mxe
VERSION_INC=./src/version.inc
VERSION=`cat $VERSION_INC | cut -d '=' -f 2` # VERSION=x.x.x
TMPDIR="./tmp"

if [[ $# > 0 ]]; then
	VERSION=$1
fi

function errorexit
{
	echo -e "\033[31;1m*** ERROR ***: \033[0m" $2
	exit $1
}

function build
{
	make -C ./src -f Makefile.mxe clean || errorexit 1 "pre-build cleanup failed"
	make -C ./src -f Makefile.mxe all -j2 || errorexit 1 "build error"
}

echo "Checking for dependencies..."

command -v makensis || errorexit 1 "makensis not found"
command -v $QMAKE || errorexit 1 "qmake not found"

echo $MXE_ROOT
echo $VERSION

build

rm -rf $TMPDIR
mkdir $TMPDIR
cp src/*.exe $TMPDIR
cp -r windows_build_resources/* $TMPDIR
cp LICENSE $TMPDIR/LICENSE.txt
cp README.md $TMPDIR
cp README.md $TMPDIR/readme.txt
mkdir -p 

makensis -DVERSION=$VERSION -V4 scripts/gcn64ctl.nsi
