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
#include "../hdr/parameters.hpp"

namespace STMParameters
{

TransitionRates STModelParameters::generate_rates(double env1, double env2, int interval,
	double borealExpected)
{
	std::vector<double> p;
	std::map<char, double> alpha_logit, beta_logit, theta_logit;
	double epsilon_logit;

	// the following mess is the expression of the linear predictor for each rate in the model	
	p = get_par("alpha", 'B');
	alpha_logit['B'] = p[0] + p[1]*env1 + p[2]*env2 + p[3]*pow(env1,2) + p[4]*pow(env2,2) 
	  + p[5]*pow(env1,3) + p[6]*pow(env2,3);

	p = get_par("alpha", 'T');
	alpha_logit['T'] = p[0] + p[1]*env1 + p[2]*env2 + p[3]*pow(env1,2) + p[4]*pow(env2,2) 
	  + p[5]*pow(env1,3) + p[6]*pow(env2,3);

	p = get_par("beta", 'B');
	beta_logit['B'] = p[0] + p[1]*env1 + p[2]*env2 + p[3]*pow(env1,2) + p[4]*pow(env2,2) +
	  p[5]*pow(env1,3) + p[6]*pow(env2,3);

	p = get_par("beta", 'T');
	beta_logit['T'] = p[0] + p[1]*env1 + p[2]*env2 + p[3]*pow(env1,2) + p[4]*pow(env2,2) +
	  p[5]*pow(env1,3) + p[6]*pow(env2,3);

	p = get_par("theta");
	theta_logit['\0'] = p[0] + p[1]*env1 + p[2]*env2 + p[3]*pow(env1,2) + p[4]*pow(env2,2)
	  + p[5]*pow(env1,3) + p[6]*pow(env2,3);

	p = get_par("theta", 'T');
	theta_logit['T'] = p[0] + p[1]*env1 +  p[2]*env2 + p[3]*pow(env1,2) + p[4]*pow(env2,2)
	  + p[5]*pow(env1,3) + p[6]*pow(env2,3);

	p = get_par("epsilon");
	epsilon_logit = p[0] + p[1]*env1 + p[2]*env2 + p[3]*pow(env1,2) + p[4]*pow(env2,2) + 
	  p[5]*pow(env1,3) + p[6]*pow(env2,3) + p[7]*borealExpected;


	std::map<char, double> alpha = make_annual(alpha_logit, interval);
	std::map<char, double> beta = make_annual(beta_logit, interval);
	std::map<char, double> theta = make_annual(theta_logit, interval);
	double epsilon = make_annual(epsilon_logit, interval);
	TransitionRates rates (alpha, beta, theta, epsilon);
	return rates;
}


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


std::vector<double> STModelParameters::get_par(std::string parname, char state)
{
	int start = -1;
	int length = 7;

	if(parname == "alpha") {
		if(state == 'B')
			start = 0;
		else if(state == 'T')
			start = length;
	}
	else if(parname == "beta") {
		if(state == 'B')
			start = 2 * length;
		else if(state == 'T')
			start = 3*length;
	}
	else if(parname == "theta") {
		if(state == '\0')
			start = 4*length;
		else if(state == 'T')
			start = 5*length;
	}
	else if(parname == "epsilon") {
		start = 6*length;
		length++;
	}
	
	std::vector<double> result = std::vector<double>(parameters.begin() + start,
		parameters.begin() + start + length);
	
	return result;
}


size_t STModelParameters::size() const
{ return parameters.size(); }

double & STModelParameters::at(int i)
{ return parameters.at(i); }

const std::vector<std::string> & STModelParameters::names() const
{ return parameterNames; }

STModelParameters::STModelParameters(std::vector<double> pars) : parameters(pars)
{
	// set up the names;
	// this is hard-coded (bad!), but it is fundamental to the definition of the model
	// they are identical to those used in the R code for sim annealing
	parameterNames.push_back("ab0");
	parameterNames.push_back("ab1");
	parameterNames.push_back("ab2");
	parameterNames.push_back("ab3");
	parameterNames.push_back("ab4");
	parameterNames.push_back("ab5");
	parameterNames.push_back("ab6");

	parameterNames.push_back("at0");
	parameterNames.push_back("at1");
	parameterNames.push_back("at2");
	parameterNames.push_back("at3");
	parameterNames.push_back("at4");
	parameterNames.push_back("at5");
	parameterNames.push_back("at6");

	parameterNames.push_back("bb0");
	parameterNames.push_back("bb1");
	parameterNames.push_back("bb2");
	parameterNames.push_back("bb3");
	parameterNames.push_back("bb4");
	parameterNames.push_back("bb5");
	parameterNames.push_back("bb6");

	parameterNames.push_back("bt0");
	parameterNames.push_back("bt1");
	parameterNames.push_back("bt2");
	parameterNames.push_back("bt3");
	parameterNames.push_back("bt4");
	parameterNames.push_back("bt5");
	parameterNames.push_back("bt6");

	parameterNames.push_back("t0");
	parameterNames.push_back("t1");
	parameterNames.push_back("t2");
	parameterNames.push_back("t3");
	parameterNames.push_back("t4");
	parameterNames.push_back("t5");
	parameterNames.push_back("t6");

	parameterNames.push_back("tt0");
	parameterNames.push_back("tt1");
	parameterNames.push_back("tt2");
	parameterNames.push_back("tt3");
	parameterNames.push_back("tt4");
	parameterNames.push_back("tt5");
	parameterNames.push_back("tt6");

	parameterNames.push_back("e0");
	parameterNames.push_back("e1");
	parameterNames.push_back("e2");
	parameterNames.push_back("e3");
	parameterNames.push_back("e4");
	parameterNames.push_back("e5");
	parameterNames.push_back("e6");
	parameterNames.push_back("e7");
}

} // !namespace Parameters