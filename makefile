CC=/opt/local/bin/c++-mp-4.9
CF=-std=c++11

# for compiling with openMP, use the first
# otherwise, use the second
CO=$(CF) -fopenmp
#CO=$(CF)

bin/output.o: src/output.cpp hdr/output.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/output.o src/output.cpp




# TESTS

test: test/bin/output_test
	./test/bin/output_test

test/bin/output_test: test/output_test.cpp src/output.cpp hdr/output.hpp
	mkdir -p test/bin
	$(CC) $(CO) -o test/bin/output_test test/output_test.cpp