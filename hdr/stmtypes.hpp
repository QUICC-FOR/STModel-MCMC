#ifndef STM_TYPES_H
#define STM_TYPES_H
#include <map>
#include <string>
#include <vector>
#include <iostream>
namespace STM
{
	enum class StateTypes: char; // defined in the 2- or 4-state model cpp file
	typedef double ParValue;
	typedef std::string ParName;
	typedef std::map<ParName, ParValue> ParMap;
	typedef std::map<StateTypes, ParValue> StateMap;
	typedef std::pair<ParName, ParValue> ParPair;
	
	enum class PrevalenceModelTypes
	{
		Empirical=0,
		STM=1,
		Global=2
	};
}
#endif