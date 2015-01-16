#include <thread>
#include <vector>
#include "../hdr/engine.hpp"
#include "../hdr/output.hpp"
#include "../hdr/parameters.hpp"
#include "../hdr/likelihood.hpp"

int main(int argc, char ** argv)
{
	// handle arguments
	int maxIterations = 100;	// temporary value
	
	// handle input data -- will need to write the code to get these from disk
	std::vector<STMParameters::ParameterSettings> inits;
	std::vector<STMLikelihood::Transition> transitionData;
	std::map<std::string, STMLikelihood::PriorDist> priors;
	
	// create and initialize main objects
	STMOutput::OutputQueue * outQueue = new STMOutput::OutputQueue;
	STMLikelihood::Likelihood * likelihood = new STMLikelihood::Likelihood 
			(transitionData, priors);

	// spawn engine and outputworker in threads
	bool engineFinished = false;
	std::thread engineThread (&STMEngine::Metropolis::run_sampler, 
			STMEngine::Metropolis(inits, outQueue, likelihood), maxIterations);
	std::thread outputThread (&STMOutput::OutputWorkerThread::start,
			STMOutput::OutputWorkerThread(outQueue, &engineFinished));
	
	// wait until engine completes
	engineThread.join();
	// signal to the outputworker to terminate
	engineFinished = true;	
	
	// wait for outputworker to finish
	outputThread.join();
	
	// any remaining cleanup
	delete outQueue;
	delete likelihood;
	
	return 0;
}