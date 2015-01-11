#include "../hdr/engine.hpp"
#include "../hdr/parameters.hpp"
#include "../hdr/output.hpp"
#include <ctime>
#include <cassert>
#include <string>
#include <cmath>

namespace STMEngine {

// static variable definition
gsl_rng * Metropolis::rng = NULL;
int Metropolis::rngReferenceCount = 0;

/*
	Implementation of public functions
*/

Metropolis::Metropolis() :
outputBufferSize(50000)
{
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
	while(!parameterHistory->adapted())
		auto_adapt();

	int numCompleted = 0;
	while(numCompleted < n) {
		int sampleSize = ((n - numCompleted < outputBufferSize) ? (n - numCompleted) : outputBufferSize);
		currentSamples.reserve(sampleSize);
		do_sample(sampleSize);
		outputQueue->push(currentSamples);	// note that this may block if the queue is busy
		currentSamples.clear();
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
	// probably the best way to adapt the samplers is to truly discard these things
	// just clear the samples when done, save the tuning parameters, and reset the state
	// of the parameters object to the initial state
	while(!parameters ->adapted()) {
		vector<double> acceptanceRates = do_sample(adaptationSampleSize);
		bool adapted = true;
		for(int i = 0; i < acceptanceRates.size(); i++) {
			// if the acceptance rate is way off, we double the adaptation rate
			if(acceptanceRates[i] < targetAcceptanceInterval[0]/2.0) {
				parameters->set_tuning(i, parameters->tuning(i) / 2.0*adaptationRate);
				adapted = false;
			} else if(acceptanceRates[i] < targetAcceptanceInterval[0]) {
				parameters->set_tuning(i, parameters->tuning(i) / adaptationRate);
				adapted = false;
			} else if(acceptanceRates[i] > targetAcceptanceInterval[1]*2.0) {
				parameters->set_tuning(i, parameters->tuning(i) * 2.0*adaptationRate);
				adapted = false;
			} else if(acceptanceRates[i] > targetAcceptanceInterval[1]) {
				parameters->set_tuning(i, parameters->tuning(i) * adaptationRate);
				adapted = false;
			}
		}
		if(adapted) parameters->adapted() = true;
	}
	parameters->reset();
}


vector<double> Metropolis::do_sample(int n)
// n is the number of samples to take
{
	int numParams = parameters->num_params();
	vector<int> numAccepted (numParams, 0);
	vector<double> acceptanceRates (numParams, 0);
	
	// we will shuffle the order each time we sample; this sets up an array of indices to do so
	int indices [numParams];
	for(int i = 0; i < numParams; i++) indices[i] = i;

	for(int i = 0; i < n; i++) {
		gsl_ran_shuffle(rng, indices, nParams, sizeof(int));
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
	return parameters->current_state()[index] + gsl_ran_gaussian(rng, parameters->tuning()[i]);
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


double Metropolis::log_posterior_prob(vector<double> params, int index);
{
	return likelihood->compute_log_likelihood(params) + likelihood->log_prior(params[index], index);
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