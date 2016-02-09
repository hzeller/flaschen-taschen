CXXFLAGS=-Wall -O3 

MAGICK_CXXFLAGS=`GraphicsMagick++-config --cppflags --cxxflags`
MAGICK_LDFLAGS=`GraphicsMagick++-config --ldflags --libs`

all : send-image

send-image : send-image.cc
	$(CXX) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -o $@ $^ $(MAGICK_LDFLAGS)

clean:
	rm -f send-image
