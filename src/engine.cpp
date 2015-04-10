#include "../hdr/engine.hpp"
#include "../hdr/output.hpp"
#include "../hdr/likelihood.hpp"
#include <ctime>
#include <cassert>
#include <string>
#include <cmath>
#include <algorithm> // std::random_shuffle
#include <iostream>
#include <iomanip>

using std::vector;

namespace {
	std::string timestamp()
	{
		time_t rawtime;
		time(&rawtime);
		struct tm * timeinfo = localtime(&rawtime);
		char fmtTime [20];
		strftime(fmtTime, 20, "%F %T", timeinfo);
		std::string ts(fmtTime);
		return ts;		
	}
}

namespace STMEngine {

/*
	Implementation of public functions
*/

Metropolis::Metropolis(const std::vector<STMParameters::ParameterSettings> & inits, 
		STMOutput::OutputQueue * const queue, STMLikelihood::Likelihood * const lhood,
		EngineOutputLevel outLevel, int thin, int burnin, bool rngSetSeed, int rngSeed) :
// objects that are not owned by the object
outputQueue(queue), likelihood(lhood),

// objects that we own or share
parameters(inits), rngSetSeed(rngSetSeed), rngSeed(rngSeed), burnin(burnin),
rng(gsl_rng_alloc(gsl_rng_mt19937), gsl_rng_free), outputLevel(outLevel), thinSize(thin),

// the parameters below have default values with no support for changing them
outputBufferSize(500), adaptationSampleSize(100), adaptationRate(1.1)
{
	// check pointers
	if(!queue || !lhood)
		throw std::runtime_error("Metropolis: passed null pointer on construction");
		
	if(thin < 1)
		throw std::runtime_error("Metropolis: thin interval must be greater than 0");
}


void Metropolis::run_sampler(int n)
{
	set_up_rng();
	
	if(!parameters.adapted())
		auto_adapt();

	int burninCompleted = parameters.iteration();
	int numCompleted = 0;
	while(numCompleted < n) {
		int sampleSize;
		if(burninCompleted < burnin)
		{
			sampleSize = ( (burnin - burninCompleted < outputBufferSize) ? 
					(burnin - burninCompleted) : outputBufferSize);
		}
		else
		{
			sampleSize = ((n - numCompleted < outputBufferSize) ? (n - numCompleted) : 
					outputBufferSize);
		}
		currentSamples.reserve(sampleSize);
		do_sample(sampleSize);
		
		if(burninCompleted < burnin)
		{
			burninCompleted += sampleSize;		
		}
		else
		{
			STMOutput::OutputBuffer buffer (currentSamples, parameters.names(),
					STMOutput::OutputKeyType::POSTERIOR);
			outputQueue->push(buffer);	// note that this may block if the queue is busy
			numCompleted += sampleSize;		
		}

		currentSamples.clear();
		
		/* if desired: some output with the current time */
		if(outputLevel >= EngineOutputLevel::Normal) {
			if(numCompleted == 0)
			{
				std::cerr << timestamp() << "   MCMC burnin iteration " << burninCompleted
						<< " of " << burnin << '\n';						
			}
			else
			{
				std::cerr << timestamp() << "   MCMC burnin iteration " << numCompleted << " of " 
						<< n << '\n';			
			}
		}
	}
}


/*
	Implementation of private functions: here there be dragons
*/

void Metropolis::auto_adapt()
{
	if(outputLevel >= EngineOutputLevel::Normal) {
		std::cerr << timestamp() << " Starting automatic adaptation\n";
	}
	std::vector<STM::ParName> parNames (parameters.names());		
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
		if(outputLevel >= EngineOutputLevel::Talkative) {
			std::cerr << "\n    " << timestamp() << " iter " << parameters.iteration() << ", acceptance rates:\n";
			std::cerr << "    " << parameters.str_acceptance_rates() << "\n";
			std::cerr << "    sampler variance:\n";
			std::cerr << "    " << parameters.str_sampling_variance() << "\n";
		}
		adaptationSampleSize *= 1.25;
	}
	parameters.reset();
	if(outputLevel >= EngineOutputLevel::Normal) {
		std::cerr << timestamp() << " Adaptation completed successfully\n";
	}
}


