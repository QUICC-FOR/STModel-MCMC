#include "../hdr/engine.hpp"
#include "../hdr/likelihood.hpp"
#include "../hdr/input.hpp"
#include <ctime>
#include <string>
#include <cmath>
#include <algorithm> // std::random_shuffle
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <gsl/gsl_randist.h>


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

	std::string engineVersion = "Metropolis1.0";
}

namespace STMEngine {


/*
	Implementation of public functions
*/

Metropolis::Metropolis(const std::vector<STMParameters::ParameterSettings> & inits, 
		STMOutput::OutputQueue * const queue, STMLikelihood::Likelihood * const lhood,
		EngineOutputLevel outLevel, STMOutput::OutputOptions outOpt, int thin, int burnin, 
		bool rngSetSeed, int rngSeed) :
// objects that are not owned by the object
outputQueue(queue), likelihood(lhood),

// objects that we own or share
parameters(inits), rngSetSeed(rngSetSeed), rngSeed(rngSeed), burnin(burnin),
rng(gsl_rng_alloc(gsl_rng_mt19937), gsl_rng_free), outputLevel(outLevel), thinSize(thin),
posteriorOptions(outOpt),

// the parameters below have default values with no support for changing them
minAdaptationLoops(5), maxAdaptationLoops(25), adaptationSampleSize(500), 
outputBufferSize(500)
{
	// check pointers
	if(!queue || !lhood)
		throw std::runtime_error("Metropolis: passed null pointer on construction");
		
	if(thin < 1)
		throw std::runtime_error("Metropolis: thin interval must be greater than 0");
		
	if(posteriorOptions.method() == STMOutput::OutputMethodType::STDOUT)
		saveResumeData = false;
	else
		saveResumeData = true;
		
	if(saveResumeData) serialize_all();
}


// unserialization constructor
Metropolis::Metropolis(std::map<std::string, STMInput::SerializationData> & sd, 
		STMLikelihood::Likelihood * const lhood, STMOutput::OutputQueue * const queue) : 
		likelihood(lhood), outputQueue(queue), parameters(sd.at("Parameters")),
		posteriorOptions(sd.at("OutputOptions")), 
		rng(gsl_rng_alloc(gsl_rng_mt19937), gsl_rng_free), saveResumeData(true)
{
	STMInput::SerializationData esd = sd.at("Metropolis");
	// check versions and return error if no match
	std::string saveVersion = esd.at("version")[0];
	if(saveVersion != engineVersion)
	{
		std::ostringstream msg;
		msg << "Error, serialized input version = '" << saveVersion;
		msg << ",' does not match program version = '" << engineVersion << "'";
		throw(msg.str());
	}
				
	// check pointers
	if(!queue || !lhood)
		throw std::runtime_error("Metropolis: passed null pointer on construction");

	currentPosteriorProb = STMInput::str_convert<double>(esd.at("currentPosteriorProb")[0]);
	outputBufferSize = STMInput::str_convert<int>(esd.at("outputBufferSize")[0]);
	thinSize = STMInput::str_convert<int>(esd.at("thinSize")[0]);
	burnin = STMInput::str_convert<int>(esd.at("burnin")[0]);
	adaptationSampleSize = STMInput::str_convert<int>(esd.at("adaptationSampleSize")[0]);
	adaptationSampleSize = STMInput::str_convert<int>(esd.at("minAdaptationLoops")[0]);
	adaptationSampleSize = STMInput::str_convert<int>(esd.at("maxAdaptationLoops")[0]);
	rngSeed = STMInput::str_convert<int>(esd.at("rngSeed")[0]);
	rngSetSeed = STMInput::str_convert<bool>(esd.at("rngSetSeed")[0]);
	outputLevel = EngineOutputLevel(STMInput::str_convert<int>(esd.at("outputLevel")[0]));

}


std::string Metropolis::version()
{ return engineVersion; }


void Metropolis::run_sampler(int n)
{
	set_up_rng();

if(not parameters.adapted())
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
					STMOutput::OutputKeyType::posterior, posteriorOptions);
			outputQueue->push(buffer);	// note that this may block if the queue is busy
			numCompleted += sampleSize;		
		}

		currentSamples.clear();
		if(saveResumeData)
			serialize_all();
		
		/* if desired: some output with the current time */
		if(outputLevel >= EngineOutputLevel::Normal) {
			if(numCompleted == 0)
			{
				std::cerr << timestamp() << "   MCMC burnin iteration " << burninCompleted
						<< " of " << burnin << std::endl;						
			}
			else
			{
				std::cerr << timestamp() << "   MCMC iteration " << numCompleted << " of " 
						<< n << std::endl;			
			}
		}
	}
}




