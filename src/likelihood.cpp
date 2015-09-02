/*
	QUICC-FOR ST-Model MCMC 
	likelihood.cpp
	
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
	
	


	This program takes advantage of the openMP library to speed execution on multi-core
	systems. See the makefile and associated documentation for instructions on building
	for multi-threaded usage. The global constant S_NUM_THREADS defines how many threads
	to use; for ideal performance, this should be 1 or 2 less than the number of physical
	CPU cores on your machine; the value used here (11) was tested for optimum performance
	on an 12-core Mac Pro.

*/

#include <cmath>
#include <algorithm>
#include <omp.h>
#include <iostream>
#include <gsl/gsl_randist.h>
#include "../hdr/likelihood.hpp"
#include "../hdr/parameters.hpp"
#include "../hdr/input.hpp"

using std::vector;

namespace STMLikelihood {

Likelihood::Likelihood(const std::vector<STMModel::STMTransition> & transitionData, 
		const std::string & transitionDataOriginFile, 
		const std::map<std::string, PriorDist> & pr, unsigned int numThreads,
		int parameterInterval) : transitions(transitionData), priors(pr), 
		transitionFileName(transitionDataOriginFile), likelihoodThreads(numThreads), 
		targetInterval(parameterInterval), checked(false)
{ }


Likelihood::Likelihood(STMInput::SerializationData sd, const std::vector<std::string> &parNames,
		const std::vector<STMModel::STMTransition> & transitionData) : 
		transitions(transitionData), checked(false)
{
	transitionFileName = sd.at("transitionFileName")[0];
	likelihoodThreads = STMInput::str_convert<int>(sd.at("likelihoodThreads")[0]);
	std::vector<double> prMean = STMInput::str_convert<double>(sd.at("priorMeans"));
	std::vector<double> prSD = STMInput::str_convert<double>(sd.at("priorSD"));
	std::vector<int> prFam = STMInput::str_convert<int>(sd.at("priorFamily"));
	targetInterval = STMInput::str_convert<unsigned int>(sd.at("targetInterval")[0]);

	STMModel::STMTransition::set_prevalence_model(STM::PrevalenceModelTypes(STMInput::str_convert<int>(sd.at("prevalenceModel")[0])));
	if(STMModel::STMTransition::get_prevalence_model() != STM::PrevalenceModelTypes::Empirical)
	{
		for(auto tr : transitions)
			tr.set_global_prevalence();
	}

	for(int i = 0; i < parNames.size(); i++)
	{
		priors[parNames[i]] = PriorDist (prMean.at(i), prSD.at(i), 
				PriorFamilies(prFam.at(i)));
	}
}


void Likelihood::self_check(const STMParameters::STModelParameters & params)
{
	for(int i = 0; i < transitions.size(); i++)
	{
		STMModel::STMTransition & dat = transitions.at(i);
		double lik = dat.transition_prob(params.current_state(), targetInterval);
		if(not std::isfinite(std::log(lik)))
		{
			std::cerr << "Warning: found infinite likelihood on initialization for ";
			std::cerr << "transition data line " << i+2 << "\n";
			std::cerr << "  This data point will be removed from likelihood calculations\n";
			transitions.erase(transitions.begin() + i);
		}
	}
	checked = true;
}

std::string Likelihood::serialize(char s, const std::vector<STM::ParName> & parNames) const
{
	std::ostringstream result;

	result << "transitionFileName" << s << transitionFileName << "\n";
	result << "likelihoodThreads" << s << likelihoodThreads << "\n";
	result << "targetInterval" << s << targetInterval << "\n";
	result << "prevalenceModel" << s << int(STMModel::STMTransition::get_prevalence_model()) << "\n";

	STM::ParMap prMean, prSD;
	std::map<std::string, PriorFamilies> prFam;
	for(const auto & pn : parNames)
	{
		const PriorDist & p = priors.at(pn);
		prMean[pn] = p.mean;
		prSD[pn] = p.sd;
		prFam[pn] = p.family;
	}

	result << "priorMeans";
	for(const auto & v : parNames)
		result << s << prMean[v];
	result << "\npriorSD";
	for(const auto & v : parNames)
		result << s << prSD[v];
	result << "\npriorFamily";
	for(const auto & v : parNames)
		result << s << int(prFam[v]);
	result << "\n";

	
	return result.str();
}


double Likelihood::compute_log_likelihood(const STMParameters::STModelParameters & params)
{
	if(not checked) self_check(params);
	double sumlogl = 0;

	omp_set_num_threads(likelihoodThreads);
	{
	#pragma omp parallel for default(shared) reduction(+:sumlogl)
		for(int i = 0; i < transitions.size(); i++)
		{
			STMModel::STMTransition & dat = transitions.at(i);
			double lik = dat.transition_prob(params.current_state(), targetInterval);
			sumlogl += std::log(lik);
		} // ! for i
	} // !parallel for
	
	return sumlogl;
}



double Likelihood::log_prior(const std::pair<std::string, double> & param) const
{
	double val;
	const PriorDist & prior = priors.at(param.first);
	if(prior.family == PriorFamilies::Normal)
	{
		val = gsl_ran_gaussian_pdf(param.second - prior.mean, prior.sd);
	} else if(prior.family == PriorFamilies::Cauchy)
	{
		val = gsl_ran_cauchy_pdf(param.second - prior.mean, prior.sd);
	} else
	{
		throw(std::runtime_error("Invalid prior distribution specified"));
	}

	return std::log(val);
}


} //!namespace Likelihood
