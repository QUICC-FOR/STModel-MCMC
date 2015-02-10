#include "../hdr/parameters.hpp"
#include "../hdr/input.hpp"
#include <iostream>


void check_tr_rates(STMParameters::STModelParameters p, double e1, double e2, double in)
{
	STMParameters::TransitionRates trRates = p.generate_rates(e1,e2,in);
	std::cerr << "checking transition rates with env1 = " << e1 << ", env2 = " << e2 << ", and interval = " << in << "\n";
	std::cerr << "alpha_b " << trRates.alpha_b << "\n";
	std::cerr << "alpha_t " << trRates.alpha_t << "\n";
	std::cerr << "beta_b " << trRates.beta_b << "\n";
	std::cerr << "beta_t " << trRates.beta_t << "\n";
	std::cerr << "theta " << trRates.theta << "\n";
	std::cerr << "theta_t " << trRates.theta_t << "\n";
	std::cerr << "epsilon " << trRates.epsilon << "\n";
	std::cerr << "\n";

}

int main(void)
{
	const char * parFileName = "inp/inits.txt";
	const char * transFileName = "inp/trans.txt";

	std::vector<STMParameters::ParameterSettings> inits;

	STMInput::STMInputHelper inp (parFileName, transFileName);
	inits = inp.parameter_inits();


	STMParameters::STModelParameters params (inits);
	check_tr_rates(params, 0,0,10);
	check_tr_rates(params, 1,0,10);
	check_tr_rates(params, 0.2,0.6,10);
	
	STMParameters::STMParameterMap cs = params.current_state();
	for(auto v : cs)
		std::cerr << v.first << ": " << v.second << "\n";
	std::cerr << "\n";
	params.update(STMParameters::STMParameterPair("tt0", 1111.573));
	std::cerr << "tt0: " << params.at("tt0").second << "\n\n";
	
}

