#include "../../hdr/model.hpp"

namespace STM
{
	enum class StateTypes: char
	{
		T = 'T',
		B = 'B',
		M = 'M',
		R = 'R'
	};
}

namespace STMModel
{

std::vector<char> State::state_names()
{
	std::vector<char> result = {'T','B', 'M', 'R'};
	return result;
}


std::map<STM::StateTypes, std::map<STM::StateTypes, TransProbFunction> > STMTransition::transitionFunctions;

/*
	This function encodes the four state model
	It stores function generators in the transitionFunctions object
	This object is a 2-D map, so that transitionFunctions[STM::StateTypes::A][STM::StateTypes::B]
	contains a function generator
	
	when called (with a parameter e giving the expected values, i.e., prevalence) from a 
	transition object, it returns a function taking a parameter set p as a parameter
	
	this function returns the transition probability from STM::StateTypes::A to STM::StateTypes::B
	given the parameters and the prevalence
*/
void STMTransition::setup_transition_functions()
{
	// shorten the names for this function
	using S = STM::StateTypes;
	std::map<STM::StateTypes, std::map<STM::StateTypes, TransProbFunction> > &tf = 
			STMTransition::transitionFunctions;

	// T -> R, B -> R, M -> R
	tf[S::T][S::R] = tf[S::B][S::R] = tf[S::M][S::R] = [](STM::ParMap &p, STM::StateMap &e) 
	{ return p["epsilon"]; };
	
	// T -> M
	tf[S::T][S::M] = [&tf](STM::ParMap &p, STM::StateMap &e) 
		{ return p["beta_b"] * (e.at(S::B) + e.at(S::M)) * (1.0 - tf[S::T][S::R](p, e)); };
	
	// T -> T
	tf[S::T][S::T] = [&tf](STM::ParMap &p, STM::StateMap &e) 
		{ return 1.0 - tf[S::T][S::R](p, e) - tf[S::T][S::M](p, e); }; 

	// B -> M
	tf[S::B][S::M] = [&tf](STM::ParMap &p, STM::StateMap &e) 
		{  return p["beta_t"] * (e.at(S::T) + e.at(S::M)) * (1.0 - tf[S::B][S::R](p, e)); };

	// B -> B
	tf[S::B][S::B] = [&tf](STM::ParMap &p, STM::StateMap &e) 
		{ return 1.0 - tf[S::B][S::R](p, e) - tf[S::B][S::M](p, e); }; 

	// M -> T
	tf[S::M][S::T] = [&tf](STM::ParMap &p, STM::StateMap &e) 
		{ return p["theta"] * p["theta_t"] * (1.0 - tf[S::M][S::R](p, e)); }; 

	// M -> B
	tf[S::M][S::B] = [&tf](STM::ParMap &p, STM::StateMap &e) 
		{ return p["theta"] * (1 - p["theta_t"]) * (1.0 - tf[S::M][S::R](p, e)); }; 

	// M -> M
	tf[S::M][S::M] = [&tf](STM::ParMap &p, STM::StateMap &e) 
		{ return 1.0 - tf[S::M][S::T](p, e) - tf[S::M][S::B](p, e) - tf[S::M][S::R](p, e); };

	// R -> T
	tf[S::R][S::T] = [&tf](STM::ParMap &p, STM::StateMap &e) 
		{ return p["alpha_t"] * (e.at(S::M) + e.at(S::T)) *  
				(1 - p["alpha_b"]*(e.at(S::B)+e.at(S::M))); 
		};

	// R -> B
	tf[S::R][S::B] = [](STM::ParMap &p, STM::StateMap &e) 
		{ return p["alpha_b"] * (e.at(S::M) + e.at(S::B)) * 
				(1 - p["alpha_t"]*(e.at(S::T)+e.at(S::M)));
		};

	// R -> M
	tf[S::R][S::M] = [](STM::ParMap &p, STM::StateMap &e) 
		{ return p["alpha_b"] * (e.at(S::M) + e.at(S::B)) * 
				(p["alpha_t"] * (e.at(S::M) + e.at(S::T)));
		};

	// R -> R
	tf[S::R][S::R] = [&tf](STM::ParMap &p, STM::StateMap &e) 
		{  return 1.0 - tf[S::R][S::T](p, e) - tf[S::R][S::B](p, e) - 
				tf[S::R][S::M](p, e);
		};
}



void State::self_check()
{
	using S = STM::StateTypes;
	if(theState != S::T and theState != S::B and theState != S::M and theState != S::R)
		invalid_state();
}




STM::ParMap STMTransition::generate_annual_rates(const STM::ParMap & p) const
{
	STM::ParMap annualLogitParams;
	annualLogitParams["alpha_b"] = p.at("ab0") + p.at("ab1")*env1 + p.at("ab2")*env2 + 
			p.at("ab3")*pow(env1,2) + p.at("ab4")*pow(env2,2) + p.at("ab5")*pow(env1,3) + 
			p.at("ab6")*pow(env2,3);
			
	annualLogitParams["alpha_t"] = p.at("at0") + p.at("at1")*env1 + p.at("at2")*env2 + 
			p.at("at3")*pow(env1,2) + p.at("at4")*pow(env2,2) + p.at("at5")*pow(env1,3) + 
			p.at("at6")*pow(env2,3);

	annualLogitParams["beta_b"] = p.at("bb0") + p.at("bb1")*env1 + p.at("bb2")*env2 + 
			p.at("bb3")*pow(env1,2) + p.at("bb4")*pow(env2,2) + p.at("bb5")*pow(env1,3) + 
			p.at("bb6")*pow(env2,3);

	annualLogitParams["beta_t"] = p.at("bt0") + p.at("bt1")*env1 + p.at("bt2")*env2 + 
			p.at("bt3")*pow(env1,2) + p.at("bt4")*pow(env2,2) + p.at("bt5")*pow(env1,3) + 
			p.at("bt6")*pow(env2,3);

	annualLogitParams["theta"] = p.at("t0") + p.at("t1")*env1 + p.at("t2")*env2 + 
			p.at("t3")*pow(env1,2) + p.at("t4")*pow(env2,2) + p.at("t5")*pow(env1,3) + 
			p.at("t6")*pow(env2,3);

	annualLogitParams["theta_t"] = p.at("tt0") + p.at("tt1")*env1 +  p.at("tt2")*env2 + 
			p.at("tt3")*pow(env1,2) + p.at("tt4")*pow(env2,2) + p.at("tt5")*pow(env1,3) + 
			p.at("tt6")*pow(env2,3);

	annualLogitParams["epsilon"] = p.at("e0") + p.at("e1")*env1 + p.at("e2")*env2 + 
			p.at("e3")*pow(env1,2) + p.at("e4")*pow(env2,2) + p.at("e5")*pow(env1,3) + 
			p.at("e6")*pow(env2,3);
	
	return annualLogitParams;
}

}