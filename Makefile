CXX      := clang++

CXXFLAGS := -O3 -Wall
LIBS := -lfmt

ifdef HEADER_ONLY
CXXFLAGS += -DFMT_HEADER_ONLY
LIBS =
endif

all: round

round: round.cc roundlib.hpp
	clang++ $(CXXFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f round
