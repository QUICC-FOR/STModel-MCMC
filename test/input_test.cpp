#include "hdr/parameters.hpp"
#include "hdr/likelihood.hpp"
#include "hdr/input.hpp"
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

	std::cerr << "Initial values:\n";
	for(auto par : inits)
		std::cerr << par.name << ": " << par.initialValue << "\n";
	
	struct Transition {
	char initial, final;
	double env1, env2;
	std::map<char, double> expected;
	int interval;
};

	std::cerr << "\nTransitions:\n";
	for(auto tr : transitionData) {
		std::cerr << tr.initial << " -> " << tr.final << " : " << tr.interval << " yr : " << tr.env1 << " " << tr.env2 << "\n";
		for(auto ex : tr.expected)
			std::cerr << "    exp" << ex.first << ": " << ex.second << "\n";
	}
	
	std::cerr << "\nPriors\n";
	for(auto pr : priors)
		std::cerr << pr.first << ": " << pr.second.mean << ", " << pr.second.sd << "\n";
}