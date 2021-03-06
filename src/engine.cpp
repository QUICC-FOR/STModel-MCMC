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
#include <random>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_fit.h>

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

	std::string engineVersion = "Metropolis1.5";
	
	
	std::pair<double, int> weighted_mean(const std::vector<std::pair<double, int> > &x)
	{
		long double sum = 0;
		std::pair<double, int> result (0,0);
		for(const auto & item : x)
		{
			sum += item.first * item.second;
			result.second += item.second;
		}
		result.first = sum / result.second;
		return result;
	}
	
	std::pair<STM::ParMap, int> weighted_mean(const std::pair<STM::ParMap, int> &x, 
			const std::pair<STM::ParMap, int> &y)
	{
		{	// error checking block
			// will throw an exception if x has a key in it that is not in y or vice-versa
			std::string msg = "";
			for(const auto & p : x.first)
			{
				if(not y.first.count(p.first))
					msg = msg + "Metropolis::weighted_mean: first input has key " + p.first + " which is missing in second\n";
			}
			for(const auto & p : y.first)
			{
				if(not x.first.count(p.first))
					msg = msg + "Metropolis::weighted_mean: second input has key " + p.first + " which is missing in first\n";
			}
			if(msg != "")			
				throw std::runtime_error(msg);
		}
		
		std::pair<STM::ParMap, int> result;
		result.second = x.second + y.second;			
		for(const auto & p : x.first)
		{
			result.first[p.first] = ((x.first.at(p.first) * x.second) + 
					(y.first.at(p.first) * y.second)) / result.second;
		}
		return result;
	}
}

namespace STMEngine {


/*
	Implementation of public functions
*/

Metropolis::Metropolis(const std::vector<STMParameters::ParameterSettings> & inits, 
		STMOutput::OutputQueue * const queue, STMLikelihood::Likelihood * const lhood,
		EngineOutputLevel outLevel, STMOutput::OutputOptions outOpt, int thin, int burnin, 
		bool doDIC, bool rngSetSeed, int rngSeed) :
// objects that are not owned by the object
outputQueue(queue), likelihood(lhood),

// objects that we own or share
parameters(inits), rngSetSeed(rngSetSeed), rngSeed(rngSeed), burnin(burnin),
rng(gsl_rng_alloc(gsl_rng_mt19937), gsl_rng_free), outputLevel(outLevel), thinSize(thin),
posteriorOptions(outOpt), computeDIC(doDIC),

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
		
	// compute the log likelihood for the initial conditions
	currentLL = likelihood->compute_log_likelihood(parameters);
		
	// initialize thetaBar with parameter names
	thetaBar.second = 0;
	for(auto p : parameters.names())
		thetaBar.first[p] = 0;
	
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
	rngSeed = STMInput::str_convert<unsigned long int>(esd.at("rngSeed")[0]);
	rngSetSeed = STMInput::str_convert<bool>(esd.at("rngSetSeed")[0]);
	outputLevel = EngineOutputLevel(STMInput::str_convert<int>(esd.at("outputLevel")[0]));
	currentLL = STMInput::str_convert<double>(esd.at("currentLL")[0]);
	computeDIC = STMInput::str_convert<bool>(esd.at("computeDIC")[0]);
	DBar = std::pair<double, int>(STMInput::str_convert<double>(esd.at("DBar")[0]), 
			STMInput::str_convert<int>(esd.at("DBar")[1]));
	thetaBar.second = STMInput::str_convert<int>(esd.at("thetaBar_sampSize")[0]);
	for(const auto & p : parameters.names())
	{
		std::string pkey = "thetaBar_" + p;
		thetaBar.first[p] = STMInput::str_convert<double>(esd.at(pkey)[0]);
	}

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
	// for safety, always start by re-computing the current likelihood
	currentLL = likelihood->compute_log_likelihood(parameters);
	bool computeDevianceNow = false;
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
			computeDevianceNow = computeDIC;
		}
		currentSamples.reserve(sampleSize);
		if(computeDevianceNow)
			sampleDeviance.reserve(sampleSize);
		do_sample(sampleSize, computeDevianceNow);
		
		if(burninCompleted < burnin)
		{
			burninCompleted += sampleSize;		
		}
		else
		{
			if(computeDIC)
				prepare_deviance(); // this function takes care of clearing the old vector

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
	// end of sampling; compute DIC and output if needed
	if(computeDIC)
	{
		STMParameters::STModelParameters tbarPars (parameters);
		for(const auto & p : thetaBar.first)
			tbarPars.update(p);
		double devThetaBar = -2.0 * likelihood->compute_log_likelihood(tbarPars);

		double pd = DBar.first - devThetaBar;
		double DIC = devThetaBar + 2.0 * pd;
		
		// now just save it all
		std::ostringstream dicOutput;
		dicOutput << "pD: " << pd << "\n";
		dicOutput << "Mean deviance (d-bar): " << DBar.first << "\n";
		dicOutput << "Deviance of mean (d(theta-bar)): " << devThetaBar << "\n";
		dicOutput << "DIC: " << DIC << "\n";
		STMOutput::OutputBuffer buffer (dicOutput.str(), STMOutput::OutputKeyType::dic, 
			posteriorOptions);
		outputQueue->push(buffer);	
	}
	if(saveResumeData)
		serialize_all();

}




