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
#include "../hdr/stmtypes.hpp"

struct ModelSettings 
{
	const char * parFileName;
	const char * transFileName;
	int thin;
	int burnin;
	int numThreads;
	unsigned int targetInterval;
	STMOutput::OutputMethodType outMethod;
	std::string outDir;
	int maxIterations;
	bool resume;
	const char * resumeFile;
	
	STMEngine::EngineOutputLevel verbose;
	
	ModelSettings() : parFileName("inp/inits.txt"), transFileName("inp/trans.txt"),
			maxIterations(100), verbose(STMEngine::EngineOutputLevel::Normal), thin(1), 
			burnin(0), targetInterval(1), numThreads(8), outDir("output"), resume(false),
			outMethod(STMOutput::OutputMethodType::CSV), resumeFile("") {}
};

void parse_args(int argc, char **argv, ModelSettings & s);
void print_help();


int main(int argc, char ** argv)
{
	// handle arguments, set default values
	ModelSettings settings;
	parse_args(argc, argv, settings);
	
	// handle input data
	std::vector<STMModel::STMTransition> transitionData;
	std::map<std::string, STMInput::SerializationData> resumeData;
	std::vector<STMParameters::ParameterSettings> inits;

	try {
		STMInput::STMInputHelper inp (settings.transFileName, STMInput::InputType::transitions);
		transitionData = inp.transitions();
		std::cerr << "Loaded transition data\n";
	}
	catch (std::runtime_error &e) {
		std::cerr << e.what() << '\n';
		exit(1);
	}

	STMLikelihood::Likelihood * likelihood = nullptr;
	
	if(settings.resume)
	{
		STMInput::STMInputHelper inp (settings.resumeFile, STMInput::InputType::resume);
		resumeData = inp.resume_data();
		std::cerr << "Read resume data\n";
		
		
		likelihood = new STMLikelihood::Likelihood (resumeData.at("Likelihood"), 
				resumeData.at("Parameters").at("parNames"), transitionData);
		std::cerr << "Built likelihood\n";
	} else // not resuming
	{
		std::map<std::string, STMLikelihood::PriorDist> priors;
		try {
			STMInput::STMInputHelper inp (settings.parFileName, STMInput::InputType::parameters);
			inits = inp.parameter_inits();
			priors = inp.priors();
			std::cerr << "Read parameter data\n";
		}
		catch (std::runtime_error &e) {
			std::cerr << e.what() << '\n';
			exit(1);
		}
		likelihood = new STMLikelihood::Likelihood 
			(transitionData, settings.transFileName, priors, settings.numThreads,
					 settings.targetInterval);
		std::cerr << "Built likelihood\n";
	}

	
	
	STMOutput::OutputQueue * outQueue = new STMOutput::OutputQueue;

	// spawn engine and outputworker in threads
	bool engineFinished = false;
	if(settings.resume)
	{
		std::thread engineThread (&STMEngine::Metropolis::run_sampler, 
				STMEngine::Metropolis(resumeData, likelihood, outQueue), 
				settings.maxIterations);
		std::cerr << "Engine resumed successfully\n";
		std::thread outputThread (&STMOutput::OutputWorkerThread::start,
				STMOutput::OutputWorkerThread(outQueue, &engineFinished));	

		// wait until engine completes
		engineThread.join();
		// signal to the outputworker to terminate
		engineFinished = true;	
	
		// wait for outputworker to finish
		outputThread.join();
	} else
	{
		std::thread engineThread (&STMEngine::Metropolis::run_sampler, 
				STMEngine::Metropolis(inits, outQueue, likelihood, settings.verbose, 
				STMOutput::OutputOptions(settings.outDir, settings.outMethod), settings.thin, 
				settings.burnin), settings.maxIterations);
		std::cerr << "Engine started successfully\n";
		std::thread outputThread (&STMOutput::OutputWorkerThread::start,
				STMOutput::OutputWorkerThread(outQueue, &engineFinished));
		
		// wait until engine completes
		engineThread.join();
		// signal to the outputworker to terminate
		engineFinished = true;	
	
		// wait for outputworker to finish
		outputThread.join();
	}
	
	// any remaining cleanup
	delete outQueue;
	delete likelihood;
	
	return 0;
}


void parse_args(int argc, char **argv, ModelSettings & s)
{
	int thearg;
	while((thearg = getopt(argc, argv, "hsr:p:t:o:n:i:b:l:c:v:")) != -1)
	{
		switch(thearg)
		{
			case 'h':
				print_help();
				break;
			case 's':
				s.outMethod = STMOutput::OutputMethodType::STDOUT;
				break;
			case 'r':
				s.resume = true;
				s.resumeFile = optarg;
				break;
			case 'p':			
				s.parFileName = optarg;
				break;
			case 't':
				s.transFileName = optarg;
				break;
			case 'o':
				s.outDir = optarg;
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
			case 'l':
				s.targetInterval = atoi(optarg);
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
	std::cerr << "    -s:             output to standard out (default is CSV files)\n";
	std::cerr << "    -r <filname>:   resume the sampler from the file indicated\n";
	std::cerr << "                         note that the transitionData are not saved with the resume data\n";		
	std::cerr << "                         so reloading it with the -t option is required\n";		
	std::cerr << "                         specifying the number of iterations with -i is also required\n";		
	std::cerr << "                         -p, -o, -n, -b, -c, and -v will be ignored, as they are loaded from the resume data\n";		
	std::cerr << "    -p <filname>:   specifies the location of the parameter information file\n";
	std::cerr << "    -t <filname>:   specifies the location of the transition data\n";
	std::cerr << "    -o <directory>: set the output directory name (ignored when -s is set)\n";
	std::cerr << "    -n <integer>:   thinning interval\n";
	std::cerr << "    -i <integer>:   specifies the number of mcmc iterations (after adaptation)\n";
	std::cerr << "    -b <integer>:   set the number of burn in samples\n";
	std::cerr << "    -c <integer>:   set the number of cores on which to compute the model (default 8)\n";
	std::cerr << "    -l <integer>:   set the target transition interval (in years) for the parameters (default 1)\n";
	std::cerr << "    -v <integer>:   set verbosity; control level of output as follows:\n";	
	std::cerr << "                         0: Quiet; print nothing\n";	
	std::cerr << "                         1: Normal; only print status messages\n";	
	std::cerr << "                         2: Talkative; prints acceptance rates during adaptation\n";	
	std::cerr << "                         3: Verbose; prints the likelihood at each iteration\n";	
	std::cerr << "                         4: Extra Verbose; prints parameter values at each iteration\n";	
	exit(1);
}