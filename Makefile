CXX      := clang++

CXXFLAGS := -O3 -Wall

round: round.cc roundlib.hpp
	clang++ $(CXXFLAGS) -o $@ $< -lfmt
