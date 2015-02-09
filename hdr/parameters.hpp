#ifndef STM_PARAMS_H
#define STM_PARAMS_H

#include <map>
#include <vector>
#include <exception>
#include <string>

namespace STMParameters {

/*
	the following types ensure type safety for parameters, allowing types to be easily
	changed without changing other parts of the program
	under no circumstances should raw types (e.g., double) be used to refer to parameters
	
	STMParameterValueType:
		this is the type that a parameter value takes
	STMParameterNameType:
		the type that a parameter name takes (usually std::string)
	STMParameterMap:
		a map of name-value pairs for parameters
	STMParameterPair:
		a single name-value pair for parameters
*/
typedef double STMParameterValueType;
typedef std::string STMParameterNameType;
typedef std::map<STMParameterNameType, STMParameterValueType> STMParameterMap;
typedef std::pair<STMParameterNameType, STMParameterValueType> STMParameterPair;


// a convenience container with the full set of macro parameters
struct TransitionRates
{
	STMParameterValueType alpha_b, alpha_t, beta_b, beta_t, theta, theta_t, epsilon;

	TransitionRates(STMParameterValueType ab, STMParameterValueType at, 
			STMParameterValueType bb, STMParameterValueType bt, STMParameterValueType t, 
			STMParameterValueType tt, STMParameterValueType e) : alpha_b(ab), alpha_t(at),
			beta_b(bb), beta_t(bt), theta(t), theta_t(tt), epsilon(e) {}
};


struct ParameterSettings
/*
	data-only object containing all of the relevant information about a single parameter
	name: the name of the parameter
	variance: the sampling variance of the parameter (used for tuning)
	acceptanceRate: the most recent acceptance rate of the parameter
	initialValue: the initial value of the parameter
*/
{
	STMParameterNameType name;
	STMParameterValueType initialValue;
	double variance;
	double acceptanceRate;
	
	ParameterSettings() {}
	
	ParameterSettings(STMParameterNameType parName, STMParameterValueType init, double var = 1): 
			name(parName), initialValue(init), variance(var), acceptanceRate(0) {}
};


class STModelParameters
{
	public:
	/*
		create the object
		initPars: the initial set of parameters; see the documentation for the Parameter
		struct. This will give the model not only the starting values, but the parameter
		names and starting variances
	*/
	STModelParameters(const std::vector<ParameterSettings> & initPars);


	/* 
		uses environmental conditions (passed as parameters) along with the model
		parameters (stored internally) to generate the transition rates
	*/
	TransitionRates generate_rates(double env1, double env2, int interval) const;
	/* borealExpected version
		TransitionRates generate_rates(double env1, double env2, int interval,
				double borealExpected) const;
	*/

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
		
		const STMParameterPair & at(const STMParameterNameType & p) const
		returns the value of the given parameter
	*/	
	const STMParameterMap & current_state() const;
	void update(const STMParameterPair & par);
	STMParameterPair at(const STMParameterNameType & p) const;


	/*
		sampler variance controls the size of each "jump" when selecting new
		parameter values. High variance means larger jumps (and consequently higher
		rejection rates) and vice-versa. These values can be automatically adjusted
		to "tune" the sampler for optimal rejection rates

		sampler_variance(STMParameterNameType par)
		returns the variance of parameter par
		
		set_sampler_variance(std::string par, double val)
		sets the variance of parameter par to the value val
	*/
	double sampler_variance(const STMParameterNameType & par) const;
	void set_sampler_variance(const STMParameterNameType & par, double val);
	
	
	/*
		acceptance rates are supplied by the sampling engine during sampling and
		should be updated with each round of sampling
		these functions allow the engine to interact with the adaptation state
		of the parameters
		
		set_acceptance_rates(STMParameterMap rates)
		sets all acceptance rates; expects a map keyed by parameter name
		
		set_acceptance_rates(STMParameterNameType par, double rate)
		sets the acceptance rates of a single parameter
		
		not_adapted(STMParameterNameType par)
		returns -1 if the acceptance rate is too low, 1 if too high, and 0 if the 
		parameter is adapted
		
		adapted()
		returns true if all parameters are adapted
	*/
	void set_acceptance_rates(const std::map<STMParameterNameType, double> & rates);
	void set_acceptance_rate(const STMParameterNameType & par, double rate);
	int not_adapted(const STMParameterNameType & par) const;
	bool adapted() const;

	/*
		Utility functions
		size() returns the number of parameters
		names() returns a const reference to the list of parameter names; this function
			is guaranteed to always return names in the same order for the duration of
			the program (not just the life of a single instance of the STModelParameter 
			class) 
		reset() sets the parameter object to its initial state and returns the iteration 
			counter to 0
		increment(int n) increases the iteration counter by n (default of 1)
	*/
	size_t size() const;
	const std::vector<STMParameterNameType> & names() const;
	void reset();
	void increment(int n = 1);

		
	private:
	STMParameterValueType make_annual(STMParameterValueType logit_val, int interval) const;
	
	// static variables; these are shared among ALL parameter objects
	static std::vector<STMParameterNameType> parNames;
	static std::map<STMParameterNameType, ParameterSettings> parSettings;
	static std::vector<double> targetAcceptanceInterval;

	// the data below is owned by each individual object
	double iterationCount;
	STMParameterMap parameterValues;
};


} // !STMParameters namespace


#endif