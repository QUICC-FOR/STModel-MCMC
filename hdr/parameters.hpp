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
	double alpha_b, alpha_t, beta_b, beta_t, theta, theta_t, epsilon;

	TransitionRates(double ab, double at, double bb, double bt, double t, double tt, 
			double e) : alpha_b(ab), alpha_t(at), beta_b(bb), beta_t(bt), theta(t), 
			theta_t(tt), epsilon(e) {}
};


struct Parameter
/*
	data-only object containing all of the relevant information about a single parameter
	value: the current value of the parameter
	name: the name of the parameter
	variance: the sampling variance of the parameter (used for tuning)
	acceptanceRate: the most recent acceptance rate of the parameter
	initialValue: the initial value of the parameter
*/
{
	double value;
	std::string name;
	double initialValue;
	double variance;
	double acceptanceRate;
	
	Parameter() {}
	
	Parameter(double val, std::string parName, double init, double var = 1): 
			value(val), name(parName), initialValue(init), variance(var), 
			acceptanceRate(0) {}
};


class STModelParameters
{
	public:
	/*
		create the object
		data is garbage; will be replaced with something that makes sense
	*/
	STModelParameters(const std::vector<Parameter> & initPars);


	/* 
		uses environmental conditions (passed as parameters) along with the model
		parameters (stored internally) to generate the transition rates
	*/
	TransitionRates generate_rates(double env1, double env2, int interval,
	  double borealExpected) const;


	/*
		Access to the state (i.e., the vector of parameters) is provided by a single
		getter and by an elementwise setter. These protections prevent accidentally 
		overwriting the	parameters with a vector of the wrong length
		They also ensure that the parameter name always travels as a pair with the value
		
		current_state() const
		return a map keyed by parameter name with values being the current parameter value
		
		void update(std::string par, double val)
		set parameter par to val
		note that update does only basic bounds checking - it will throw an exception on
		invalid indices; however values are treated as correct and within the parameters 
		of the model
	*/	
	std::map<std::string, double> current_state() const;
	void update(std::string par, double val);


	/*
		sampler variance controls the size of each "jump" when selecting new
		parameter values. High variance means larger jumps (and consequently higher
		rejection rates) and vice-versa. These values can be automatically adjusted
		to "tune" the sampler for optimal rejection rates

		sampler_variance(std::string par)
		returns the variance of parameter par
		
		set_sampler_variance(std::string par, double val)
		sets the variance of parameter par to the value val
	*/
	const double & sampler_variance(std::string par) const;
	void set_sampler_variance(std::string par, double val);
	
	
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
	void set_acceptance_rate(std::string par, double rate);
	int not_adapted(std::string par) const;
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
	const std::vector<std::string> & names();
	void reset();
	void increment(int n = 1);

		
	private:
	std::map<char, double> make_annual(const std::map<char, double> val, int interval) const;
	double make_annual(const double logit_val, int interval) const;
	
	// need to write functions to get all of these data members initialized and filled
//	std::vector<double> parameters;
//	std::vector<double> variance;
//	std::vector<double> acceptanceRates;
	std::vector<double> targetAcceptanceInterval;
//	std::vector<double> inits;
	double iterationCount;
	std::map<std::string, Parameter> parameters;
	std::vector<std::string> parNames;
};


} // !STMParameters namespace


#endif