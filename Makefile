all: canonical_histogram
	./canonical_histogram

canonical_histogram: canonical_histogram.cpp
	$(CXX) -O3 -Wall -std=c++17 -o canonical_histogram canonical_histogram.cpp
