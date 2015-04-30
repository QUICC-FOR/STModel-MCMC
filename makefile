CC=/opt/local/bin/c++-mp-4.9
CF=-std=c++11

# for compiling with openMP, use the first
# otherwise, use the second
CO=$(CF) -fopenmp
#CO=$(CF)

GSL=-lgsl

twostate: bin/stm2_mcmc
fourstate: bin/stm4_mcmc

# executables
# two state
bin/stm2_mcmc: bin/main.o bin/engine.o bin/parameters.o bin/likelihood.o bin/output.o \
bin/input.o bin/model_2.o
	$(CC) $(CO) $(GSL) -o bin/stm2_mcmc bin/main.o bin/engine.o bin/parameters.o \
	bin/likelihood.o bin/output.o bin/input.o bin/model_2.o

# four state
bin/stm4_mcmc: bin/main.o bin/engine.o bin/parameters.o bin/likelihood.o bin/output.o \
bin/input.o bin/model_4.o
	$(CC) $(CO) $(GSL) -o bin/stm4_mcmc bin/main.o bin/engine.o bin/parameters.o \
	bin/likelihood.o bin/output.o bin/input.o bin/model_4.o


# object files
bin/main.o: src/main.cpp hdr/engine.hpp hdr/output.hpp hdr/parameters.hpp \
hdr/likelihood.hpp hdr/input.hpp hdr/model.hpp hdr/stmtypes.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/main.o src/main.cpp
	
bin/input.o: src/input.cpp hdr/input.hpp hdr/parameters.hpp hdr/likelihood.hpp \
hdr/model.hpp hdr/stmtypes.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/input.o src/input.cpp

bin/engine.o: src/engine.cpp hdr/engine.hpp hdr/parameters.hpp hdr/likelihood.hpp \
hdr/output.hpp hdr/stmtypes.hpp hdr/input.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/engine.o src/engine.cpp

bin/likelihood.o: src/likelihood.cpp hdr/likelihood.hpp hdr/model.hpp hdr/stmtypes.hpp \
hdr/parameters.hpp hdr/input.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/likelihood.o src/likelihood.cpp

bin/parameters.o: src/parameters.cpp hdr/parameters.hpp hdr/input.hpp hdr/stmtypes.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/parameters.o src/parameters.cpp

bin/output.o: src/output.cpp hdr/output.hpp hdr/stmtypes.hpp hdr/input.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/output.o src/output.cpp

# 4-state model
bin/model_4.o: src/four_state/model_4s.cpp hdr/model.hpp hdr/stmtypes.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/model_4.o src/four_state/model_4s.cpp

# 2-state model
bin/model_2.o: src/two_state/model_2s.cpp hdr/model.hpp hdr/stmtypes.hpp
	mkdir -p bin
	$(CC) $(CO) -c -o bin/model_2.o src/two_state/model_2s.cpp


# TESTS

test: test/bin/main_test
	./test/bin/main_test
	
test/bin/main_test: test/bin/main_test.o bin/engine.o bin/input.o bin/likelihood.o \
bin/parameters.o bin/output.o
	$(CC) $(CO) $(GSL) -o test/bin/main_test test/bin/main_test.o bin/engine.o \
	bin/likelihood.o bin/input.o bin/parameters.o bin/output.o
	
test/bin/main_test.o: test/main_test.cpp hdr/engine.hpp hdr/likelihood.hpp \
hdr/input.hpp hdr/parameters.hpp hdr/output.hpp
	$(CC) $(CO) -c -o test/bin/main_test.o test/main_test.cpp

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
done_tests: test/bin/input_test test/bin/param_test test/bin/like_test test/bin/engine_test
	./test/bin/input_test
	./test/bin/param_test
	./test/bin/like_test
	./test/bin/engine_test




####### DATA
data: inp/GenSA_initForFit_rf_0.05.txt inp/initForFit_rf_0.05.rdata prep_data.r
	Rscript prep_data.r