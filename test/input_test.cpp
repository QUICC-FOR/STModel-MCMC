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

	std::cerr << "1\n";
	STMInput::STMInputHelper inp (parFileName, transFileName);
// 	std::cerr << "2\n";
// 	inits = inp.parameter_inits();
// 	std::cerr << "3\n";
// 	priors = inp.priors();
// 	std::cerr << "4\n";
// 	transitionData = inp.transitions();
// 	std::cerr << "5\n";

}