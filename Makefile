all: canonical_histogram
	./canonical_histogram

canonical_histogram: canonical_histogram.cpp histogram.h
	$(CXX) -O3 -Wall -std=c++17 -pthread -o canonical_histogram canonical_histogram.cpp
