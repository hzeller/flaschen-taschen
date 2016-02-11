
RGB_INCDIR=rgb-matrix/include
RGB_LIBDIR=rgb-matrix/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread

INCLUDES=-I./rpi_ws281x -I../common -I$(RGB_INCDIR)
WS2812LIB_OBJECTS=$(addprefix rpi_ws281x/, dma.o mailbox.o pwm.o rpihw.o ws2811.o)
OBJECTS=ws28xx-flaschen-taschen.o lpd6803-flaschen-taschen.o ft-gpio.o ft-thread.o rgb-flaschen-taschen.o
SERVER_OBJECTS=opc-server.o udp-server.o pixel-push-server.o

CFLAGS=-Wall -O3 $(INCLUDES)
CXXFLAGS=$(CFLAGS)
LDFLAGS+=-lpthread

all : ft-server

ft-server: ft-server-main.o $(SERVER_OBJECTS) $(OBJECTS) libws281x.a $(RGB_LIBRARY)
	$(CXX) -o $@ $^ $(LDFLAGS)

# ws281x does not provide makefile, so build it out-of-band.
libws281x.a : $(WS2812LIB_OBJECTS)
	ar rcs $@ $^

$(RGB_LIBRARY):
	$(MAKE) -C $(RGB_LIBDIR)

clean:
	rm -f libws281x.a ft-server ft-server-main.o$(OBJECTS) $(SERVER_OBJECTS) $(WS2812LIB_OBJECTS)
