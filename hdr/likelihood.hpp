#ifndef LIKELIHOOD_H
#define LIKELIHOOD_H

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
namespace Parameters {
	class Parameters;
	class TransitionRates;
}

namespace Likelihood {

typedef struct {
	char initial, final;
	double env1, env2;
	std::map<char, double> expected;
	
} Transition;


// function pointer type for likelihood functions
typedef long double (*LhoodFuncPtr)(Parameters::TransitionRates, Transition);


class Likelihood {
  public:
  	Likelihood(std::vector<Transition> data);
	long double compute_likelihood(Parameters::Parameters params);

  private:
  	std::vector<Transition> transitions;
	std::map<char, std::map<char, LhoodFuncPtr> > lhood;
};

} // !STModel namespace

#endif