CC=/opt/local/bin/c++-mp-4.9
CF=-std=c++11

# for compiling with openMP, use the first
# otherwise, use the second
CO=$(CF) -fopenmp
#CO=$(CF)

bin/main.o: src/main.cpp hdr/engine.hpp hdr/output.hpp hdr/parameters.hpp \
hdr/likelihood.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/main.o src/main.cpp

bin/engine.o: src/engine.cpp hdr/engine.hpp hdr/parameters.hpp hdr/likelihood.hpp \
hdr/output.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/engine.o src/engine.cpp

bin/parameters.o: src/parameters.cpp hdr/parameters.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/parameters.o src/parameters.cpp

bin/likelihood.o: src/likelihood.cpp hdr/likelihood.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/likelihood.o src/likelihood.cpp

bin/output.o: src/output.cpp hdr/output.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/output.o src/output.cpp




# TESTS

test: test/bin/output_test
	./test/bin/output_test

test/bin/output_test: test/output_test.cpp src/output.cpp hdr/output.hpp
	mkdir -p test/bin
	$(CC) $(CO) -o test/bin/output_test test/output_test.cpp
