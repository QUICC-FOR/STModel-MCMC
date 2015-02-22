#ifndef STM_STATES_H
#define STM_STATES_H

#include <stdexcept>
#include <sstream>
#include <string>

namespace STMStates
{
	enum class S: char;

	class StateException: public std::runtime_error
	{
		public:
		explicit StateException(const char* message): std::runtime_error(message) { }
	};	
	
	
	class State
	{
		public:
		// the constructor throws a StateException on an invalid state
		State(char s): theState(S(s)) { self_check(); };
		S get() { return theState; };
		
		private:
		S theState;
		void self_check();
		void invalid_state()
		{
			std::stringstream msg;
			msg << "invalid state: " << char(theState);
			const std::string m = msg.str();
			throw StateException(m.c_str());
		}

	};
}




#endif
