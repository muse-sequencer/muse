#=========================================================
#  MusE
#  Linux Music Editor
#    $Id: make.inc,v 1.1.1.1 2003/10/27 18:51:34 wschweer Exp $
#  (C) Copyright 1999-2001 Werner Schweer (ws@seh.de)
#=========================================================

#----------------------------------------------------
#     basic path
#
ifndef QTDIR_BIN
QTDIR_BIN   = /usr/qt3/bin
endif

ifndef QTDIR_LIB
QTDIR_LIB   = /usr/qt3/lib
endif

ifndef QTDIR_INC
QTDIR_INC  = /usr/qt3/include
endif

PREFIX      = /usr/muse
INSTDIR     = $(PREFIX)/share
INSTLIBDIR  = $(PREFIX)/lib
BINDIR      = /usr/bin

#-----------------------------------------------------
#     basic flags
#
# experimental KDE Flag
# compiles with KDE Support:
#
#     KMainWindow  <--> QMainWindow
#     KToolbar     <--> QToolBar
#
# Current Status: don´t use: (set to NO); there are some issues
#                 If you want to debug this:
#                 you also need Tommi Ilmonen´s "givertcap" because
#                 you cannot run MusE suid root with KDE libs
HAVE_KDE = no
KDEDIR   = /usr/kde2

#
#     AUDIO, ALSA  - always required
#     ALSACVS  - use newest ALSA CVS (include /usr/include/alsa/asoundlib.h
#                                    instead of /usr/include/sys/asooundlib.h)
AUDIO = yes
ALSA  = yes
OSS   = yes
ALSACVS = yes

# use Tommi Ilmonen´s "givertcap "to give RealTime capabilities to
# MusE; with the use of this utility MusE did not need "suid root"
# flags anymore
# more info at http://www.tml.hut.fi/~tilmonen/givertcap/

RTCAP = no

#---------------------------------------------------------------

CC         = /usr/bin/g++
QTLIB      = qt-mt
MOC        = $(QTDIR_BIN)/moc
UIC        = $(QTDIR_BIN)/uic
LUPDATE    = $(QTDIR_BIN)/lupdate
LRELEASE   = $(QTDIR_BIN)/lrelease
CPP        = cpp

#-----------------------------------------------------
OLDVERSION = 0.5.1
VERSION    = 0.5.2

INCLUDES = -I.
#FLAGS    = -Wall -W -Wshadow -D_GNU_SOURCE
FLAGS    = -Wall -W -D_GNU_SOURCE
# FLAGS    = -W -D_GNU_SOURCE
ifeq ($(HAVE_KDE),yes)
INCLUDES += -I$(KDEDIR)/include
FLAGS    += -DHAVE_KDE
endif

ifeq ($(ALSACVS),yes)
FLAGS += -DALSACVS
endif

#-----------------------------------------------------
#  DEBUG = yes generates debuggable (non optimized) version
#
DEBUG = no

ifeq ($(DEBUG),yes)
FLAGS   += -g  # -DMEMORY_DEBUG
else
FLAGS   += -O2 -g
FLAGS   += -DNDEBUG
endif

ifeq ($(AUDIO),yes)
FLAGS   += -DAUDIO
endif

ifeq ($(RTCAP),yes)
FLAGS   += -DRTCAP
endif

#-----------------------------------------------------
#  nothing to config below this line
#
ifdef TOPDIR
INCLUDES += -Imidiedit -Iscore -Iarranger -Iliste
INCLUDES += -Iwidgets -Imixer -Idriver -Iwaveedit -Implugins
else
INCLUDES += -I$(MUSETOP) -I$(MUSETOP)/widgets -I$(MUSETOP)/driver
endif

INCLUDES += -Iarchiv -I$(QTDIR_INC)
FLAGS  += -DQT_CLEAN_NAMESPACE -DQT_NOCOMPAT -DQT_NO_ASCII_CAST
# FLAGS  += -fno-exceptions
FLAGS  += -D_REENTRANT -DVERSION=\"$(VERSION)\"
FLAGS  += -DINSTDIR=\"$(INSTDIR)\"
FLAGS  += -DINSTLIBDIR=\"$(INSTLIBDIR)\"

ifeq ($(ALSA),yes)
FLAGS  += -DALSA
endif
ifeq ($(OSS),yes)
FLAGS  += -DOSS
endif

ALL_CFLAGS = $(FLAGS) $(INCLUDES) $(CFLAGS)

.PHONY: all target clean depend test translations
.SUFFIXES: .ui

%.o: %.c
	$(CC) -c $(ALL_CFLAGS) $< -o $@
obj/%.o: %.c
	$(CC) -c $(ALL_CFLAGS) $< -o $@
.ui.h:
	$(UIC) $< -o $*.h
.ui.c:
	$(UIC) -o $*.c -impl $*.h $<
obj/moc_%.c: %.h
	$(MOC) $< -o obj/moc_$*.c


all:  target

clean::
	-@rm -f *.o obj/* *.bak $(GENSRC:=.h) $(GENSRC:=.c)

depend::  $(GENSRC:=.h)
depend::
	-$(CC) -MM $(ALL_CFLAGS) $(SRC:=.c) $(MSRC:=.c) | sed -e 's/^.*\.o:/obj\/\0/' >.depend

-include .depend

ifndef TOPDIR
MAKE = make
endif

install:: ;

