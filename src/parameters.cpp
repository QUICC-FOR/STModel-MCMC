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
#include "../hdr/parameters.hpp"

namespace STMParameters
{

/*
	IMPLEMENTATION OF PUBLIC FUNCTIONS
*/
STModelParameters::STModelParameters(const std::vector<Parameter> & initPars) : 
targetAcceptanceInterval {0.27, 0.34}
{
	for(const Parameter & par : initPars) {
		if(parameters.count(par.name) > 0)
			throw std::runtime_error("STModelParameters: duplicate parameter passed in constructor");
		else
			parameters[par.name] = par;
	}
}


TransitionRates STModelParameters::generate_rates(double env1, double env2, int interval,
	double borealExpected) const
{
	const std::map<std::string, Parameter> & p = parameters;		// just to shorten the name for readability
	double alpha_b_logit, alpha_t_logit, beta_b_logit, beta_t_logit, theta_logit, 
			theta_t_logit, epsilon_logit;

	// the following mess is the expression of the linear predictor for each rate in the model	
	alpha_b_logit = p.at("ab0").value + p.at("ab1").value*env1 + p.at("ab2").value*env2 + 
			p.at("ab3").value*pow(env1,2) + p.at("ab4").value*pow(env2,2) + 
			p.at("ab5").value*pow(env1,3) + p.at("ab6").value*pow(env2,3);
	  
	alpha_t_logit = p.at("at0").value + p.at("at1").value*env1 + p.at("at2").value*env2 + 
			p.at("at3").value*pow(env1,2) + p.at("at4").value*pow(env2,2) + 
			p.at("at5").value*pow(env1,3) + p.at("at6").value*pow(env2,3);

	beta_b_logit = p.at("bb0").value + p.at("bb1").value*env1 + p.at("bb2").value*env2 + 
			p.at("bb3").value*pow(env1,2) + p.at("bb4").value*pow(env2,2) + 
			p.at("bb5").value*pow(env1,3) + p.at("bb6").value*pow(env2,3);

	beta_t_logit = p.at("bt0").value + p.at("bt1").value*env1 + p.at("bt2").value*env2 + 
			p.at("bt3").value*pow(env1,2) + p.at("bt4").value*pow(env2,2) + 
			p.at("bt5").value*pow(env1,3) + p.at("bt6").value*pow(env2,3);

	theta_logit = p.at("t0").value + p.at("t1").value*env1 + p.at("t2").value*env2 + 
			p.at("t3").value*pow(env1,2) + p.at("t4").value*pow(env2,2) + 
			p.at("t5").value*pow(env1,3) + p.at("t6").value*pow(env2,3);

	theta_t_logit = p.at("tt0").value + p.at("tt1").value*env1 +  p.at("tt2").value*env2 + 
			p.at("tt3").value*pow(env1,2) + p.at("tt4").value*pow(env2,2) + 
			p.at("tt5").value*pow(env1,3) + p.at("tt6").value*pow(env2,3);

	epsilon_logit = p.at("e0").value + p.at("e1").value*env1 + p.at("e2").value*env2 + 
			p.at("e3").value*pow(env1,2) + p.at("e4").value*pow(env2,2) + 
			p.at("e5").value*pow(env1,3) + p.at("e6").value*pow(env2,3) + 
			p.at("e7").value*borealExpected;


	double alpha_b = make_annual(alpha_b_logit, interval);
	double alpha_t = make_annual(alpha_t_logit, interval);
	double beta_b = make_annual(beta_b_logit, interval);
	double beta_t = make_annual(beta_t_logit, interval);
	double theta = make_annual(theta_logit, interval);
	double theta_t = make_annual(theta_t_logit, interval);
	double epsilon = make_annual(epsilon_logit, interval);
	TransitionRates rates (alpha_b, alpha_t, beta_b, beta_t, theta, theta_t, epsilon);
	return rates;
}


const std::vector<std::string> & STModelParameters::names()
{
	if(parNames.size() != size()) {
		for(const auto & p : parameters)
			parNames.push_back(p.first);
	}
	return parNames;
}

void STModelParameters::set_acceptance_rate(std::string par, double rate)
{ parameters.at(par).acceptanceRate = rate; }


int STModelParameters::not_adapted(std::string par) const
{
	if(parameters.at(par).acceptanceRate < targetAcceptanceInterval[0])
		return -1;
	else if(parameters.at(par).acceptanceRate  > targetAcceptanceInterval[1])
		return 1;
	else return 0;
}


bool STModelParameters::adapted() const
{
	int count = 0;
	for(const auto & p : parameters)
		count += std::abs(not_adapted(p.first));
	if(count > 0) return true;
	else return false;
}


void STModelParameters::reset()
{
	// TEST - make sure this actually has the effect it is supposed to
	for(auto & p : parameters)
		p.second.value = p.second.initialValue;
	iterationCount = 0;
}


size_t STModelParameters::size() const
{ return parameters.size(); }


const double & STModelParameters::sampler_variance(std::string par) const
{ return parameters.at(par).variance; }


void STModelParameters::set_sampler_variance(std::string par, double val)
{ parameters.at(par).variance = val; }


void STModelParameters::increment(int n)
{ iterationCount += n; }


std::map<std::string, double> STModelParameters::current_state() const
{
	std::map<std::string, double> pars;
	for(const auto & p : parameters)
		pars[p.first] = p.second.value;
	return pars; 
}


void STModelParameters::update(std::string par, double val)
{ parameters.at(par).value = val; }



/*
	IMPLEMENTATION OF PRIVATE FUNCTIONS
*/

std::map<char, double> STModelParameters::make_annual(const std::map<char, double> val, 
  int interval) const
{
	std::map<char, double> result;
	for(auto it = val.begin(); it != val.end(); it++)
		result[it->first] = make_annual(it->second, interval);
		
	return result;
}


double STModelParameters::make_annual(const double logit_val, int interval) const
{
	// compute the transition rate on the native interval
	// uses the inverse logit, corrected to avoid overflow
	double val;
	if(logit_val > 0)
		val = 1.0 / (1.0 + std::exp(-logit_val));
	else
		val = std::exp(logit_val) / (1.0 + std::exp(logit_val));		

	// transform to the annual interval
	return 1 - std::pow((1 - val), interval);
}


} // !namespace Parameters