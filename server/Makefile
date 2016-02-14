# To demo the 'final' look of the FlaschenTaschen display, we have a mode
# in which we support the same protocol but output things to a LED matrix
# (see https://github.com/hzeller/rpi-rgb-led-matrix )
# This switches between the two options.
RGB_MATRIX_DEMO=0

# RGB matrix related. It is checked out as a submodule in rgb-matrix/
RGB_INCDIR=rgb-matrix/include
RGB_LIBDIR=rgb-matrix/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
RGB_LDFLAGS=-lrt -lm -lpthread

# The WS281x uses a library checked out as a submodule in rpi_ws281x/
WS2812LIB_OBJECTS=$(addprefix rpi_ws281x/, dma.o mailbox.o pwm.o rpihw.o ws2811.o)

INCLUDES=-I../common
OBJECTS=ft-thread.o opc-server.o udp-server.o pixel-push-server.o

ifeq ($(RGB_MATRIX_DEMO), 1)
   DEFINES=-DDEMO_RGB=1
   INCLUDES+=-I$(RGB_INCDIR)
   OBJECTS+=rgb-flaschen-taschen.o
   STATIC_LIBS+=$(RGB_LIBRARY)
   LDFLAGS+=$(RGB_LDFLAGS)
else
   DEFINES=-DDEMO_RGB=0
   INCLUDES+=-I./rpi_ws281x
   OBJECTS+=ws2811-flaschen-taschen.o ws2801-flaschen-taschen.o lpd6803-flaschen-taschen.o ft-gpio.o multi-spi.o crate-mapping.o column-assembly.o 
   STATIC_LIBS+=libws281x.a
   LDFLAGS+=$(WS2812_LDFLAGS)
endif

CFLAGS=-Wall -O3 $(INCLUDES) $(DEFINES)
CXXFLAGS=$(CFLAGS)
LDFLAGS+=-lpthread

all : ft-server

ft-server: ft-server-main.o $(OBJECTS) $(STATIC_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# ws281x does not provide makefile, so build it out-of-band.
libws281x.a : $(WS2812LIB_OBJECTS)
	ar rcs $@ $^

$(RGB_LIBRARY):
	DEFINES=-DRGB_CLASSIC_PINOUT $(MAKE) -C $(RGB_LIBDIR)

clean:
	rm -f libws281x.a ft-server ft-server-main.o $(OBJECTS) $(WS2812LIB_OBJECTS) ft-server-main.o
