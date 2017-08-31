CXXFLAGS=-Wall -Wextra -O3 -I../include -I. -std=c++03
LIB_OBJECTS=udp-flaschen-taschen.o bdf-font.o graphics.o
LIB_CXXFLAGS=$(CXXFLAGS) -fPIC

TARGET=libftclient

all : $(TARGET).a $(TARGET).so.1

$(TARGET).a: $(LIB_OBJECTS)
	ar rcs $@ $^

$(TARGET).so.1 : $(LIB_OBJECTS)
	g++ -shared -Wl,-soname,$@ -o $@ $^

%.o : %.cc
	$(CXX) $(LIB_CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(LIB_OBJECTS) $(TARGET).a $(TARGET).so.1
