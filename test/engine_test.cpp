#include "../hdr/likelihood.hpp"
#include "../hdr/input.hpp"
#include "../hdr/parameters.hpp"
#include "../hdr/engine.hpp"
#include "../hdr/output.hpp"
#include <iostream>

int main(void)
{
	const char * parFileName = "inp/inits.txt";
	const char * transFileName = "inp/trans_short.txt";
	int maxIterations = 100;

int ln = 0;
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

	STMEngine::Metropolis engine (inits, outQueue, likelihood, 
			STMEngine::EngineOutputLevel::ExtraVerbose);

	engine.run_sampler(maxIterations);
}