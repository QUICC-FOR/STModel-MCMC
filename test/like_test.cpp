#include "../hdr/parameters.hpp"
#include "../hdr/likelihood.hpp"
#include "../hdr/input.hpp"
#include <iostream>


int main(void)
{
	const char * parFileName = "inp/inits.txt";
	const char * transFileName = "inp/trans.txt";

	std::vector<STMParameters::ParameterSettings> inits;
	std::vector<STMLikelihood::Transition> transitionData;
	std::map<std::string, STMLikelihood::PriorDist> priors;

	STMInput::STMInputHelper inp (parFileName, transFileName);
	inits = inp.parameter_inits();
	priors = inp.priors();
	transitionData = inp.transitions();

	STMParameters::STModelParameters params (inits);
	STMLikelihood::Likelihood * likelihood = new STMLikelihood::Likelihood 
			(transitionData, priors);

	// print out the prior prob
	// with a minimally informative prior, these should be basically identical
	for(auto pn : params.names())
		std::cerr << pn << ": " << likelihood->log_prior(params.at(pn)) << "\n";

	std::cerr << "\nLog likelihood: " << likelihood->compute_log_likelihood(params) << "\n";


}