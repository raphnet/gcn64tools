CC=gcc
LD=$(CC)
VERSION=1.6.0

VERSION_STR=\"$(VERSION)\"

UNAME := $(shell uname -s)
ifeq ($(UNAME), Linux)
HIDAPI_NAME=hidapi-hidraw
else
HIDAPI_NAME=hidapi
endif


ifeq ($(shell uname -o), Msys)
COMPAT_OBJS=sleep.o memmem.o strcasestr.o app.o
PLATFORM_CFLAGS=-DWINDOWS
EXTRA_LDFLAGS=-mwindows # uncomment for console output
MAKENSIS=/c/Program\ Files\ \(x86\)/NSIS/makensis
endif

CFLAGS=-Wall -g `pkg-config $(HIDAPI_NAME) zlib --cflags` --std=c99 $(PLATFORM_CFLAGS) -DVERSION_STR=$(VERSION_STR)
LDFLAGS=`pkg-config $(HIDAPI_NAME) zlib --libs` -g -lz
GTK_CFLAGS=`pkg-config --cflags gtk+-3.0 gmodule-2.0 gio-2.0`
GTK_LDFLAGS=`pkg-config --libs gtk+-3.0 gmodule-2.0 gio-2.0`
GLIB_CRES=$(shell pkg-config --variable=glib_compile_resources gio-2.0)
RESOURCES=$(shell $(GLIB_CRES) --sourcedir=. --generate-dependencies resources.xml)

PREFIX=/usr/local

PROGS=gcn64ctl mempak_ls mempak_format mempak_extract_note mempak_insert_note mempak_rm mempak_convert gcn64ctl_gui
MEMPAKLIB_OBJS=mempak.o mempak_fs.o $(COMPAT_OBJS)
GUI_OBJS=gcn64ctl_gui.o gui_mpkedit.o gui_fwupd.o gui_logger.o gui_dfu_programmer.o gui_gc2n64_manager.o gui_update_progress_dialog.o gui_xferpak.o resources.o


.PHONY : clean install release_windows

all: $(PROGS) gui.xml


gcn64ctl_gui: $(GUI_OBJS) gcn64.o gcn64lib.o gc2n64_adapter.o delay.o hexdump.o ihex.o ihex_signature.o mempak_gcn64usb.o xferpak.o xferpak_tools.o gbcart.o uiio.o uiio_gtk.o $(MEMPAKLIB_OBJS)
	$(LD) $^ $(LDFLAGS) $(GTK_LDFLAGS) -o $@ $(EXTRA_LDFLAGS)

gcn64ctl: main.o gcn64.o gcn64lib.o hexdump.o gc2n64_adapter.o ihex.o delay.o mempak_gcn64usb.o perftest.o biosensor.o xferpak.o xferpak_tools.o gbcart.o uiio.o $(MEMPAKLIB_OBJS)
	$(LD) $^ $(LDFLAGS) -o $@

app.o: app.rc icon.ico
	windres app.rc -o app.o

gui.xml: gcn64cfg.glade
	grep -v requires gcn64cfg.glade > gui.xml

resources.c: $(RESOURCES)
	$(GLIB_CRES) $(RES_FILE) --target=$@ --sourcedir=. --generate-source resources.xml

resources.o: resources.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $<

gcn64ctl_gui.o: gcn64ctl_gui.c gcn64ctl_gui.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $<

gui_%.o: gui_%.c gui_%.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $<

uiio_gtk.o: uiio_gtk.c uiio_gtk.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $<

mempak_convert: mempak_convert.o $(MEMPAKLIB_OBJS)
	$(LD) $^ $(LDFLAGS) -o $@

mempak_rm: mempak_rm.o $(MEMPAKLIB_OBJS)
	$(LD) $^ $(LDFLAGS) -o $@

mempak_insert_note: mempak_insert_note.o $(MEMPAKLIB_OBJS)
	$(LD) $^ $(LDFLAGS) -o $@

mempak_extract_note: mempak_extract_note.o $(MEMPAKLIB_OBJS)
	$(LD) $^ $(LDFLAGS) -o $@

mempak_ls: mempak_ls.o $(MEMPAKLIB_OBJS)
	$(LD) $^ $(LDFLAGS) -o $@

mempak_format: mempak_format.o $(MEMPAKLIB_OBJS)
	$(LD) $^ $(LDFLAGS) -o $@


%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.exe $(PROGS)

install:
	@echo "Install not done yet. Sorry"

release_windows:
	cp $(addsuffix .exe,$(PROGS)) release_windows
	cp gui.xml release_windows
	cp project_image.png release_windows
	$(MAKENSIS) //DVERSION=$(VERSION) gcn64ctl.nsi
	ls installers
	@echo "Done"
