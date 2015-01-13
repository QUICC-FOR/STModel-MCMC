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


// forward declarations
namespace STMParameters {
	class STModelParameters;
	struct TransitionRates;
}

namespace STMLikelihood {

struct Transition {
	char initial, final;
	double env1, env2;
	std::map<char, double> expected;
	int interval;
};


// function pointer type for likelihood functions
typedef double (*LhoodFuncPtr)(STMParameters::TransitionRates, std::map<char, double>);


class Likelihood {
	public:
  	Likelihood(const std::vector<Transition> & data);
	double compute_log_likelihood(STMParameters::STModelParameters params);
	double log_prior(int i, double val);

	private:
	struct PriorDist {
  		double mean;
  		double sd;
	};
  
	// check to make sure these variables are properly initialized
	std::vector<Transition> transitions;
	std::map<char, std::map<char, LhoodFuncPtr> > lhood;
	std::vector<PriorDist> priors;
};

} // !STMLikelihood namespace

#endif