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
#include <sstream>

#include "../hdr/parameters.hpp"

namespace STMParameters
{

// static variable definition
std::vector<STM::ParName> STModelParameters::parNames;
std::map<STM::ParName, ParameterSettings> STModelParameters::parSettings;
std::vector<double> STModelParameters::targetAcceptanceInterval {0.27, 0.34};


/*
	IMPLEMENTATION OF PUBLIC FUNCTIONS
*/
STModelParameters::STModelParameters(const std::vector<ParameterSettings> & initPars)
{
	for(const ParameterSettings & par : initPars) {

		if(parSettings.count(par.name) == 0)
			parSettings[par.name] = par;

		if(std::find(parNames.begin(), parNames.end(), par.name) == parNames.end())
			parNames.push_back(par.name);
	}
	reset();
}


const std::vector<STM::ParName> & STModelParameters::names() const
{ return parNames; }

	
void STModelParameters::set_acceptance_rates(const std::map<STM::ParName, double> 
		& rates)
{
	for(const auto & p : rates)
		set_acceptance_rate(p.first, p.second);
}


void STModelParameters::set_acceptance_rate(const STM::ParName & par, double rate)
{ parSettings.at(par).acceptanceRate = rate; }


std::string STModelParameters::serialize(char s) const
{
	std::ostringstream result;
	std::vector<std::string> pNames = names();

	result << "parNames";
	for(const auto & v : pNames) result << s << v;
	result << "\n";
	
	STM::ParMap initialVals, pVariance, pAcceptance;
	for(const auto & pn : pNames)
	{
		const ParameterSettings & ps = parSettings[pn];
		initialVals[pn] = ps.initialValue;
		pVariance[pn] = ps.variance;
		pAcceptance[pn] = ps.acceptanceRate;
	}
	
	result << "initialVals";
	for(const auto & pn : pNames) result << s << initialVals[pn];
	result << "\nsamplerVariance";
	for(const auto & pn : pNames) result << s << pVariance[pn];
	result << "\nacceptanceRates";
	for(const auto & pn : pNames) result << s << pAcceptance[pn];

	result << "\ntargetAcceptanceInterval";
	for(const auto & v : targetAcceptanceInterval)
		result << s << v;
	result << "\niterationCount" << s << iteration();
	
	result << "\nparameterValues";
	for(const auto & pn : pNames) result << s << parameterValues.at(pn);
	result << "\n";

	return result.str();

}


std::string STModelParameters::str_acceptance_rates() const
{
	std::string red = "\033[1;31m";
	std::string cyan = "\033[1;36m";
	std::string normal = "\033[0m";
	std::stringstream res;
	res << std::fixed;
	res.precision(3);
	res << "[ ";
	for(const auto ps: parSettings) {
		if(not_adapted(ps.first))
			res << red;
		else
			res << cyan;
		res << ps.second.acceptanceRate << " ";
	}
	res << normal;
	res << "]";
	return res.str();
}


std::string STModelParameters::str_sampling_variance() const
{
	std::string red = "\033[1;31m";
	std::string cyan = "\033[1;36m";
	std::string normal = "\033[0m";
	std::stringstream res;
	res << std::fixed;
	res.precision(3);
	res << "[ ";
	for(const auto ps: parSettings) {
		if(not_adapted(ps.first))
			res << red;
		else
			res << cyan;
		res << ps.second.variance << " ";
	}
	res << normal;
	res << "]";
	return res.str();
}


int STModelParameters::not_adapted(const STM::ParName & par) const
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
	if(count > 0) return false;
	else return true;
}


void STModelParameters::reset()
{
	for(const auto & p : names())
		parameterValues[p] = parSettings[p].initialValue;
	iterationCount = 0;
}


size_t STModelParameters::size() const
{ return parSettings.size(); }


double STModelParameters::sampler_variance(const STM::ParName & par) const
{ return parSettings.at(par).variance; }


void STModelParameters::set_sampler_variance(const STM::ParName & par, double val)
{ parSettings.at(par).variance = val; }


void STModelParameters::increment(int n)
{ iterationCount += n; }


int STModelParameters::iteration() const
{ return iterationCount; }


const STM::ParMap & STModelParameters::current_state() const
{ return parameterValues; }


void STModelParameters::update(const STM::ParPair & par)
{ parameterValues.at(par.first) = par.second; }


STM::ParPair STModelParameters::at(const STM::ParName & p) const
{ return STM::ParPair (p, parameterValues.at(p)); }


} // !namespace Parameters