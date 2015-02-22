#include "../../hdr/states.hpp"

namespace STMStates {

enum class StateTypes: char
{
	T = 'T',
	B = 'B',
	M = 'M',
	R = 'R'
};


void State::self_check()
{
	using S = StateTypes;
	if(theState != S::T and theState != S::B and theState != S::M and theState != S::R)
		invalid_state();
}


}