CC=gcc
LD=$(CC)
PREFIX?=/usr/local

EXEEXT=
PLATFORM_CFLAGS=


### HIDAPI
UNAME := $(shell uname -s)
ifeq ($(UNAME), Linux)
HIDAPI_NAME=hidapi-hidraw
else ifeq ($(UNAME), FreeBSD)
HIDAPI_NAME=hidapi
PLATFORM_CFLAGS=-DFREEBSD
CC=clang
else
HIDAPI_NAME=hidapi
endif
HIDAPI_CFLAGS=`pkg-config $(HIDAPI_NAME) --cflags`
HIDAPI_LDFLAGS=`pkg-config $(HIDAPI_NAME) --libs`


### ZLIB
ZLIB_CFLAGS=`pkg-config zlib --cflags`
ZLIB_LDFLAGS=`pkg-config zlib --libs`


### GTK
GTK_CFLAGS=`pkg-config --cflags gtk+-3.0 gmodule-2.0 gio-2.0`
GTK_LDFLAGS=`pkg-config --libs gtk+-3.0 gmodule-2.0 gio-2.0`
GLIB_CRES=$(shell pkg-config --variable=glib_compile_resources gio-2.0)
RESOURCES=$(shell $(GLIB_CRES) --sourcedir=. --generate-dependencies resources.xml)

include Makefile.common

install:
	cp gcn64ctl gcn64ctl_gui mempak_convert mempak_extract_note mempak_insert_note mempak_ls mempak_rm $(PREFIX)/bin


