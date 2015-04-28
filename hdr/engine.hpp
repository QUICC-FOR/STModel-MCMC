#ifndef STM_ENGINE_H
#define STM_ENGINE_H

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <vector>
#include <map>
#include <memory>
#include "output.hpp"
#include "parameters.hpp"
#include "stmtypes.hpp"

namespace STMLikelihood {
	class Likelihood;
}


namespace STMEngine {

enum class EngineOutputLevel {
	Quiet=0,			// print nothing
	Normal=1,			// only print status messages
	Talkative=2,		// acceptance rates during adaptation
	Verbose=3,			// prints the likelihood at each iteration
	ExtraVerbose=4		// prints parameter values at each iteration
};


class Metropolis
{
	public:
	Metropolis(const std::vector<STMParameters::ParameterSettings> & inits, 
			STMOutput::OutputQueue * const queue, STMLikelihood::Likelihood * 
			const lhood, EngineOutputLevel outLevel = EngineOutputLevel::Normal, 
			STMOutput::OutputOptions outOpt = STMOutput::OutputOptions(),
			int thin = 1, int burnin = 0, bool rngSetSeed = false, int rngSeed = 0);
//	Metropolis(const Metropolis & m);
//	Metropolis & operator= (const Metropolis &m);
// 	~Metropolis();
	void run_sampler(int n);

	private:
	// private functions
	void auto_adapt();
	std::map<std::string, double> do_sample(int n);
	STM::ParPair propose_parameter(const 
			STM::ParName & par) const;
	int select_parameter(const STM::ParPair & p);
	double log_posterior_prob(const STMParameters::STModelParameters & par, 
			const STM::ParPair & pair) const;
	void set_up_rng();
	void serialize_all() const;
	std::string serialize(char sep) const;
	static std::string version();

	
	// pointers to objects that the engine doesn't own, but that it uses
	STMOutput::OutputQueue * outputQueue;
	STMLikelihood::Likelihood * likelihood;

	// objects that the engine owns
	
	// data and settings that should be saved in resumeData
	STMParameters::STModelParameters parameters;
	std::shared_ptr<gsl_rng> rng;
	double currentPosteriorProb;
	double adaptationRate;

	// settings
	int outputBufferSize;
	int thinSize;
	int burnin;
	int adaptationSampleSize;
	bool rngSetSeed;
	int rngSeed;
	EngineOutputLevel outputLevel;
	STMOutput::OutputOptions posteriorOptions;
	
	// data that do not need to be saved in resumeData
	std::string resumeDataFileName;
	std::vector<STM::ParMap> currentSamples;
	bool saveResumeData;
};

} // namespace

#endif
