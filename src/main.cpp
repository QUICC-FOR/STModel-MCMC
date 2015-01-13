#include <thread>
#include "../hdr/engine.hpp"

int main(int argc, char ** argv)
{
	// handle arguments
	int maxIterations = 100;	// temporary value
	
	// handle input data
	
	// create and initialize main objects
	
	// spawn engine and outputworker in threads
	std::thread engineThread (&STMEngine::Metropolis::run_sampler, 
			STMEngine::Metropolis(), maxIterations);
	
	// wait until engine completes
	engineThread.join();
	
	// signal to the outputworker to terminate
	
	// wait for outputworker to finish
	
	// any remaining cleanup
	
	return 0;
}