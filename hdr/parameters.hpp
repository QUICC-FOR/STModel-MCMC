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

	TransitionRates(std::map<char, double> al, std::map<char, double> be, 
	  std::map<char, double> th, double ep) : alpha(al), beta(be), theta(th), epsilon(ep)
	  {}
};


class STModelParameters
{
	public:
	/*
		create the object
		data is garbage; will be replaced with something that makes sense
	*/
	STModelParameters(std::vector<double> data);


	/* 
		uses environmental conditions (passed as parameters) along with the model
		parameters (stored internally) to generate the transition rates
	*/
	TransitionRates generate_rates(double env1, double env2, int interval,
	  double borealExpected) const;


	/*
		Access to the state (i.e., the vector of parameters) is provided by a single
		getter (which returns a const-qualifed copy of the entire vector) and by an
		elementwise setter. These protections prevent accidentally overwriting the
		parameters with a vector of the wrong length
		
		const std::vector<double> & current_state() const
		return the vector of parameters
		
		void update(int i, double val)
		set parameter i to val
		note that update does only basic bounds checking - it will throw an exception on
		invalid indices; however values are treated as correct and within the parameters 
		of the model
	*/	
	const std::vector<double> & current_state() const;
	void update(int i, double val);


	/*
		sampler variance controls the size of each "jump" when selecting new
		parameter values. High variance means larger jumps (and consequently higher
		rejection rates) and vice-versa. These values can be automatically adjusted
		to "tune" the sampler for optimal rejection rates

		sampler_variance(int i)
		returns the variance of parameter i
		
		set_sampler_variance(int i, double val)
		sets the variance of parameter i to the value val
	*/
	const double & sampler_variance(int i) const;
	void set_sampler_variance(int i, double val);
	
	
	/*
		acceptance rates are supplied by the sampling engine during sampling and
		should be updated with each round of sampling
		these functions allow the engine to interact with the adaptation state
		of the parameters
		
		set_acceptance_rates(std::vector<double> rates)
		sets the acceptance rates to the values specified in rates (which must be
		a vector the same length as the number of parameters)
		
		not_adapted(int i)
		for parameter i, returns -1 if the acceptance rate is too low, 1 if too high,
		and 0 if the parameter is adapted
		
		adapted()
		returns true if all parameters are adapted
	*/
	void set_acceptance_rates(const std::vector<double> & rates);
	int not_adapted(int i) const;
	bool adapted() const;

	/*
		Utility functions
		size() returns the number of parameters
		names() returns a const reference to the list of parameter names
		reset() sets the parameter object to its initial state and returns the iteration 
			counter to 0
		increment(int n) increases the iteration counter by n (default of 1)
	*/
	size_t size() const;
	const std::vector<std::string> & names() const;
	void reset();
	void increment(int n = 1);

		
	private:
	std::map<char, double> make_annual(const std::map<char, double> val, int interval) const;
	double make_annual(const double logit_val, int interval) const;
	std::vector<double> get_par(std::string parname, char state = '\0') const;
	
	// need to write functions to get all of these data members initialized and filled
	std::vector<double> parameters;
	std::vector<std::string> parameterNames;
	std::vector<double> variance;
	std::vector<double> acceptanceRates;
	std::vector<double> targetAcceptanceInterval;
	std::vector<double> inits;
	double iterationCount;
};


} // !STMParameters namespace


#endif