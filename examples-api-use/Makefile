# Some Simple examples to get started.
FLASCHEN_TASCHEN_API_DIR=../api

CXXFLAGS=-Wall -O3 -I$(FLASCHEN_TASCHEN_API_DIR)/include -I.
LDFLAGS=-L$(FLASCHEN_TASCHEN_API_DIR)/lib -lftclient
FTLIB=$(FLASCHEN_TASCHEN_API_DIR)/lib/libftclient.a

all : simple-example simple-animation

simple-animation: simple-animation.cc
simple-example: simple-example.cc

% : %.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

%.o : %.cc
	$(CXX) $(LIB_CXXFLAGS) -c -o $@ $<

$(FTLIB) :
	make -C $(FLASCHEN_TASCHEN_API_DIR)/lib

clean:
	rm -f simple-example simple-animation
