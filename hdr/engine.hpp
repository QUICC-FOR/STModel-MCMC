#ifndef STM_ENGINE_H
#define STM_ENGINE_H

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <vector>
#include <map>
#include "parameters.hpp"

// forward declarations
namespace STMOutput {
	class OutputQueue;
}

namespace STMLikelihood {
	class Likelihood;
}


namespace STMEngine {

enum class EngineOutputLevel {
	Quiet,			// print nothing
	Normal,			// only print status messages
	Verbose,		// prints the likelihood at each iteration
	ExtraVerbose	// prints parameter values at each iteration
};

class Metropolis
{
	public:
	Metropolis(const std::vector<STMParameters::ParameterSettings> & inits, 
			STMOutput::OutputQueue * const queue, STMLikelihood::Likelihood * 
			const lhood, EngineOutputLevel outLevel = EngineOutputLevel::Normal, 
			bool rngSetSeed = false, int rngSeed = 0);
	Metropolis(const Metropolis & m);
	Metropolis & operator= (const Metropolis &m);
	~Metropolis();
	void run_sampler(int n);

	private:
	// private functions
	void auto_adapt();
	std::map<std::string, double> do_sample(int n);
	STMParameters::STMParameterPair propose_parameter(const 
			STMParameters::STMParameterNameType & par) const;
	int select_parameter(const STMParameters::STMParameterPair & p);
	double log_posterior_prob(const STMParameters::STModelParameters & par, 
			const STMParameters::STMParameterPair & pair) const;
	void set_up_rng();
	
	// pointers to objects that the engine doesn't own, but that it uses
	STMOutput::OutputQueue * outputQueue;
	STMLikelihood::Likelihood * likelihood;

	// objects that the engine owns
	STMParameters::STModelParameters parameters;
	std::vector<STMParameters::STMParameterMap> currentSamples;
	gsl_rng * rng;
	double currentPosteriorProb;

	// settings
	int outputBufferSize;
	int adaptationSampleSize;
	double adaptationRate;
	bool rngSetSeed;
	int rngSeed;
	EngineOutputLevel outputLevel;
};

} // namespace

#endif
