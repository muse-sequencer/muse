SHELL = /bin/sh
CXX    = g++
CFLAGS       = -fPIC -shared -pedantic -Wall -Wextra -Werror -std=c++11
LDFLAGS      = -shared
DEBUGFLAGS   = -O0 -g -D_DEBUG
RELEASEFLAGS = -Os -g0 -DNDEBUG

TARGETX64  = lv2Gtk2Helper64.so
TARGETX32  = lv2Gtk2Helper32.so
FLAGS64 = -m64
FLAGS32 = -m32
SOURCES = $(shell echo *.cpp)
HEADERS = $(shell echo *.h)
OBJECTS = $(SOURCES:.c=.o)
GTKMM2LIBS = $(shell pkg-config gtkmm-2.4 --cflags --libs)
GTK2LIBS = $(shell pkg-config gtk+-2.0 --cflags --libs)

all: $(TARGETX64) $(TARGETX32)

$(TARGETX64): $(OBJECTS)
		$(CXX) $(FLAGS64) $(CFLAGS) $(RELEASEFLAGS) -o $(TARGETX64) $(OBJECTS) $(GTKMM2LIBS) $(GTK2LIBS)
$(TARGETX32): $(OBJECTS)
		$(CXX) $(FLAGS32) $(CFLAGS) $(RELEASEFLAGS) -o $(TARGETX32) $(OBJECTS) $(GTKMM2LIBS) $(GTK2LIBS)
