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
#include <gsl/gsl_randist.h>
#include "../hdr/likelihood.hpp"
#include "../hdr/parameters.hpp"

using std::vector;

// total number of threads to be used if the program is built for multi-threaded use
#define S_NUM_THREADS 11

namespace STMLikelihood {

Likelihood::Likelihood(const std::vector<STMModel::STMTransition> & transitionData, 
		const std::map<std::string, PriorDist> & pr, unsigned int numThreads) : 
		transitions(transitionData), priors(pr), likelihoodThreads(numThreads)
{ }

double Likelihood::compute_log_likelihood(const STMParameters::STModelParameters & params)
{
	double sumlogl = 0;

	omp_set_num_threads(likelihoodThreads);
	{
	#pragma omp parallel for default(shared) reduction(+:sumlogl)
		for(int i = 0; i < transitions.size(); i++)
		{
			STMModel::STMTransition & dat = transitions.at(i);
			double lik = dat.transition_prob(params.current_state());
		
			// likelihood might be zero or negative (due to subtracting probabilities)
			if(lik <= 0) lik = std::nextafter(0,1);
		
			sumlogl += std::log(lik);
		} // ! for i
	} // !parallel for
	
	return sumlogl;
}



double Likelihood::log_prior(const std::pair<std::string, double> & param) const
{
	return std::log(gsl_ran_gaussian_pdf(param.second - priors.at(param.first).mean, 
			priors.at(param.first).sd));
}
} //!namespace Likelihood
