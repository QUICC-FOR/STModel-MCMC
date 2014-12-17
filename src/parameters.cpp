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
#include "../hdr/parameters.hpp"

namespace Parameters
{

TransitionRates::TransitionRates(int intervalTime, std::map<char, long double> al, 
		std::map<char, long double> be, std::map<char, long double> th, long double ep) : 
		interval(intervalTime), alpha_logit(al), beta_logit(be), theta_logit(th), 
		epsilon_logit(ep)
{}
			
long double TransitionRates::alpha(const char state) const
{	return make_annual(alpha_logit.at(state)); }

long double TransitionRates::beta(const char state) const
{	return make_annual(beta_logit.at(state)); }

long double TransitionRates::theta(const char state) const
{	return make_annual(theta_logit.at(state)); }

long double TransitionRates::epsilon() const
{	return make_annual(epsilon_logit); }

long double TransitionRates::make_annual(const long double logit_val) const
{
	// compute the transition rate on the native interval
	// uses the inverse logit, corrected to avoid overflow
	long double val;
	if(logit_val > 0)
		val = 1.0 / (1.0 + std::exp(-logit_val));
	else
		val = std::exp(logit_val) / (1.0 + std::exp(logit_val));		

	// transform to the annual interval
	return 1 - std::pow((1 - val), interval);
}




} // !namespace Parameters