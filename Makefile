have = $(shell command -v $(1) 2>/dev/null || true)

ifeq ($(origin CXX), command line)
  ifeq (, $(call have,$(CXX)))
    $(error CXX '$(CXX)' not found)
  endif
else
  CXX := clang++
  ifeq (, $(call have,$(CXX)))
    CXX := g++
    ifeq (, $(call have,g++))
      $(error no C++ compiler found: install, e.g., clang++ or g++ or define an alternative via `make CXX=...`)
    endif
  endif
endif

CXXFLAGS := -O3 -Wall
LIBS := -lfmt

ifdef HEADER_ONLY
CXXFLAGS += -DFMT_HEADER_ONLY
LIBS =
endif

all: round

round: round.cc roundlib.hpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f round
