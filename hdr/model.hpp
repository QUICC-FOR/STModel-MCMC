#ifndef STM_MODEL_H
#define STM_MODEL_H

/*
	QUICC-FOR ST-Model MCMC 
	
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
	
	
	Implementation of the model likelihood
	Includes procedures for computing the model likelihood assuming known parameters


*/

#include <map>
#include <string>
#include <sstream>
#include <cmath>
#include <functional>
#include <vector>
#include "stmtypes.hpp"

namespace STMModel
{

inline STM::ParValue deannual(STM::ParValue val, int interval);
inline STM::ParValue inv_logit(STM::ParValue logit_val);


// function type for transition probability function
typedef std::function<STM::ParValue(STM::ParMap &, STM::StateMap &)> TransProbFunction;


class StateException: public std::runtime_error
{
	public:
	explicit StateException(const char* message): std::runtime_error(message) { }
};	


class State
{
	public:
	// the constructor throws a StateException on an invalid state
	State(char s): theState(STM::StateTypes(s)) { self_check(); };
	STM::StateTypes get() const { return theState; };
	static std::vector<char> state_names();
	
	private:
	STM::StateTypes theState;
	void self_check();
	void invalid_state();
};


class STMTransition
{
	public:
	STMTransition(char state1, char state2, double env1, double env2, 
			std::map<char, double> prevalence, int interval);

	STM::ParValue transition_prob(const STM::ParMap & p, int targetInterval);
	void set_global_prevalence();
	static STM::PrevalenceModelTypes get_prevalence_model()	{ return prevalenceModel; }
	static void set_prevalence_model(const STM::PrevalenceModelTypes &pr);

	private:
	static void setup_transition_functions();
	TransProbFunction generate_transition_function();	
	STM::ParMap generate_transform_rates(const STM::ParMap & p) const;
	STM::ParMap generate_interval_rates(const STM::ParMap & p, int targetInterval) const;
	void invalid_transition();
	void compute_stm_prevalence(const STM::ParMap &rates);

	static std::map<STM::StateTypes, std::map<STM::StateTypes, TransProbFunction> > transitionFunctions;
	State initial, final;
	double env1, env2;
	STM::StateMap expected;
	int interval;
	TransProbFunction transProb;
	static STM::PrevalenceModelTypes prevalenceModel;
};



// IMPLEMENTATION

inline STM::ParValue transform_interval(STM::ParValue val, int originalInterval, 
		int targetInterval)
{ return 1 - std::pow((1 - val), (double(originalInterval)/targetInterval)); }


inline STM::ParValue inv_logit(STM::ParValue logit_val)
{
	if(logit_val > 0)
		return 1.0 / (1.0 + std::exp(-logit_val));
	else
		return std::exp(logit_val) / (1.0 + std::exp(logit_val));		
}


inline void State::invalid_state()
{
	std::stringstream msg;
	msg << "invalid state: " << char(theState);
	const std::string m = msg.str();
	throw StateException(m.c_str());
}


inline void STMTransition::invalid_transition()
{
	std::stringstream msg;
	msg << "invalid transition: " << char(initial.get());
	msg << " -> " << char(final.get());
	const std::string m = msg.str();
	throw StateException(m.c_str());
}


inline STM::ParValue STMTransition::transition_prob(const STM::ParMap & p, int targetInterval)
{ 
	STM::ParMap rates = generate_interval_rates(p, targetInterval);
	if(STMTransition::prevalenceModel == STM::PrevalenceModelTypes::STM)
		compute_stm_prevalence(rates);
	return transProb(rates, expected); 
}


inline STMTransition::STMTransition(char state1, char state2, double env1, double env2, 
		std::map<char, double> prevalence, int interval) : 
		initial(state1), final(state2), env1(env1), env2(env2), interval(interval)
{
	if(transitionFunctions.empty())
		setup_transition_functions();
	for(const auto & pr : prevalence)
		expected[State(pr.first).get()] = pr.second;
	if(prevalenceModel == STM::PrevalenceModelTypes::Global)
		set_global_prevalence();
	generate_transition_function(); 
}


inline void STMTransition::set_global_prevalence()
{
	for(auto & exp : expected)
		expected[exp.first] = 1.0;
}


inline STM::ParMap STMTransition::generate_interval_rates(const STM::ParMap & p, 
		int targetInterval) const
{
	STM::ParMap transformLogitParams = generate_transform_rates(p);
	STM::ParMap macroParams;
	for(const auto & par : transformLogitParams)
		macroParams[par.first] = transform_interval(inv_logit(par.second), interval, 
				targetInterval);
	return macroParams;
}


inline TransProbFunction STMTransition::generate_transition_function()
{ 
	try
		{ transProb = STMTransition::transitionFunctions.at(initial.get()).at(final.get()); }
	catch (std::out_of_range e)
		{ invalid_transition(); }
}


} // STMModel namespace
#endif