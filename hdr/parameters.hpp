#ifndef STM_PARAMS_H
#define STM_PARAMS_H

#include <map>
#include <vector>
#include <string>
#include "stmtypes.hpp"


namespace STMInput
{
	class SerializationData;
}


namespace STMParameters {


struct ParameterSettings
/*
	data-only object containing all of the relevant information about a single parameter
	name: the name of the parameter
	variance: the sampling variance of the parameter (used for tuning)
	acceptanceRate: the most recent acceptance rate of the parameter
	initialValue: the initial value of the parameter
*/
{
	STM::ParName name;
	STM::ParValue initialValue;
	double variance;
	double acceptanceRate;
	bool isConstant;
	
	ParameterSettings() {}
	
	ParameterSettings(STM::ParName parName, STM::ParValue init, bool isConstant, double var = 2.38, 
			double acceptanceRate = 0): name(parName), initialValue(init), variance(var), 
			acceptanceRate(acceptanceRate), isConstant(isConstant) {}
};


class STModelParameters
{
	public:
	/*
		create the object
		initPars: the initial set of parameters; see the documentation for the Parameter
		struct. This will give the model not only the starting values, but the parameter
		names and starting variances
		serialize() returns a representation of the object as a string suitable for saving to disk
	*/
	STModelParameters(const std::vector<ParameterSettings> & initPars);
	STModelParameters(STMInput::SerializationData & sd);
	STModelParameters();
	std::string serialize(char s) const;


	/* 
		uses environmental conditions (passed as parameters) along with the model
		parameters (stored internally) to generate the transition rates
	*/

	const STM::ParMap & current_state() const;
	void update(const STM::ParPair & par);
	STM::ParPair at(const STM::ParName & p) const;


	/*
		sampler variance controls the size of each "jump" when selecting new
		parameter values. High variance means larger jumps (and consequently higher
		rejection rates) and vice-versa. These values can be automatically adjusted
		to "tune" the sampler for optimal rejection rates

		sampler_variance(STM::ParName par)
		returns the variance of parameter par
		
		set_sampler_variance(std::string par, double val)
		sets the variance of parameter par to the value val
		this function does bounds checking but DOES NOT raise exceptions; an attempt
		to set variance to a value outside the bounds (1e-3, 1e3) will result in the variance
		being set to the corresponding limit
	*/
	double sampler_variance(const STM::ParName & par) const;
	void set_sampler_variance(const STM::ParName & par, double val);
	
	
	/*
		acceptance rates are supplied by the sampling engine during sampling and
		should be updated with each round of sampling
		these functions allow the engine to interact with the adaptation state
		of the parameters
		
		set_acceptance_rates(STM::ParMap rates)
		sets all acceptance rates; expects a map keyed by parameter name
		
		set_acceptance_rates(STM::ParName par, double rate)
		sets the acceptance rates of a single parameter
		
		not_adapted(STM::ParName par)
		returns -1 if the acceptance rate is too low, 1 if too high, and 0 if the 
		parameter is adapted
		
		adapted()
		returns true if all parameters are adapted
		
		print_adaptation: print a columnar display of adaptation rates and variance
	*/
	void set_acceptance_rates(const std::map<STM::ParName, double> & rates);
	void set_acceptance_rate(const STM::ParName & par, double rate);
	double acceptance_rate(const STM::ParName & par) const;
	double optimal_acceptance_rate() const;
	int adaptation_status(const STM::ParName & par) const;
	bool adapted() const;
	bool adapted(STM::ParName par) const;
	void print_adaptation(bool inColor = false, int ncol=1) const;

	/*
		Utility functions
		size() returns the number of parameters
		names() returns a const reference to the list of parameter names; this function
			is guaranteed to always return names in the same order for the duration of
			the program (not just the life of a single instance of the STModelParameter 
			class)
		active_names() works as with names(), but returns only active parameters (i.e.,
			parameters that will be varied by the sampler, instead of fixed to a constant
			value)
		reset() sets the parameter object to its initial state and returns the iteration 
			counter to 0
		increment(int n) increases the iteration counter by n (default of 1)
		iteration() returns the iteration count
	*/
	size_t size() const;
	const std::vector<STM::ParName> & names() const;
	const std::vector<STM::ParName> & active_names() const;
	void reset_active_pars() const;
	void reset();
	void increment(int n = 1);
	int iteration() const;
		
	private:	
	// static variables; these are shared among ALL parameter objects
	static std::vector<STM::ParName> parNames;
	static std::vector<STM::ParName> activeParNames;
	static std::map<STM::ParName, ParameterSettings> parSettings;
	static std::vector<double> targetAcceptanceInterval;
	static double optimalAcceptanceRate;
	const static double varianceMax;
	const static double varianceMin;

	// the data below is owned by each individual object
	double iterationCount;
	STM::ParMap parameterValues;
};


} // !STMParameters namespace


#endif