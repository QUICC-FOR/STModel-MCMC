#include <thread>
#include <vector>
#include <iostream>
#include <unistd.h> // for getopt
#include <cstdlib> // atoi

#include "../hdr/engine.hpp"
#include "../hdr/output.hpp"
#include "../hdr/input.hpp"
#include "../hdr/parameters.hpp"
#include "../hdr/likelihood.hpp"
#include "../hdr/model.hpp"

struct ModelSettings 
{
	const char * parFileName;
	const char * transFileName;
	int thin;
	int burnin;
	int numThreads;
	int maxIterations;
	STMEngine::EngineOutputLevel verbose;
	
	ModelSettings() : parFileName("inp/inits.txt"), transFileName("inp/trans.txt"),
			maxIterations(100), verbose(STMEngine::EngineOutputLevel::Normal), thin(1), 
			burnin(0), numThreads(8) {}
};

void parse_args(int argc, char **argv, ModelSettings & s);
void print_help();


int main(int argc, char ** argv)
{
	// handle arguments, set default values
	ModelSettings settings;
	parse_args(argc, argv, settings);
	
	// handle input data
	std::vector<STMParameters::ParameterSettings> inits;
	std::vector<STMModel::STMTransition> transitionData;
	std::map<std::string, STMLikelihood::PriorDist> priors;
	try {
		STMInput::STMInputHelper inp (settings.parFileName, settings.transFileName);
		inits = inp.parameter_inits();
		priors = inp.priors();
		transitionData = inp.transitions();
	}
	catch (std::runtime_error &e) {
		std::cerr << e.what() << '\n';
		exit(1);
	}
	
	
	// create and initialize main objects
	STMOutput::OutputQueue * outQueue = new STMOutput::OutputQueue;
	STMLikelihood::Likelihood * likelihood = new STMLikelihood::Likelihood 
			(transitionData, priors, settings.numThreads);

	// spawn engine and outputworker in threads
	bool engineFinished = false;
	std::thread engineThread (&STMEngine::Metropolis::run_sampler, 
			STMEngine::Metropolis(inits, outQueue, likelihood, settings.verbose, 
			settings.thin, settings.burnin), settings.maxIterations);
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


void parse_args(int argc, char **argv, ModelSettings & s)
{
	int thearg;
	while((thearg = getopt(argc, argv, "hp:t:n:i:b:c:v:")) != -1)
	{
		switch(thearg)
		{
			case 'h':
				print_help();
				break;
			case 'p':			
				s.parFileName = optarg;
				break;
			case 't':
				s.transFileName = optarg;
				break;
			case 'n':
				s.thin = atoi(optarg);
				break;
			case 'i':
				s.maxIterations = atoi(optarg);
				break;
			case 'b':
				s.burnin = atoi(optarg);
				break;
			case 'c':
				s.numThreads = atoi(optarg);
				break;
			case 'v':
				s.verbose = STMEngine::EngineOutputLevel(atoi(optarg));
				break;
			case '?':
				print_help();
				break;
		}
	}
}

void print_help()
{
	std::cerr << "Command line options:\n";
	std::cerr << "    -h:             display this help\n";
	std::cerr << "    -p <filname>:   specifies the location of the parameter information file\n";
	std::cerr << "    -t <filname>:   specifies the location of the transition data\n";
	std::cerr << "    -n <integer>:   thinning interval\n";
	std::cerr << "    -i <integer>:   specifies the number of mcmc iterations (after adaptation)\n";
	std::cerr << "    -b <integer>:   set the number of burn in samples\n";
	std::cerr << "    -c <integer>:   set the number of cores on which to compute the model (default 8)\n";
	std::cerr << "    -v <integer>:   set verbosity; control level of output as follows:\n";	
	std::cerr << "                         0: Quiet; print nothing\n";	
	std::cerr << "                         1: Normal; only print status messages\n";	
	std::cerr << "                         2: Talkative; prints acceptance rates during adaptation\n";	
	std::cerr << "                         3: Verbose; prints the likelihood at each iteration\n";	
	std::cerr << "                         4: Extra Verbose; prints parameter values at each iteration\n";	
	exit(1);
}