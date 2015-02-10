#include <thread>
#include <vector>
#include <iostream>
#include "../hdr/engine.hpp"
#include "../hdr/output.hpp"
#include "../hdr/input.hpp"
#include "../hdr/parameters.hpp"
#include "../hdr/likelihood.hpp"

int main(int argc, char ** argv)
{
	const char * parFileName = "inp/inits.txt";
	const char * transFileName = "inp/trans_short.txt";
	int maxIterations = 200;

	std::vector<STMParameters::ParameterSettings> inits;
	std::vector<STMLikelihood::Transition> transitionData;
	std::map<std::string, STMLikelihood::PriorDist> priors;

	STMInput::STMInputHelper inp (parFileName, transFileName);
	inits = inp.parameter_inits();
	priors = inp.priors();
	transitionData = inp.transitions();

	STMLikelihood::Likelihood * likelihood = new STMLikelihood::Likelihood 
			(transitionData, priors);
	STMOutput::OutputQueue * outQueue = new STMOutput::OutputQueue;



	// spawn engine and outputworker in threads
	bool engineFinished = false;
std::cerr << "Starting engine\n";
 	std::thread engineThread (&STMEngine::Metropolis::run_sampler, 
 			STMEngine::Metropolis(inits, outQueue, likelihood), maxIterations);
std::cerr << "Starting output worker\n";
	std::thread outputThread (&STMOutput::OutputWorkerThread::start,
			STMOutput::OutputWorkerThread(outQueue, &engineFinished));
	
	// wait until engine completes
	engineThread.join();

	// signal to the outputworker to terminate
std::cerr << "Engine finished\n";
	engineFinished = true;	
	
	// wait for outputworker to finish
	outputThread.join();
	
	// any remaining cleanup
	delete outQueue;
	delete likelihood;
	
	return 0;
}