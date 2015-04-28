#ifndef STM_LIKELIHOOD_H
#define STM_LIKELIHOOD_H

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

#include <vector>
#include <map>
#include "model.hpp"
#include "stmtypes.hpp"

// forward declarations
namespace STMParameters {
	class STModelParameters;
	struct TransitionRates;
}


namespace STMLikelihood {
enum class PriorFamilies
{
	Normal=0,
	Cauchy=1
};

struct PriorDist {
	double mean;
	double sd;
	PriorFamilies family;
	PriorDist() {}
	PriorDist (double m, double s, PriorFamilies f) : mean(m), sd(s), family(f) {}
};



// function pointer type for likelihood functions
// typedef double (*LhoodFuncPtr)(STMParameters::TransitionRates, std::map<char, double>); -- disabled because I am not sure it is used anymore


class Likelihood {
	public:
  	Likelihood(const std::vector<STMModel::STMTransition> & transitionData,
  			const std::string & transitionDataOriginFile,
  			const std::map<std::string, PriorDist> & pr, unsigned int numThreads = 8);
	double compute_log_likelihood(const STMParameters::STModelParameters & params);
	double log_prior(const std::pair<std::string, double> & param) const;
	std::string serialize(char s, const std::vector<STM::ParName> & parNames) const;

	private:
	std::vector<STMModel::STMTransition> transitions;
//	std::map<char, std::map<char, LhoodFuncPtr> > lhood;-- disabled because I am not sure it is used anymore
	std::map<std::string, PriorDist> priors;
	unsigned int likelihoodThreads;
	std::string transitionFileName;		// from where did the transition data originate?
};

} // !STMLikelihood namespace

#endif