FLASCHEN_TASCHEN_API_DIR=../api

CXXFLAGS=-Wall -Wextra -pedantic -O3 -I$(FLASCHEN_TASCHEN_API_DIR)/include -I. -std=c++03
LDFLAGS=-L$(FLASCHEN_TASCHEN_API_DIR)/lib -lftclient
FTLIB=$(FLASCHEN_TASCHEN_API_DIR)/lib/libftclient.a

MAGICK_CXXFLAGS=$(shell GraphicsMagick++-config --cppflags --cxxflags)
MAGICK_LDFLAGS=$(shell GraphicsMagick++-config --ldflags --libs)

FFMPEG_LDFLAGS=$(shell pkg-config --cflags --libs  libavcodec libavformat libswscale libavutil libavdevice)
all : send-text

send-text: send-text.cc

send-image : send-image.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -o $@ $< $(MAGICK_LDFLAGS) $(LDFLAGS)

# We're using a couple of deprecated functions. Pull request to update this to
# the latest libraries is welcome.
send-video: send-video.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) -Wno-deprecated-declarations -o $@ $< $(FFMPEG_LDFLAGS) $(LDFLAGS)

% : %.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

%.o : %.cc
	$(CXX) $(LIB_CXXFLAGS) -c -o $@ $<

$(FTLIB):
	make -C $(FLASCHEN_TASCHEN_API_DIR)/lib

clean:
	rm -f send-text send-image send-video
