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

namespace STModel {

// forward declarations
class Parameters;

typedef struct {
	char initial, final;
	double env1, env2;
	std::map<char, double> expected;
	
} Transition;

class Likelihood {
  public:
  	Likelihood(std::vector<Transition> data);

  private:
  	// methods
	long double compute_likelihood(Parameters params)
	long double logl(Transition dat, Parameters par)
  
  	// data
  	std::vector<Transition> transitions;
	
	// state variables
	
	// settings
};

} // !STModel namespace

#endif