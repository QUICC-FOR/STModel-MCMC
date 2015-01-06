CC=/opt/local/bin/c++-mp-4.9
CF=-std=c++11

# for compiling with openMP, use the first
# otherwise, use the second
CO=$(CF) -fopenmp
#CO=$(CF)

bin/parameters.o: src/parameters.cpp hdr/parameters.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/parameters.o src/parameters.cpp
