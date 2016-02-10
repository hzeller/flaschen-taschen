CXXFLAGS=-Wall -O3 -I../common -I.
OBJECTS=udp-flaschen-taschen.o

MAGICK_CXXFLAGS=`GraphicsMagick++-config --cppflags --cxxflags`
MAGICK_LDFLAGS=`GraphicsMagick++-config --ldflags --libs`

all : send-image simple-example

send-image : send-image.cc $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -o $@ $^ $(MAGICK_LDFLAGS)

simple-example : simple-example.cc $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f send-image
