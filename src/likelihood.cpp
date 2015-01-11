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
#include "../hdr/likelihood.hpp"
#include "../hdr/parameters.hpp"

using std::vector;

// total number of threads to be used if the program is built for multi-threaded use
#define S_NUM_THREADS 11

namespace STMLikelihood {

Likelihood::Likelihood(vector<Transition> data) :
	transitions(data)
{

	/* 
		set up the map of functions to the likelihood expressions
		this is the canonical definition of the 4-state model, so pay attention
		
		the lhood data structure is a map of maps of lambda functions
		the first map is keyed by the initial state:
			lhood[i] produces a reference to a map of functions defining
			the transition rates possible for i, where i is a character giving the initial
			state.
		the second map is keyed by the final state:
			lhood[i][f] yields a lambda function defining the transition rate from state
			i to state f
		
		the lambda functions share a common signature
			lhood[i][f](rates, expected) returns a double giving the probability of the
			transition.
				rates: an object of class STMParameters::TransitionRates giving the values
					specified in the model
				expected: a map keyed by characters (where the key is the state)
					yields the expected prevalence of the state
		
		thus lhood[i][f](rates, expected) provides the likelihood of transitioning from i to f 
			given the data (in expected) and the parameters (in rates)
			
		THE INTENDED USE
		given expected frequencies and a set of TransitionRates rates:
			lhood[tr.initial][tr.final](rates, expected)
	*/
	lhood['B']['M'] = [](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return r.beta['T'] * (e['T'] + e['M']) * (1-r.epsilon); };

	lhood['B']['B'] = [](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return (1 - r.epsilon - (r.beta['T'] * (e['T'] + e['M']) * 
			(1-r.epsilon))); };

	lhood['T']['M'] = [](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return r.beta['B'] * (e['B']+e['M']) * (1 - r.epsilon); };
	
	lhood['T']['T'] = [](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return 1 - r.epsilon - (r.beta['B'] * (e['B']+e['M']) * (
			1 - r.epsilon)); };

	lhood['M']['B'] = [](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return r.theta['\0'] * (1 - r.theta['T']) * (1 - r.epsilon); };
		
	lhood['M']['T'] = [](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return r.theta['\0'] * r.theta['T'] * (1 - r.epsilon); };
	
	lhood['M']['M'] = [](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return r.theta['\0'] * r.theta['T'] * (1 - r.epsilon); };

	lhood['M']['R'] = lhood['T']['R'] = lhood['B']['R'] =
		[](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return r.epsilon; };

	lhood['R']['B'] = [](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return r.alpha['B'] * (e['M'] + e['B']) * (1 - r.alpha['T']*(e['T']+e['M'])); };	// phi_b	
	
	lhood['R']['T'] = [](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return r.alpha['T'] * (e['M'] + e['T']) * (1 - r.alpha['B']*(e['B']+e['M'])); };	// phi_t
	
	lhood['R']['M'] = [](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return r.alpha['B'] * (e['M'] + e['B']) * (r.alpha['T'] * (e['M'] + e['T'])); };	// phi_m

	lhood['R']['R'] = [](STMParameters::TransitionRates r, std::map<char, double> e)
		{ return 1 - 
			(r.alpha['B'] * (e['M'] + e['B']) * (1 - r.alpha['T']*(e['T']+e['M']))) - 	// phi_b
			(r.alpha['T'] * (e['M'] + e['T']) * (1 - r.alpha['B']*(e['B']+e['M']))) - 	// phi_t
			(r.alpha['B'] * (e['M'] + e['B']) * (r.alpha['T'] * (e['M'] + e['T'])));	// phi_m
		};

}


double Likelihood::compute_likelihood(STMParameters::STModelParameters params)
{
	double sumlogl = 0;

	#pragma omp parallel num_threads(S_NUM_THREADS)
	{
	#pragma omp for reduction(+:sumlogl)
	for(int i = 0; i < transitions.size(); i++)
	{
		Transition dat = transitions.at(i);
		double lik;
		STMParameters::TransitionRates rates = params.generate_rates(dat.env1, dat.env2, dat.interval, dat.expected['B']);
		lik = lhood[dat.initial][dat.final](rates, dat.expected);
		
		// likelihood might be zero or negative (due to subtracting probabilities)
		if(lik <= 0) lik = std::nextafter(0,1);
		
		sumlogl += std::log(lik);
	} // ! for i
	} // !parallel for
	
	return sumlogl;
}


} //!namespace Likelihood
