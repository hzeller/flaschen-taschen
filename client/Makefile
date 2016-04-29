CXXFLAGS=-Wall -O3 -I../common -I.
LIB_OBJECTS=udp-flaschen-taschen.o bdf-font.o

LDFLAGS=-L. -lftclient

MAGICK_CXXFLAGS=`GraphicsMagick++-config --cppflags --cxxflags`
MAGICK_LDFLAGS=`GraphicsMagick++-config --ldflags --libs`

FFMPEG_LDFLAGS=`pkg-config --cflags --libs  libavcodec libavformat libswscale libavutil`

all : simple-example simple-animation send-text pong-game pong-client

simple-animation: simple-animation.cc
simple-example: simple-example.cc
send-text: send-text.cc
pong-game: pong-game.cc
pong-client: pong-client.cc

send-image : send-image.cc
	$(CXX) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -o $@ $^ $(MAGICK_LDFLAGS) $(LDFLAGS)

send-video: send-video.cc
	$(CXX) $(CXXFLAGS) -o $@ $< $(FFMPEG_LDFLAGS) $(LDFLAGS)

% : %.cc libftclient.a
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

libftclient.a: $(LIB_OBJECTS)
	ar rcs $@ $^

clean:
	rm -f send-text send-image send-video simple-example simple-animation pong-game $(LIB_OBJECTS) libftclient.a