std::map<STM::ParName, double> Metropolis::do_sample(int n)
// n is the number of samples to take
// returns a map of acceptance rates keyed by parameter name
{
	// 	shuffle the order of parameters
	std::vector<STM::ParName> parNames (parameters.names());
	std::random_shuffle(parNames.begin(), parNames.end(), 
			[this](int n){ return gsl_rng_uniform_int(rng.get(), n); });
	
	std::map<STM::ParName, int> numAccepted;
	for(const auto & par : parNames)
		numAccepted[par] = 0;

	for(int i = 0; i < n; i++)
	{
		for(int j = 0; j < thinSize; j++)
		{
			// step through each parameter
			for(const auto & par : parNames) {
				STM::ParPair proposal = propose_parameter(par);
				numAccepted[par] += select_parameter(proposal);
			}
		}
		parameters.increment();
		currentSamples.push_back(parameters.current_state());

		//		if desired, some debugging output
		if(outputLevel >= EngineOutputLevel::Verbose) {
			std::cerr << "  iteration " << parameters.iteration() - 1 << 
					"    posterior probability: " << currentPosteriorProb << "\n";
			if(outputLevel >= EngineOutputLevel::ExtraVerbose) {
			std::ios_base::fmtflags oldflags = std::cerr.flags();
				std::streamsize oldprecision = std::cerr.precision();

				std::cerr << std::fixed << std::setprecision(3) << " ";
				STM::ParMap st = parameters.current_state();
				int coln = 0;
				for(auto pa : st) {
					std::cerr << std::setw(6) << pa.first << std::setw(8) << pa.second;
					if(++coln >= 7) {
						std::cerr << "\n ";
						coln = 0;
					}
				}
				std::cerr << "\n";
			
				std::cerr.flags (oldflags);
				std::cerr.precision (oldprecision);
			}
		}
}

	std::map<STM::ParName, double> acceptanceRates;
	for(const auto & par : parNames)
		acceptanceRates[par] = double(numAccepted[par]) / (n*thinSize);
	return acceptanceRates;
}


STM::ParPair Metropolis::propose_parameter(const 
		STM::ParName & par) const
{
	return STM::ParPair (par, parameters.current_state().at(par) + 
			gsl_ran_gaussian(rng.get(), parameters.sampler_variance(par)));
}


int Metropolis::select_parameter(const STM::ParPair & p)
// returns 1 if proposal is accepted, 0 otherwise
{
	STMParameters::STModelParameters proposal (parameters);
	proposal.update(p);
	
	double proposalLogPosterior = log_posterior_prob(proposal, p);
	double previousLogPosterior = log_posterior_prob(parameters, parameters.at(p.first));
	double acceptanceProb = exp(proposalLogPosterior - previousLogPosterior);

	// 	check for nan -- right now this is not being handled, but it should be
	if(std::isnan(acceptanceProb))
		acceptanceProb = 0;

	double testVal = gsl_rng_uniform(rng.get());
	if(testVal < acceptanceProb) {
		currentPosteriorProb = proposalLogPosterior;
		parameters.update(p);
		return 1;
	} else {
		currentPosteriorProb = previousLogPosterior;
		return 0;
	}
}


double Metropolis::log_posterior_prob(const STMParameters::STModelParameters & par, 
		const STM::ParPair & pair) const
{ return likelihood->compute_log_likelihood(par) + likelihood->log_prior(pair); }


void Metropolis::set_up_rng()
{
	int seed = (rngSetSeed ? rngSeed : (int) time(NULL));
	gsl_rng_set(rng.get(), seed);
}
} // namespace
