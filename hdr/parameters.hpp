#ifndef ST_PARAMS_H
#define ST_PARAMS_H

#include <map>
#include <exception>

namespace Parameters {

class parameter_error: public std::runtime_error
{
	public:
		parameter_error() : runtime_error("Unknown state passed to Parameters object") {};
};

/*
	NOTES TO SELF
	it makes sense to have TransitionRates be a simpler object, as these things
	are going to be accessed a lot; makes no sense to call the functions more than once
	so have Parameters do all of the make_annual bullshit, etc
	TransitionRates is just a struct with 3 maps (alpha, beta, theta) and a long double
	(epsilon)
*/

class TransitionRates
{
	public:
		long double theta(const char state = 0) const;
		long double beta(const char state) const;
		long double alpha(const char state) const;
		long double epsilon() const;
		
		TransitionRates(int intervalTime, std::map<char, long double> al, 
		std::map<char, long double> be, std::map<char, long double> th, long double ep);
		
	private:
		long double make_annual(const long double logit_val) const;
		int interval;
		std::map<char, long double> alpha_logit, beta_logit, theta_logit;
		long double epsilon_logit;
};

class Parameters
{
	public:
		TransitionRates generate_rates(double env1, double env2, int interval);
};

} // !Parameters namespace


#endif