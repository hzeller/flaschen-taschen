# To demo the 'final' look of the FlaschenTaschen display, we have other
# output modes, that allow to already see the full resolution of the final
# display without having it available. We support all the same network API
# interface, so things can be tested independently.
#
# You can set the backend to use with
#
#  make FT_BACKEND=ft PROTOCOL=udp CONSTANT_FPS=0
#
# The default is 'terminal', as this is typically what you'd do. On the
# 'real' FT machine, we can set the FT_BACKEND=ft to interact with the
# SPI board and the LED strips.
#
# FT_BACKEND can be one of the following
# ft
#    The implementation that talks to the real FlaschenTaschen LED strip
#    installation.
#
# terminal
#     Very simple output to current terminal. Requires decent terminal
#     implementation that understands ANSI color escapes. Known terminals to
#     work are konsole or xterm. Newer Gnome terminals as well.
#     Requires iTerm2 > 3.0 on Mac.
#
# rgb-matrix
#    Output things to a LED matrix
#    (see https://github.com/hzeller/rpi-rgb-led-matrix )
#    This requires the project to be checked out with submodules
#    (git clone --recursive)
#
# PROTOCOL can be udp or tcp. If tcp is chosen, every new socket connection
#    creates a new thread
#
# CONSTANT_FPS can be any positive number. Its recommended to use if multiple
#    clients sending images to the server at once. If 0 the server tries to
#    show every images received. (Can lead to artifacts)

FT_BACKEND=terminal
PROTOCOL=udp
CONSTANT_FPS=0

# Spixel related. It is checked out as as submodule in spixels/
SPIXELS_INCDIR=spixels/include
SPIXELS_LIBDIR=spixels/lib
SPIXELS_LIBRARY=spixels/lib/libspixels.a

# RGB matrix related. It is checked out as a submodule in rgb-matrix/
RGB_INCDIR=rgb-matrix/include
RGB_LIBDIR=rgb-matrix/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
RGB_LDFLAGS=-lrt -lm -lpthread

INCLUDES=-I../api/include
OBJECTS=ft-thread.o composite-flaschen-taschen.o ppm-reader.o servers.o

# Nested if/else are very awkward, so we just compare each possible outcome
ifeq ($(FT_BACKEND), ft)
   DEFINES=-DFT_BACKEND=0
   OBJECTS+=crate-column-flaschen-taschen.o column-assembly.o
   INCLUDES+=-I$(SPIXELS_INCDIR)
   STATIC_LIBS+=$(SPIXELS_LIBRARY)
endif

ifeq ($(FT_BACKEND), rgb-matrix)
   DEFINES=-DFT_BACKEND=1
   INCLUDES+=-I$(RGB_INCDIR)
   OBJECTS+=rgb-matrix-flaschen-taschen.o
   STATIC_LIBS+=$(RGB_LIBRARY)
   LDFLAGS+=$(RGB_LDFLAGS)
endif

ifeq ($(FT_BACKEND), terminal)
   DEFINES=-DFT_BACKEND=2
   OBJECTS+=terminal-flaschen-taschen.o hd-terminal-flaschen-taschen.o
endif

ifeq ($(PROTOCOL), udp)
   DEFINES+=-DPROTOCOL=0
   OBJECTS+=udp-server.o
endif

ifeq ($(PROTOCOL), tcp)
   DEFINES+=-DPROTOCOL=1
   OBJECTS+=tcp-server.o
endif

CFLAGS=-Wall -O3 $(INCLUDES) $(DEFINES)
CXXFLAGS=$(CFLAGS)
LDFLAGS+=-lpthread
DEFINES+=-DCONSTANT_FPS=$(CONSTANT_FPS)

PKG_CPPFLAGS = `Magick++-config --cppflags`
PKG_CXXFLAGS = `Magick++-config --cxxflags`
PKG_LIBS = `Magick++-config --ldflags --libs`

debug: CXXFLAGS += -DDEBUG -g -O0
debug: CCFLAGS += -DDEBUG -g -O0
debug: ft-server

all : ft-server

ft-server: main.o $(OBJECTS) $(STATIC_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(PKG_CPPFLAGS) $(PKG_CXXFLAGS) $(PKG_LIBS)

%.o : %.cc .compiler-flags
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o : %.c .compiler-flags
	$(CC) $(CXXFLAGS) -c -o $@ $<

$(RGB_LIBRARY): FORCE
	$(MAKE) -C $(RGB_LIBDIR)

$(SPIXELS_LIBRARY): FORCE
	$(MAKE) -C $(SPIXELS_LIBDIR)

# Make sure to re-compile when the compiler flags change.
.compiler-flags: FORCE
	@echo '$(CXXFLAGS)$(OBJECTS)' | cmp -s - $@ || echo '$(CXXFLAGS)$(OBJECTS)' > $@

.PHONY: FORCE

clean:
	rm -f ft-server main.o $(OBJECTS)
