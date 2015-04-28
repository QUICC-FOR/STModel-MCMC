#ifndef STM_TYPES_H
#define STM_TYPES_H
#include <map>
#include <string>
#include <vector>

namespace STM
{
	enum class StateTypes: char;
	typedef double ParValue;
	typedef std::string ParName;
	typedef std::map<ParName, ParValue> ParMap;
	typedef std::map<StateTypes, ParValue> StateMap;
	typedef std::pair<ParName, ParValue> ParPair;

	class SerializationData
	{
		public:
		SerializationData();
		
		template<typename T> std::vector<T> at(std::string n) const;
		
		private:
		std::map<std::string, std::string> sData;
		
	};
	
	
	
	// IMPLEMENTATION
	template<typename T> std::vector<T> SerializationData::at(std::string n) const
	{
		// uses stringstreams to try to convert the raw data (a string) to the requested T type
		std::vector<T> result;
		for(const auto & stVal : sData.at(n))
		{
			std::istringstream iss;
			iss.str() = stVal;
			T val;
			iss >> val;
			result.push_back(val);
		}
		return result;
	}
	

}
#endif