
INCLUDES=-I./rpi_ws281x
WS2812LIB_OBJECTS=$(addprefix rpi_ws281x/, dma.o mailbox.o pwm.o rpihw.o ws2811.o)
OBJECTS=ws28xx-flaschen-taschen.o lpd6803-flaschen-taschen.o ft-gpio.o thread.o
SERVER_OBJECTS=opc-server.o udp-server.o pixel-push-server.o

CFLAGS=-Wall -O3 $(INCLUDES)
CXXFLAGS=$(CFLAGS)
LIBS=-lpthread

all : ft-server

ft-server: ft-server-main.o $(SERVER_OBJECTS) $(OBJECTS) libws281x.a
	$(CXX) -o $@ $^ $(LIBS)

# ws281x does not provide makefile, so build it out-of-band.
libws281x.a : $(WS2812LIB_OBJECTS)
	ar rcs $@ $^

clean:
	rm -f libws281x.a ft-server ft-server-main.o$(OBJECTS) $(SERVER_OBJECTS) $(WS2812LIB_OBJECTS)
