CXXFLAGS=-Wall -O3 -I../common -I.
OBJECTS=udp-flaschen-taschen.o

MAGICK_CXXFLAGS=`GraphicsMagick++-config --cppflags --cxxflags`
MAGICK_LDFLAGS=`GraphicsMagick++-config --ldflags --libs`

all : send-image simple-example simple-animation

simple-animation: simple-animation.cc

simple-example: simple-example.cc

send-image : send-image.cc $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -o $@ $^ $(MAGICK_LDFLAGS)

% : %.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(OBJECTS)

clean:
	rm -f send-image simple-example simple-animation
