#ifndef ST_PARAMS_H
#define ST_PARAMS_H

#include <map>

namespace Parameters {

class TransitionRates
{
	public:
		long double theta(const char state = 0) const;
		long double beta(const char state) const;
		long double phi(const char state) const;
		long double epsilon() const;
		
};

class Parameters
{
	public:
		TransitionRates generate_rates(double env1, double env2, 
			std::map<char, double> expected);
};

} // !Parameters namespace


#endif