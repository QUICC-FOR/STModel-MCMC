#ifndef STM_PARAMS_H
#define STM_PARAMS_H

#include <map>
#include <vector>
#include <exception>
#include <string>

namespace STMParameters {

class parameter_error: public std::runtime_error
{
	public:
		parameter_error() : runtime_error("Unknown state passed to Parameters object") {};
};


struct TransitionRates
{
	std::map<char, double> alpha, beta, theta;
	double epsilon;
*
	TransitionRates(std::map<char, double> al, std::map<char, double> be, 
	  std::map<char, double> th, double ep) : alpha(al), beta(be), theta(th), epsilon(ep)
	  {}
};


class STModelParameters
{
	public:
		STModelParameters(std::vector<double> data);
		/* 
			uses environmental conditions (passed as parameters) along with the model
			parameters (stored internally) to generate the transition rates
		*/
		TransitionRates generate_rates(double env1, double env2, int interval,
		  double borealExpected);
		/*
			size() and at() are functions to provide a vector-like interface for this
			class. This allows for easy iteration for MCMC
			names() provides a vector of the parameter names
		*/
		size_t size() const;
		double & at(int i);
		const std::vector<std::string> & names() const;
		
		/*
			reports on whether the current parameters reflect sampling that has been
			or not (and thus represents a markov chain).
		*/
		bool adapted(); 
		
	private:
		std::map<char, double> make_annual(const std::map<char, double> val, int interval) const;
		double make_annual(const double logit_val, int interval) const;
		std::vector<double> get_par(std::string parname, char state = '\0');
		
		std::vector<double> parameters;
		std::vector<std::string> parameterNames;
		
};


} // !STMParameters namespace


#endif