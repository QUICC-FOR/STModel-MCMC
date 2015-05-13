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
#include "../hdr/input.hpp"

namespace STMParameters
{

// static variable definition
std::vector<STM::ParName> STModelParameters::parNames;
std::map<STM::ParName, ParameterSettings> STModelParameters::parSettings;
double STModelParameters::optimalAcceptanceRate = 0.234;
std::vector<double> STModelParameters::targetAcceptanceInterval = {0.15, 0.5};


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


STModelParameters::STModelParameters(STMInput::SerializationData & sd)
{
	STModelParameters::parNames = sd.at("parNames");
	std::vector<STM::ParValue> inits = STMInput::str_convert<STM::ParValue>(sd.at("initialVals"));
	std::vector<double> var = STMInput::str_convert<double>(sd.at("samplerVariance"));
	std::vector<double> accept = STMInput::str_convert<double>(sd.at("acceptanceRates"));
	std::vector<STM::ParValue> vals = STMInput::str_convert<STM::ParValue>(sd.at("parameterValues"));
	for(int i = 0; i < parNames.size(); i++)
	{
		parSettings[parNames[i]] = ParameterSettings (parNames[i], inits[i], var[i], 
				accept[i]);
		parameterValues[parNames[i]] = vals[i];
	}
	
	STModelParameters::targetAcceptanceInterval = STMInput::str_convert<double>(sd.at("targetAcceptanceInterval"));
	iterationCount = STMInput::str_convert<double>(sd.at("iterationCount")[0]);
	STModelParameters::optimalAcceptanceRate = STMInput::str_convert<double>(sd.at("optimalAcceptanceRate")[0]);
	
}


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
	for(const auto & v : targetAcceptanceInterval) result << s << v;
	result << "\noptimalAcceptanceRate" << s << optimalAcceptanceRate << '\n';
	result << "iterationCount" << s << iteration();
	
	result << "\nparameterValues";
	for(const auto & pn : pNames) result << s << parameterValues.at(pn);
	result << "\n";

	return result.str();

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


double STModelParameters::acceptance_rate(const STM::ParName & par) const
{ return parSettings.at(par).acceptanceRate; }

std::string STModelParameters::str_acceptance_rates(bool inColor) const
{
	std::stringstream res;
	res << std::fixed;
	res.precision(3);

	std::string red = "\033[1;31m";
	std::string cyan = "\033[1;36m";
	std::string normal = "\033[0m";

	if(inColor)
		res << "[ ";
	for(const auto ps: parSettings) {
		if(inColor)
		{
			if(not adapted(ps.first))
				res << red;
			else
				res << cyan;
		}
		res << ps.second.acceptanceRate << " ";
	}
	if(inColor)
	{
		res << normal;
		res << "]";
	}
	return res.str();
}


std::string STModelParameters::str_sampling_variance(bool inColor) const
{
	std::string red = "\033[1;31m";
	std::string cyan = "\033[1;36m";
	std::string normal = "\033[0m";
	std::stringstream res;
	res << std::fixed;
	res.precision(3);
	if(inColor)
		res << "[ ";
	for(const auto ps: parSettings) {
		if(inColor)
		{
			if(not adapted(ps.first))
				res << red;
			else
				res << cyan;
		}
		res << ps.second.variance << " ";
	}
	if(inColor)
	{
		res << normal;
		res << "]";
	}
	return res.str();
}


double STModelParameters::optimal_acceptance_rate() const
{ return optimalAcceptanceRate; }
int STModelParameters::adaptation_status(const STM::ParName & par) const
{
	if(parSettings.at(par).acceptanceRate < optimal_acceptance_rate())
		return -1;
	else if(parSettings.at(par).acceptanceRate  > optimal_acceptance_rate())
		return 1;
	else return 0;
}


bool STModelParameters::adapted() const
{
	bool result = false;
	for(const auto & p : names())
	{
		if(not adapted(p))
		{
			result = true;
			break;
		}
	}
	return result;
}

bool STModelParameters::adapted(STM::ParName par) const
{
	if(parSettings.at(par).acceptanceRate < targetAcceptanceInterval[0] or 
			parSettings.at(par).acceptanceRate > targetAcceptanceInterval[1])
		return false;
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