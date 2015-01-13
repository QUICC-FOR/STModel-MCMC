#include "../hdr/engine.hpp"
#include "../hdr/parameters.hpp"
#include "../hdr/output.hpp"
#include "../hdr/likelihood.hpp"
#include <ctime>
#include <cassert>
#include <string>
#include <cmath>

using std::vector;

namespace STMEngine {

/*
	note: the use of the rng pointer here is borken and NOT thread safe;
	the reference counter gets decremented with each destruction, but is not incremented
	on copy
	also, the rng pointer and ref count are static
	
	1. need to make copy constructor and overload = that includes incrementing, but not
			setting up the rng (unless it is null)
	2. need to make the variable non-static; EACH independent engine has it's very own
			rng
	3. another possibility would be to run set_up_rng() ONLY when the engine is started
			rather than on construction. once an engine is started, it shouldn't be
			copied anymore. could add extra safety by defining the copy constructors,
			and by setting the pointer to NULL in the default constructor
		--note, I like #3; it delays construction of the RNG until it is needed
*/

// static variable definition
gsl_rng * Metropolis::rng = NULL;
int Metropolis::rngReferenceCount = 0;

/*
	Implementation of public functions
*/

Metropolis::Metropolis(STMParameters::STModelParameters * const params, 
		STMOutput::OutputQueue * const queue, STMLikelihood::Likelihood * const lhood) :
// objects that are not owned by the object
parameters(params), outputQueue(queue), likelihood(lhood),

// the parameters below have default values with no support for changing them
outputBufferSize(50000), adaptationSampleSize(10000), adaptationRate(1.1)
{
	// check pointers
	if(!queue || !params || !lhood)
		throw std::runtime_error("Metropolis: passed null pointer on construction");
	set_up_rng();
}


Metropolis::~Metropolis()
{
	rngReferenceCount--;
	if(rngReferenceCount == 0) {
		gsl_rng_free(rng);
		rng = NULL;
	}
}

void Metropolis::run_sampler(int n)
{
	if(!parameters->adapted())
		auto_adapt();

	int numCompleted = 0;
	while(numCompleted < n) {
		int sampleSize = ((n - numCompleted < outputBufferSize) ? (n - numCompleted) : outputBufferSize);
		currentSamples.reserve(sampleSize);
		do_sample(sampleSize);
		STMOutput::OutputBuffer buffer (currentSamples, STMOutput::OutputKeyType::POSTERIOR);
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
	while(!parameters->adapted()) {
		parameters->set_acceptance_rates(do_sample(adaptationSampleSize));
		for(int i = 0; i < parameters->size(); i++) {
			switch(parameters->not_adapted(i)) {
				case -1 :
					parameters->set_sampler_variance(i, parameters->sampler_variance(i) / adaptationRate);
					break;
				case 1 :
					parameters->set_sampler_variance(i, parameters->sampler_variance(i) * adaptationRate);
					break;
				default:
					break;
			}
		}
	}
	parameters->reset();
}


vector<double> Metropolis::do_sample(int n)
// n is the number of samples to take
// returns a vector of acceptance rates
{
	int numParams = parameters->size();
	vector<int> numAccepted (numParams, 0);
	vector<double> acceptanceRates (numParams, 0);
	
	// we will shuffle the order each time we sample; this sets up an array of indices to do so
	int indices [numParams];
	for(int i = 0; i < numParams; i++) indices[i] = i;

	for(int i = 0; i < n; i++) {
		gsl_ran_shuffle(rng, indices, numParams, sizeof(int));
		for(int j = 0; j < numParams; j++) {
			int index = indices[j];
			double proposedVal = propose_parameter(index);
			numAccepted[index] += select_parameter(proposedVal, index);
		}
		parameters->increment();	// tell parameters we have done one complete iteration
		currentSamples.push_back(parameters->current_state());

//		if desired, some debugging output	
// 		#ifdef SAMPLER_DEBUG
// 			if(verbose > 1)
// 				cerr << vec_to_str(currentState) << '\n';
// 		#endif
		
	}

	for(int i = 0; i < numParams; i++)
		acceptanceRates[i] = double(numAccepted[i]) / n;
	return acceptanceRates;
}


double Metropolis::propose_parameter(int index)
{
	return parameters->current_state()[index] + gsl_ran_gaussian(rng, 
			parameters->sampler_variance(index));
}


int Metropolis::select_parameter(double p, int index)
{
	// returns 1 if proposal is accepted, 0 otherwise
	vector<double> proposal = parameters->current_state();
	proposal[index] = p;
	
	double acceptanceProb = exp(log_posterior_prob(proposal, index) - 
			log_posterior_prob(parameters->current_state(), index));
	double testVal = gsl_rng_uniform(rng);
	if(testVal < acceptanceProb) {
		parameters->update(index, p);
		return 1;
	} else
		return 0;
}


double Metropolis::log_posterior_prob(vector<double> params, int index)
{
	return likelihood->compute_log_likelihood(params) + 
			likelihood->log_prior(index, params[index]);
}


void Metropolis::set_up_rng()
{
	if(!rng) {
		rng = gsl_rng_alloc(gsl_rng_mt19937);
		assert(rng);
		rngReferenceCount = 1;
		gsl_rng_set(rng, (int) time(NULL));
	} else
		rngReferenceCount++;
}

}