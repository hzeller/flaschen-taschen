# To demo the 'final' look of the FlaschenTaschen display, we have other
# output modes, that allow to already see the full resolution of the final
# display without having it available. We support all the same network API
# interface, so things can be tested independently.
#
# TL;DR: If you want the output to the terminal for local experiments, try
#
# make FT_TERMINAL=terminal
#
# FT_BACKEND can be one of the following
# ft
#    The implementation that talks to the real FlaschenTaschen LED strip
#    installation.
#
# terminal
#     Very simple output to current terminal. Requires decent terminal
#     implementation that understands ANSI color escapes. Known terminals to
#     work are konsole or xterm works. Gnome terminal does not.
#
# rgb-matrix
#    Output things to a LED matrix
#    (see https://github.com/hzeller/rpi-rgb-led-matrix )
#    This requires the project to be checked out with submodules
#    (git clone --recursive)
#
FT_BACKEND=ft

# RGB matrix related. It is checked out as a submodule in rgb-matrix/
RGB_INCDIR=rgb-matrix/include
RGB_LIBDIR=rgb-matrix/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
RGB_LDFLAGS=-lrt -lm -lpthread

INCLUDES=-I../common
OBJECTS=ft-thread.o opc-server.o udp-server.o pixel-push-server.o composite-flaschen-taschen.o cie1931.o ppm-reader.o

# Nested if/else are very awkward, so we just compare each possible outcome
ifeq ($(FT_BACKEND), ft)
   DEFINES=-DFT_BACKEND=0
   OBJECTS+=ws2801-flaschen-taschen.o lpd6803-flaschen-taschen.o ft-gpio.o multi-spi.o mailbox.o rpi-dma.o crate-mapping.o column-assembly.o
endif

ifeq ($(FT_BACKEND), rgb-matrix)
   DEFINES=-DFT_BACKEND=1
   INCLUDES+=-I$(RGB_INCDIR)
   OBJECTS+=rgb-flaschen-taschen.o
   STATIC_LIBS+=$(RGB_LIBRARY)
   LDFLAGS+=$(RGB_LDFLAGS)
endif

ifeq ($(FT_BACKEND), terminal)
   DEFINES=-DFT_BACKEND=2
   OBJECTS+=terminal-flaschen-taschen.o
endif

CFLAGS=-Wall -O3 $(INCLUDES) $(DEFINES)
CXXFLAGS=$(CFLAGS)
LDFLAGS+=-lpthread -lrt

all : ft-server

ft-server: main.o $(OBJECTS) $(STATIC_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o : %.cc compiler-flags
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o : %.c compiler-flags
	$(CC) $(CXXFLAGS) -c -o $@ $<

$(RGB_LIBRARY):
	$(MAKE) -C $(RGB_LIBDIR)

# Make sure to re-compile when the compiler flags change.
compiler-flags: FORCE
	@echo '$(CXXFLAGS)$(OBJECTS)' | cmp -s - $@ || echo '$(CXXFLAGS)$(OBJECTS)' > $@

.PHONY: FORCE

clean:
	rm -f ft-server main.o $(OBJECTS)
