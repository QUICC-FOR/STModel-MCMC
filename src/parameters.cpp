/*
STModel-MCMC : parameters.cpp
	
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

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include "../hdr/parameters.hpp"

namespace STMParameters
{

// static variable definition
std::vector<STMParameterNameType> STModelParameters::parNames;
std::map<STMParameterNameType, ParameterSettings> STModelParameters::parSettings;
std::vector<double> STModelParameters::targetAcceptanceInterval {0.27, 0.34};


/*
	IMPLEMENTATION OF PUBLIC FUNCTIONS
*/
STModelParameters::STModelParameters(const std::vector<ParameterSettings> & initPars)
{
	for(const ParameterSettings & par : initPars) {
		if(parSettings.count(par.name) > 0 || 
				std::find(parNames.begin(), parNames.end(), par.name) != parNames.end())
			throw std::runtime_error(
					"STModelParameters: duplicate parameter passed in constructor");
		else {
			parSettings[par.name] = par;
			parNames.push_back(par.name);
		}
	}
	reset();
}


TransitionRates STModelParameters::generate_rates(double env1, double env2, int interval,
		double borealExpected) const
{
	const STMParameterMap & p = parameterValues;	// to shorten the name
	STMParameterValueType alpha_b_logit, alpha_t_logit, beta_b_logit, beta_t_logit, 
			theta_logit, theta_t_logit, epsilon_logit;

	// the following mess is the expression of the linear predictor for each rate in the model	
	alpha_b_logit = p.at("ab0") + p.at("ab1")*env1 + p.at("ab2")*env2 + 
			p.at("ab3")*pow(env1,2) + p.at("ab4")*pow(env2,2) + p.at("ab5")*pow(env1,3) + 
			p.at("ab6")*pow(env2,3);
	  
	alpha_t_logit = p.at("at0") + p.at("at1")*env1 + p.at("at2")*env2 + 
			p.at("at3")*pow(env1,2) + p.at("at4")*pow(env2,2) + p.at("at5")*pow(env1,3) + 
			p.at("at6")*pow(env2,3);

	beta_b_logit = p.at("bb0") + p.at("bb1")*env1 + p.at("bb2")*env2 + 
			p.at("bb3")*pow(env1,2) + p.at("bb4")*pow(env2,2) + p.at("bb5")*pow(env1,3) + 
			p.at("bb6")*pow(env2,3);

	beta_t_logit = p.at("bt0") + p.at("bt1")*env1 + p.at("bt2")*env2 + 
			p.at("bt3")*pow(env1,2) + p.at("bt4")*pow(env2,2) + p.at("bt5")*pow(env1,3) + 
			p.at("bt6")*pow(env2,3);

	theta_logit = p.at("t0") + p.at("t1")*env1 + p.at("t2")*env2 + p.at("t3")*pow(env1,2) 
			+ p.at("t4")*pow(env2,2) + p.at("t5")*pow(env1,3) + p.at("t6")*pow(env2,3);

	theta_t_logit = p.at("tt0") + p.at("tt1")*env1 +  p.at("tt2")*env2 + 
			p.at("tt3")*pow(env1,2) + p.at("tt4")*pow(env2,2) + p.at("tt5")*pow(env1,3) + 
			p.at("tt6")*pow(env2,3);

	epsilon_logit = p.at("e0") + p.at("e1")*env1 + p.at("e2")*env2 + 
			p.at("e3")*pow(env1,2) + p.at("e4")*pow(env2,2) + p.at("e5")*pow(env1,3) + 
			p.at("e6")*pow(env2,3) + p.at("e7")*borealExpected;


	STMParameterValueType alpha_b = make_annual(alpha_b_logit, interval);
	STMParameterValueType alpha_t = make_annual(alpha_t_logit, interval);
	STMParameterValueType beta_b = make_annual(beta_b_logit, interval);
	STMParameterValueType beta_t = make_annual(beta_t_logit, interval);
	STMParameterValueType theta = make_annual(theta_logit, interval);
	STMParameterValueType theta_t = make_annual(theta_t_logit, interval);
	STMParameterValueType epsilon = make_annual(epsilon_logit, interval);
	TransitionRates rates (alpha_b, alpha_t, beta_b, beta_t, theta, theta_t, epsilon);
	return rates;
}


const std::vector<STMParameterNameType> & STModelParameters::names() const
{ return parNames; }

	
void STModelParameters::set_acceptance_rates(const std::map<STMParameterNameType, double> 
		& rates)
{
	for(const auto & p : rates)
		set_acceptance_rate(p.first, p.second);
}


void STModelParameters::set_acceptance_rate(const STMParameterNameType & par, double rate)
{ parSettings.at(par).acceptanceRate = rate; }


int STModelParameters::not_adapted(const STMParameterNameType & par) const
{
	if(parSettings.at(par).acceptanceRate < targetAcceptanceInterval[0])
		return -1;
	else if(parSettings.at(par).acceptanceRate  > targetAcceptanceInterval[1])
		return 1;
	else return 0;
}


bool STModelParameters::adapted() const
{
	int count = 0;
	for(const auto & p : names())
		count += std::abs(not_adapted(p));
	if(count > 0) return true;
	else return false;
}


void STModelParameters::reset()
{
	for(const auto & p : names())
		parameterValues[p] = parSettings[p].initialValue;
	iterationCount = 0;
}


size_t STModelParameters::size() const
{ return parSettings.size(); }


double STModelParameters::sampler_variance(const STMParameterNameType & par) const
{ return parSettings.at(par).variance; }


void STModelParameters::set_sampler_variance(const STMParameterNameType & par, double val)
{ parSettings.at(par).variance = val; }


void STModelParameters::increment(int n)
{ iterationCount += n; }


const STMParameterMap & STModelParameters::current_state() const
{ return parameterValues; }


void STModelParameters::update(const STMParameterPair & par)
{ parameterValues.at(par.first) = par.second; }


STMParameterPair STModelParameters::at(const STMParameterNameType & p) const
{ return STMParameterPair (p, parameterValues.at(p)); }



/*
	IMPLEMENTATION OF PRIVATE FUNCTIONS
*/
STMParameterValueType STModelParameters::make_annual(STMParameterValueType logit_val, 
		int interval) const
{
	// compute the transition rate on the native interval
	// uses the inverse logit, corrected to avoid overflow
	STMParameterValueType val;
	if(logit_val > 0)
		val = 1.0 / (1.0 + std::exp(-logit_val));
	else
		val = std::exp(logit_val) / (1.0 + std::exp(logit_val));		

	// transform to the annual interval
	return 1 - std::pow((1 - val), interval);
}


} // !namespace Parameters