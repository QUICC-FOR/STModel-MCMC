#include "../../hdr/states.hpp"



namespace STMStates {
enum class StateTypes: char
{
	Absent = '0',
	Present = '1'
};

void State::self_check()
{
	using S = StateTypes;
	if(theState != S::Absent and theState != S::Present)
		invalid_state();
}


}