/*
	Implementation of private functions: here there be dragons
*/


void Metropolis::regression_adapt(int numSteps, int stepSize)
{
	std::vector<STM::ParName> parNames (parameters.names());
	
	std::map<STM::ParName, std::map<std::string, double *> > regressionData;
	for(const auto & par : parNames)
	{
		regressionData[par]["log_variance"] = new double [numSteps];
		regressionData[par]["variance"] = new double [numSteps];
		regressionData[par]["acceptance"] = new double [numSteps];
	}
	
	for(int i = 0; i < numSteps; i++)
	{
		// compute acceptance rates for the current variance term
		parameters.set_acceptance_rates(do_sample(stepSize));
	
		for(const auto & par : parNames)
		{
			// save regression data for each parameter
			regressionData[par]["log_variance"][i] = std::log(parameters.sampler_variance(par));
			regressionData[par]["variance"][i] = parameters.sampler_variance(par);
			regressionData[par]["acceptance"][i] = parameters.acceptance_rate(par);
			
			// choose new variances at random for each parameter; drawn from a gamma with mean 2.38 and sd 2
			parameters.set_sampler_variance(par, gsl_ran_gamma(rng.get(), 1.4161, 1.680672));
		}
	
	}
	
	// perform regression for each parameter and clean up
	for(const auto & par : parNames)
	{
		// first compute the correlation for variance and log_variance, use whichever is higher
		double corVar = gsl_stats_correlation(regressionData[par]["variance"], 1,
				regressionData[par]["acceptance"], 1, numSteps);
		double corLogVar = gsl_stats_correlation(regressionData[par]["log_variance"], 1,
				regressionData[par]["acceptance"], 1, numSteps);

		double beta0, beta1, cov00, cov01, cov11, sumsq, targetVariance;
		if(corVar >= corLogVar)
		{
			gsl_fit_linear(regressionData[par]["variance"], 1, 
					regressionData[par]["acceptance"], 1, numSteps, &beta0, &beta1, 
					&cov00, &cov01, &cov11, &sumsq);
			targetVariance = (parameters.optimal_acceptance_rate() - beta0)/beta1;
		} else
		{
			gsl_fit_linear(regressionData[par]["log_variance"], 1, 
					regressionData[par]["acceptance"], 1, numSteps, &beta0, &beta1,
					&cov00, &cov01, &cov11, &sumsq);
			targetVariance = std::exp((parameters.optimal_acceptance_rate() - beta0)/beta1);
		}
		parameters.set_sampler_variance(par, targetVariance);

		delete [] regressionData[par]["log_variance"];
		delete [] regressionData[par]["variance"];
		delete [] regressionData[par]["acceptance"];
	}
}


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
	
	regression_adapt(10, 100); // use the first two loops to try a regression approach	
	int nLoops = 2;
	
	while(nLoops < minAdaptationLoops or ((not parameters.adapted()) and nLoops < maxAdaptationLoops))	
	{
		nLoops++;
		parameters.set_acceptance_rates(do_sample(adaptationSampleSize));

		for(const auto & par : parNames) {
			double ratio;
			if(parameters.acceptance_rate(par) == 0) 
				ratio = 1e-2;
			else
				ratio = parameters.acceptance_rate(par) / parameters.optimal_acceptance_rate();
			parameters.set_sampler_variance(par, ratio*parameters.sampler_variance(par));
		}
		
		if(outputLevel >= EngineOutputLevel::Talkative) {
			std::cerr << "\n    " << timestamp() << " iter " << parameters.iteration() << "\n";
			parameters.print_adaptation(isatty(fileno(stderr)), 2);
// 			std::cerr << "    " << parameters.str_acceptance_rates(isatty(fileno(stderr))) << "\n";
// 			std::cerr << "    sampler variance:\n";
// 			std::cerr << "    " << parameters.str_sampling_variance(isatty(fileno(stderr))) << std::endl;
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
	result << "currentLL" << sep << currentLL << "\n";
	result << "computeDIC" << sep << computeDIC << "\n";
	result << "DBar" << sep << DBar.first << sep << DBar.second << "\n";
	result << "thetaBar_sampSize" << sep << thetaBar.second << "\n";
	for(const auto & theta : thetaBar.first)
		result << "thetaBar_" << theta.first << sep << theta.second << "\n";
	
	return result.str();
}



std::map<STM::ParName, double> Metropolis::do_sample(int n, bool saveDeviance)
// n is the number of samples to take
// returns a map of acceptance rates keyed by parameter name
{
	// 	shuffle the order of parameters
	std::vector<STM::ParName> parNames (parameters.active_names());
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
		if(saveDeviance)
			sampleDeviance.push_back(std::pair<double, int>(-2 * currentLL, 1));

		//		if desired, some debugging output
		if(outputLevel >= EngineOutputLevel::Verbose) {
			std::cerr << "  iteration " << parameters.iteration() - 1 << 
					"    posterior probability: " << currentPosteriorProb <<
					"    likelihood: " << currentLL << "\n";
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
	double proposalLL = likelihood->compute_log_likelihood(proposal);
	
	double proposalLogPosterior = log_posterior_prob(proposalLL, p);
	double currentLogPosterior = log_posterior_prob(currentLL, parameters.at(p.first));
	double acceptanceProb = exp(proposalLogPosterior - currentLogPosterior);

	// 	check for nan -- right now this is not being handled, but it should be
	if(std::isnan(acceptanceProb))
		acceptanceProb = 0;

	double testVal = gsl_rng_uniform(rng.get());
	if(testVal < acceptanceProb) {
		currentPosteriorProb = proposalLogPosterior;
		currentLL = proposalLL;
		parameters.update(p);
		return 1;
	} else {
		return 0;
	}
}


void Metropolis::prepare_deviance()
{
	sampleDeviance.push_back(DBar);
	DBar = weighted_mean(sampleDeviance);
	sampleDeviance.clear();
	
	
	// compute the mean of the current parameter samples
	std::pair<STM::ParMap, int> parMeans;
	for(const auto & p : parameters.names())
		parMeans.first[p] = parMeans.second = 0;
		
	for(const auto sample : currentSamples)
	{
		for(const auto & p : parameters.names())
			parMeans.first.at(p) += sample.at(p);
	}
	parMeans.second = currentSamples.size();
	for(const auto & p : parameters.names())
		parMeans.first.at(p) /= parMeans.second;
		
	thetaBar = weighted_mean(thetaBar, parMeans);
}


double Metropolis::log_posterior_prob(const double logl, const STM::ParPair & pair) const
{ return logl + likelihood->log_prior(pair); }

void Metropolis::set_up_rng()
{
	if(not rngSetSeed)
	{
		std::random_device rd;
		rngSeed = rd();
	}
	gsl_rng_set(rng.get(), rngSeed);
}
} // namespace
