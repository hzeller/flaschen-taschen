FLASCHEN_TASCHEN_API_DIR=../client

CXXFLAGS=-Wall -O3 -I$(FLASCHEN_TASCHEN_API_DIR)/include -I.
LDFLAGS=-L$(FLASCHEN_TASCHEN_API_DIR)/lib -lftclient
FTLIB=$(FLASCHEN_TASCHEN_API_DIR)/lib/libftclient.a

MAGICK_CXXFLAGS=`GraphicsMagick++-config --cppflags --cxxflags`
MAGICK_LDFLAGS=`GraphicsMagick++-config --ldflags --libs`

FFMPEG_LDFLAGS=`pkg-config --cflags --libs  libavcodec libavformat libswscale libavutil`

all : send-text

send-text: send-text.cc

send-image : send-image.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -o $@ $< $(MAGICK_LDFLAGS) $(LDFLAGS)

send-video: send-video.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) -o $@ $< $(FFMPEG_LDFLAGS) $(LDFLAGS)

% : %.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

%.o : %.cc
	$(CXX) $(LIB_CXXFLAGS) -c -o $@ $<

$(FTLIB):
	make -C $(FLASCHEN_TASCHEN_API_DIR)/lib

clean:
	rm -f send-text send-image send-video
