#ifndef STM_TYPES_H
#define STM_TYPES_H
#include <map>
#include <string>
#include <vector>
#include <iostream>
namespace STM
{
	enum class StateTypes: char;
	typedef double ParValue;
	typedef std::string ParName;
	typedef std::map<ParName, ParValue> ParMap;
	typedef std::map<StateTypes, ParValue> StateMap;
	typedef std::pair<ParName, ParValue> ParPair;
}
#endif