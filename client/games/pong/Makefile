FLASCHEN_TASCHEN_API_DIR=../../api

CXXFLAGS=-Wall -O3 -I$(FLASCHEN_TASCHEN_API_DIR)/include -I.
LDFLAGS=-L$(FLASCHEN_TASCHEN_API_DIR)/lib -lftclient
FTLIB=$(FLASCHEN_TASCHEN_API_DIR)/lib/libftclient.a

pong: game-client pong-game

game-client: game-client.cc

pong-game: pong-game.cc game-engine.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

send-image : send-image.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -o $@ $< $(MAGICK_LDFLAGS) $(LDFLAGS)

send-video: send-video.cc libftclient.a
	$(CXX) $(CXXFLAGS) -o $@ $< $(FFMPEG_LDFLAGS) $(LDFLAGS)

% : %.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

%.o : %.cc
	$(CXX) $(LIB_CXXFLAGS) -c -o $@ $<

$(FTLIB) :
	make -C $(FLASCHEN_TASCHEN_API_DIR)/lib

clean:
	rm -f pong-game game-client
