CC=/opt/local/bin/c++-mp-4.9
CF=-std=c++11

# for compiling with openMP, use the first
# otherwise, use the second
CO=$(CF) -fopenmp
#CO=$(CF)

GSL=-lgsl

bin/main.o: src/main.cpp hdr/engine.hpp hdr/output.hpp hdr/parameters.hpp \
hdr/likelihood.hpp hdr/input.hpp
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

bin/input.o: src/input.cpp hdr/input.hpp hdr/parameters.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/input.o src/input.cpp



# TESTS

test: test/bin/engine_test
	./test/bin/engine_test
	
test/bin/engine_test: test/bin/engine_test.o bin/engine.o bin/input.o bin/likelihood.o \
bin/parameters.o bin/output.o
	$(CC) $(CO) $(GSL) -o test/bin/engine_test test/bin/engine_test.o bin/engine.o \
	bin/likelihood.o bin/input.o bin/parameters.o bin/output.o
	
test/bin/engine_test.o: test/engine_test.cpp hdr/engine.hpp hdr/likelihood.hpp \
hdr/input.hpp hdr/parameters.hpp hdr/output.hpp
	$(CC) $(CO) -c -o test/bin/engine_test.o test/engine_test.cpp

test/bin/like_test: test/bin/like_test.o bin/input.o bin/likelihood.o bin/parameters.o
	$(CC) $(CO) -o test/bin/like_test test/bin/like_test.o bin/likelihood.o bin/input.o \
	bin/parameters.o $(GSL)
	
test/bin/like_test.o: test/like_test.cpp hdr/likelihood.hpp hdr/input.hpp \
hdr/parameters.hpp 
	$(CC) $(CO) -c -o test/bin/like_test.o test/like_test.cpp

test/bin/param_test: test/bin/param_test.o bin/input.o bin/parameters.o
	$(CC) $(CO) -o test/bin/param_test test/bin/param_test.o bin/parameters.o bin/input.o
	
test/bin/param_test.o: test/param_test.cpp hdr/parameters.hpp hdr/input.hpp
	$(CC) $(CO) -c -o test/bin/param_test.o test/param_test.cpp

test/bin/input_test: test/bin/input_test.o bin/parameters.o bin/likelihood.o bin/input.o
	$(CC) $(CO) -o test/bin/input_test test/bin/input_test.o bin/parameters.o \
	bin/likelihood.o bin/input.o $(GSL)

test/bin/input_test.o: test/input_test.cpp hdr/parameters.hpp hdr/likelihood.hpp \
hdr/input.hpp
	mkdir -p test/bin
	$(CC) $(CO) -c -o test/bin/input_test.o test/input_test.cpp	

test/bin/output_test: test/output_test.cpp src/output.cpp hdr/output.hpp
	mkdir -p test/bin
	$(CC) $(CO) -o test/bin/output_test test/output_test.cpp


# tests not run by default
done_tests: test/bin/input_test test/bin/param_test test/bin/like_test
	./test/bin/input_test
	./test/bin/param_test
	./test/bin/like_test