#include "../hdr/engine.hpp"
#include "../hdr/output.hpp"
#include "../hdr/likelihood.hpp"
#include <ctime>
#include <cassert>
#include <string>
#include <cmath>
#include <algorithm> // std::random_shuffle

using std::vector;

namespace STMEngine {

/*
	Implementation of public functions
*/

Metropolis::Metropolis(const std::vector<STMParameters::ParameterSettings> & inits, 
		STMOutput::OutputQueue * const queue, STMLikelihood::Likelihood * const lhood,
		bool rngSetSeed, int rngSeed) :
// objects that are not owned by the object
outputQueue(queue), likelihood(lhood),

// objects that we own
parameters(inits), rngSetSeed(rngSetSeed), rngSeed(rngSeed), rng(NULL),

// the parameters below have default values with no support for changing them
outputBufferSize(50000), adaptationSampleSize(10000), adaptationRate(1.1)
{
	// check pointers
	if(!queue || !lhood)
		throw std::runtime_error("Metropolis: passed null pointer on construction");
}


Metropolis::Metropolis(const Metropolis & m) : outputQueue(m.outputQueue), 
		likelihood(m.likelihood), parameters(m.parameters), 
		currentSamples(m.currentSamples), outputBufferSize(m.outputBufferSize), 
		adaptationSampleSize(m.adaptationSampleSize), adaptationRate(m.adaptationRate), 
		rngSetSeed(m.rngSetSeed), rngSeed(m.rngSeed)
{ if(m.rng) gsl_rng_memcpy(rng, m.rng); }

Metropolis & Metropolis::operator= (const Metropolis &m)
{
	outputQueue = m.outputQueue; 
	likelihood = m.likelihood;
	parameters = m.parameters;
	currentSamples = m.currentSamples;
	outputBufferSize = m.outputBufferSize;
	adaptationSampleSize = m.adaptationSampleSize;
	adaptationRate = m.adaptationRate;
	rngSetSeed = m.rngSetSeed;
	rngSeed = m.rngSeed;
	if(m.rng) gsl_rng_memcpy(rng, m.rng);
}



Metropolis::~Metropolis()
{ if(rng) gsl_rng_free(rng); }

void Metropolis::run_sampler(int n)
{
	if(!rng)
		set_up_rng();
	
	if(!parameters.adapted())
		auto_adapt();

	int numCompleted = 0;
	while(numCompleted < n) {
		int sampleSize = ((n - numCompleted < outputBufferSize) ? (n - numCompleted) : 
				outputBufferSize);
		currentSamples.reserve(sampleSize);
		do_sample(sampleSize);
		STMOutput::OutputBuffer buffer (currentSamples, parameters.names(),
				STMOutput::OutputKeyType::POSTERIOR);
		currentSamples.clear();
		outputQueue->push(buffer);	// note that this may block if the queue is busy
		numCompleted += sampleSize;
		
		/* if desired: some output with the current time 
		time_t rawtime;
		time(&rawtime);
		struct tm * timeinfo = localtime(&rawtime);
		char fmtTime [20];
		strftime(fmtTime, 20, "%F %T", timeinfo);
		cerr << fmtTime << "   MCMC Iteration " << samplesTaken << "; current job completed " << numCompleted << " of " << n << '\n';
		*/
	}
}


/*
	Implementation of private functions: here there be dragons
*/

void Metropolis::auto_adapt()
{
	std::vector<STMParameters::STMParameterNameType> parNames (parameters.names());		
	while(!parameters.adapted()) {
		parameters.set_acceptance_rates(do_sample(adaptationSampleSize));
		for(const auto & par : parNames) {
			switch(parameters.not_adapted(par)) {
			case -1 :
				parameters.set_sampler_variance(par, parameters.sampler_variance(par) / 
						adaptationRate);
				break;
			case 1 :
				parameters.set_sampler_variance(par, parameters.sampler_variance(par) * 
						adaptationRate);
				break;
			default:
				break;
			}
		}
	}
	parameters.reset();
}


std::map<STMParameters::STMParameterNameType, double> Metropolis::do_sample(int n)
// n is the number of samples to take
// returns a map of acceptance rates keyed by parameter name
{
	// 	shuffle the order of parameters
	std::vector<STMParameters::STMParameterNameType> parNames (parameters.names());
	std::random_shuffle(parNames.begin(), parNames.end(), 
			[this](int n){ return gsl_rng_uniform_int(rng, n); });
	
	std::map<STMParameters::STMParameterNameType, int> numAccepted;
	for(const auto & par : parNames)
		numAccepted[par] = 0;

	for(int i = 0; i < n; i++) {
		// step through each parameter
		for(const auto & par : parNames) {
			STMParameters::STMParameterPair proposal = propose_parameter(par);
			numAccepted[par] += select_parameter(proposal);
		}
		parameters.increment();
		currentSamples.push_back(parameters.current_state());

//		if desired, some debugging output	
// 		#ifdef SAMPLER_DEBUG
// 			if(verbose > 1)
// 				cerr << vec_to_str(currentState) << '\n';
// 		#endif
}

	std::map<STMParameters::STMParameterNameType, double> acceptanceRates;
	for(const auto & par : parNames)
		acceptanceRates[par] = double(numAccepted[par]) / n;
	return acceptanceRates;
}


STMParameters::STMParameterPair Metropolis::propose_parameter(const 
		STMParameters::STMParameterNameType & par) const
{
	return STMParameters::STMParameterPair (par, parameters.current_state().at(par) + 
			gsl_ran_gaussian(rng, parameters.sampler_variance(par)));
}


int Metropolis::select_parameter(const STMParameters::STMParameterPair & p)
// returns 1 if proposal is accepted, 0 otherwise
{
	STMParameters::STModelParameters proposal (parameters);
	proposal.update(p);
	
	double acceptanceProb = exp(log_posterior_prob(proposal, p) - 
			log_posterior_prob(parameters, parameters.at(p.first)));
	double testVal = gsl_rng_uniform(rng);
	if(testVal < acceptanceProb) {
		parameters.update(p);
		return 1;
	} else
		return 0;
}


double Metropolis::log_posterior_prob(const STMParameters::STModelParameters & par, 
		const STMParameters::STMParameterPair & pair) const
{ return likelihood->compute_log_likelihood(par) + likelihood->log_prior(pair); }


void Metropolis::set_up_rng()
{
	rng = gsl_rng_alloc(gsl_rng_mt19937);
	assert(rng);
	int seed = (rngSetSeed ? rngSeed : (int) time(NULL));
	gsl_rng_set(rng, seed);
}
} // namespace
