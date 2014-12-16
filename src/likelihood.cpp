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

#include "../hdr/likelihood.hpp"
#include "../hdr/parameters.hpp"

using std::vector;

// total number of threads to be used if the program is built for multi-threaded use
#define S_NUM_THREADS 11



namespace {
/*
	below are the raw expressions of the likelihood of each transition
	for use in the model, they are abstracted away somewhat using the lhood map
*/

inline long double b_to_b(TransitionRates rates, Transition dat)
{
	return (1 - rates.epsilon() - b_to_m(rates, dat));
}


inline long double b_to_m(TransitionRates rates, Transition dat)
{
	return rates.beta('T') * (dat.expected['T'] + dat.expected['M']) * (1-rates.epsilon());
}


inline long double not_r_to_r(TransitionRates rates, Transition dat)
{
	return rates.epsilon();
}


inline long double r_to_not_r(TransitionRates rates, Transition dat)
{
	return rates.phi(dat.final);
}


inline long double r_to_r(TransitionRates rates, Transition dat)
{
	return 1 - rates.phi('B') - rates.phi('T') - rates.phi('M');
}


inline long double t_to_m(TransitionRates rates, Transition dat)
{
	return rates.beta('B') * (dat.expected['B']+dat.expected['M']) * (1 - rates.epsilon());
}


inline long double t_to_t(TransitionRates rates, Transition dat)
{
	return 1 - rates.epsilon() - t_to_m(rates, dat);
}

inline long double m_to_b(TransitionRates rates, Transition dat)
{
	return rates.theta() * (1 - rates.theta('T')) * (1 - rates.epsilon());
}


inline long double m_to_t(TransitionRates rates, Transition dat)
{
	return rates.theta() * rates.theta('T') * (1 - rates.epsilon());
}


inline long double m_to_m(TransitionRates rates, Transition dat)
{
	return rates.theta() * rates.theta('T') * (1 - rates.epsilon());
}


} // ! anonymous namespace



STModel::Likelihood::Likelihood(vector<Transition> data)
transitions(data)
{

	// set up the map of functions to the likelihood expressions
	lhood['B']['B'] = &b_to_b;
	lhood['B']['M'] = &b_to_m;
	lhood['B']['R'] = &not_r_to_r;
	lhood['T']['T'] = &t_to_t;
	lhood['T']['M'] = &t_to_m;
	lhood['T']['R'] = &not_r_to_r;
	lhood['M']['B'] = &m_to_b;
	lhood['M']['T'] = &m_to_t;
	lhood['M']['M'] = &m_to_m;
	lhood['M']['R'] = &not_r_to_r;
	lhood['R']['B'] = &r_to_not_r;
	lhood['R']['T'] = &r_to_not_r;
	lhood['R']['M'] = &r_to_not_r
	lhood['R']['R'] = &r_to_r;

}










long double STModel::Likelihood::compute_likelihood(Parameters params)
{
	long double sumlogl = 0;

	#pragma omp parallel num_threads(S_NUM_THREADS)
	{
	#pragma omp for reduction(+:sumlogl)
	for(int i = 0; i < transitions.size(); i++)
	{
		const Transition currentData = transitions.at(i);
		sumlogl += logl(currentData, params);
	} // ! for i
	} // !parallel for
}



inline long double STModel::Likelihood::logl(Transition dat, Parameters par)
{
	STModel::TransitionRates trans = par.generate_rates(dat.env1, dat.env2, dat.expected);
	
}
