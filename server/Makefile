# To demo the 'final' look of the FlaschenTaschen display, we have other
# output modes, that allow to already see the full resolution of the final
# display without having it available. We support all the same network API
# interface, so things can be tested independently.
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
#
FT_BACKEND=ft

# RGB matrix related. It is checked out as a submodule in rgb-matrix/
RGB_INCDIR=rgb-matrix/include
RGB_LIBDIR=rgb-matrix/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
RGB_LDFLAGS=-lrt -lm -lpthread

# The WS281x uses a library checked out as a submodule in rpi_ws281x/
WS2812LIB_OBJECTS=$(addprefix rpi_ws281x/, dma.o mailbox.o pwm.o rpihw.o ws2811.o)

INCLUDES=-I../common
OBJECTS=ft-thread.o opc-server.o udp-server.o pixel-push-server.o composite-flaschen-taschen.o priority-flaschen-taschen-sender.o cie1931.o

# Nested if/else are very awkward, so we just compare each possible outcome
ifeq ($(FT_BACKEND), ft)
   DEFINES=-DFT_BACKEND=0
   INCLUDES+=-I./rpi_ws281x
   OBJECTS+=ws2811-flaschen-taschen.o ws2801-flaschen-taschen.o lpd6803-flaschen-taschen.o ft-gpio.o multi-spi.o crate-mapping.o column-assembly.o 
   STATIC_LIBS+=libws281x.a
   LDFLAGS+=$(WS2812_LDFLAGS)
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
LDFLAGS+=-lpthread

all : ft-server

ft-server: ft-server-main.o $(OBJECTS) $(STATIC_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o : %.cc compiler-flags
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o : %.c compiler-flags
	$(CC) $(CXXFLAGS) -c -o $@ $<

# ws281x does not provide makefile, so build it out-of-band.
libws281x.a : $(WS2812LIB_OBJECTS)
	ar rcs $@ $^

$(RGB_LIBRARY):
	DEFINES=-DRGB_CLASSIC_PINOUT $(MAKE) -C $(RGB_LIBDIR)

# Make sure to re-compile when the compiler flags change.
compiler-flags: FORCE
	@echo '$(CXXFLAGS)$(OBJECTS)' | cmp -s - $@ || echo '$(CXXFLAGS)$(OBJECTS)' > $@

.PHONY: FORCE

clean:
	rm -f libws281x.a ft-server ft-server-main.o $(OBJECTS) $(WS2812LIB_OBJECTS) ft-server-main.o
