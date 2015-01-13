#ifndef STM_ENGINE_H
#define STM_ENGINE_H

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <vector>

// forward declarations
namespace STMParameters {
	class STModelParameters;
}

namespace STMOutput {
	class OutputQueue;
}

namespace STMLikelihood {
	class Likelihood;
}


namespace STMEngine {

class Metropolis
{
	public:
	Metropolis(STMParameters::STModelParameters * const params, STMOutput::OutputQueue * 
			const queue, STMLikelihood::Likelihood * const lhood);
	~Metropolis();
	void run_sampler(int n);

	private:
	// private functions
	void auto_adapt();
	std::vector<double> do_sample(int n);
	double propose_parameter(int index);
	int select_parameter(double p, int index);
	double log_posterior_prob(std::vector<double> params, int index);
	void set_up_rng();
	
	// pointers to objects that the engine doesn't own, but that it uses
	STMParameters::STModelParameters * const parameters;
	STMOutput::OutputQueue * const outputQueue;
	STMLikelihood::Likelihood * const likelihood;

	// objects that the engine owns
	std::vector<std::vector<double> > currentSamples;
	static gsl_rng * rng;
	static int rngReferenceCount;

	// settings
	int outputBufferSize;
	int adaptationSampleSize;
	double adaptationRate;
};

} // namespace

#endif