/*
	Implementation of private functions: here there be dragons
*/

void Metropolis::auto_adapt()
{
	if(outputLevel >= EngineOutputLevel::Normal) 
	{
		std::cerr << timestamp() << " Starting automatic adaptation" << std::endl;
	}
	std::vector<STM::ParName> parNames (parameters.names());
	
	// disable thinning for the adaptation phase
	int oldThin = thinSize;
	thinSize = 1;
	
	int nLoops = 0;
	while(nLoops < minAdaptationLoops and (not parameters.adapted() or nLoops >= maxAdaptationLoops))	
	{
		parameters.set_acceptance_rates(do_sample(adaptationSampleSize));
		for(const auto & par : parNames) {
			/* adaptation status returns -1 for too low, +1 for too high, so this will 
			make the variance larger if the adaptation rate is too low and smaller if
			it is too high */
			double ratio = pow(parameters.acceptance_rate(par) / 
					parameters.optimal_acceptance_rate(), 
					parameters.adaptation_status(par));
			parameters.set_sampler_variance(par, ratio * parameters.sampler_variance(par));
			
		}
		if(outputLevel >= EngineOutputLevel::Talkative) {
			std::cerr << "\n    " << timestamp() << " iter " << parameters.iteration() << ", acceptance rates:\n";
			std::cerr << "    " << parameters.str_acceptance_rates(isatty(fileno(stderr))) << "\n";
			std::cerr << "    sampler variance:\n";
			std::cerr << "    " << parameters.str_sampling_variance(isatty(fileno(stderr))) << std::endl;
		}
		currentSamples.clear();
		if(saveResumeData)
			serialize_all();
	}
	parameters.reset(); // adaptation samples are not included in the burnin period
	if(outputLevel >= EngineOutputLevel::Normal) {
		std::cerr << timestamp() << " Adaptation completed successfully" << std::endl;
	}
	
	thinSize = oldThin;
}

void Metropolis::serialize_all() const
{
	if(!currentSamples.empty())
		throw(std::runtime_error("Metropolis: tried to serialize before saving sample data"));

	char sep = ' ';
	std::ostringstream serial;
	
	serial << "Metropolis \n";
	serial << "{\n";
	serial << serialize(sep);
	serial << "}\n\n";

	serial << "Likelihood \n";
	serial << "{\n";
	serial << likelihood->serialize(sep, parameters.names());
	serial << "}\n\n";
	
	serial << "Parameters \n";
	serial << "{\n";
	serial << parameters.serialize(sep);
	serial << "}\n\n";
	
	serial << "OutputOptions \n";
	serial << "{\n";
	serial << posteriorOptions.serialize(sep);
	serial << "}\n\n";

	STMOutput::OutputBuffer buffer (serial.str(), STMOutput::OutputKeyType::resumeData, 
			posteriorOptions);
	outputQueue->push(buffer);	
}


std::string Metropolis::serialize(char sep) const
{
	std::ostringstream result;

	result << "version" << sep << version() << "\n";
	result << "outputBufferSize" << sep << outputBufferSize << "\n";
	result << "thinSize" << sep << thinSize << "\n";
	result << "burnin" << sep << burnin << "\n";
	result << "adaptationSampleSize" << sep << adaptationSampleSize << "\n";
	result << "minAdaptationLoops" << sep << minAdaptationLoops << "\n";
	result << "maxAdaptationLoops" << sep << maxAdaptationLoops << "\n";
	result << "rngSetSeed" << sep << rngSetSeed << "\n";
	result << "rngSeed" << sep << rngSeed << "\n";
	result << "outputLevel" << sep << int(outputLevel) << "\n";
	result << "currentPosteriorProb" << sep << currentPosteriorProb << "\n";
	
	return result.str();
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
				std::cerr << std::endl;
			
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
