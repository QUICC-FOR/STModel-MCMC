/*
STModel-MCMC : model_2s.cpp
	
	  Copyright 2014 Matthew V Talluto, Isabelle Boulangeat, Dominique Gravel
	
	  This program is free software; you can redistribute it and/or modify
	  it under the terms of the GNU General Public License as published by
	  the Free Software Foundation; either version 3 of the License, or (at
	  your option) any later version.
	  
	  This program is distributed in the hope that it will be useful, but
	  WITHOUT ANY WARRANTY; without even the implied warranty of
	  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	  General Public License for more details.
	
	  You should have received a copy of the GNU General Public License
	  along with this program; if not, write to the Free Software
	  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
	

*/

#include "../../hdr/model.hpp"


namespace STM
{
	enum class StateTypes: char
	{
		Absent = '0',
		Present = '1'
	};
}

namespace STMModel
{

std::vector<char> State::state_names()
{
	std::vector<char> result = {'0','1'};
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

	// Colonizations
	tf[S::Absent][S::Present] = [](STM::ParMap p, STM::StateMap e) 
	{ return p["gamma"] * e[S::Present]; };

	// Absences
	tf[S::Absent][S::Absent] = [&tf](STM::ParMap p, STM::StateMap e) 
	{ return 1.0 - tf[S::Absent][S::Present](p,e); };
	
	// Extinctions
	tf[S::Present][S::Absent] = [](STM::ParMap p, STM::StateMap e) 
	{ return p["epsilon"]; };

	// Presences
	tf[S::Present][S::Present] = [&tf](STM::ParMap p, STM::StateMap e) 
	{ return 1.0 - tf[S::Present][S::Absent](p,e); };
}



void State::self_check()
{
	using S = STM::StateTypes;
	if(theState != S::Absent and theState != S::Present)
		invalid_state();
}


STM::ParMap STMTransition::generate_annual_rates(const STM::ParMap & p) const
{
	STM::ParMap annualLogitParams;
	annualLogitParams["gamma"] = p.at("g0") + p.at("g1")*env1 + p.at("g2")*env2 + 
			p.at("g3")*pow(env1,2) + p.at("g4")*pow(env2,2);
	annualLogitParams["epsilon"] = p.at("e0") + p.at("e1")*env1 + p.at("e2")*env2 + 
			p.at("e3")*pow(env1,2) + p.at("e4")*pow(env2,2);
	
	if(useCubic)
	{
		annualLogitParams["gamma"] += p.at("g5")*pow(env1,3) + p.at("g6")*pow(env2,3);
		annualLogitParams["epsilon"] += p.at("e5")*pow(env1,3) + p.at("e6")*pow(env2,3);
	}
	return annualLogitParams;
}






} // !namespace STMModel