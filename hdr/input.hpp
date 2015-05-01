#ifndef STM_INPUT_H
#define STM_INPUT_H

/*
	QUICC-FOR ST-Model MCMC 
	input.hpp
	
	  Copyright 2014 Matthew V Talluto, Isabelle Boulangeat, Dominique Gravel
	
	  This program is free software; you can redistribute it and/or modify
	  it under the terms of the GNU General Public License as published by
	  the Free Software Foundation; either version 3 of the License, or (at
	  your option) any later version.
	  
	  This program is distributed in the hope that it will be useful, but
	  WITHOUT ANY WARRANTY; without even the implied warranty of
	  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	  General Public License for more details.
	
	  You should have received a copy of the GNU General Public License
	  along with this program; if not, write to the Free Software
	  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
	
*/

#include <vector>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <stdexcept>
#include "parameters.hpp"
#include "likelihood.hpp"
#include "model.hpp"

namespace STMInput {

	enum class InputType
	{
		transitions,
		parameters,
		resume
	};


template<typename T> T str_convert(const std::string &s);
template<typename T> std::vector<T> str_convert(const std::vector<std::string> & sv);

class STMInputHelper
{
	public:
	STMInputHelper (const char * filename, InputType type, bool useCube = false, char delim = ',');
	std::vector<STMParameters::ParameterSettings> parameter_inits();
	std::map<std::string, STMLikelihood::PriorDist> priors();
	std::vector<STMModel::STMTransition> transitions();
	std::map<std::string, STMInput::SerializationData> resume_data() const;
	
	private:
	void setup_transitions(const char * filename, char delim);
	void setup_parameters (const char * filename, char delim);
	void setup_resume(const char * filename);
	int get_next_line(std::ifstream &file, std::vector<std::string> &dest, char delim) const;
	std::map<std::string, int> get_col_numbers(std::vector<std::string> cNames);
	void read_inits(std::ifstream &file, char delim);
	void display_parameter_help() const;
	void display_transition_help() const;
	void read_transitions(std::ifstream &file, char delim);
	std::map<char, double> read_prevalence(std::vector<std::string> line);
	std::vector<std::string> split_line(const std::string & str, char delim) const;
	void parse_resume_data(const std::string & inp, std::string & key, std::vector<std::string> & dat);
	std::string get_resume_key(std::string line);
	
	std::map<std::string, int> initColIndices, transColIndices;
	std::vector<STMParameters::ParameterSettings> initialValues;
	std::map<std::string, STMLikelihood::PriorDist> priorDists;
	std::vector<STMModel::STMTransition> trans;
	bool useCube;
	std::string prevalenceBaseName;
	std::map<std::string, STMInput::SerializationData> resumeData;
};


class SerializationData
{
	public:
	SerializationData() {};
	void add(const std::string & key, const std::vector<std::string> & data);
	std::vector<std::string> at(const std::string & key) const;

	private:
	std::map<std::string, std::vector<std::string> > sData;
	
};



class STMInputError : public std::runtime_error
{ 
	public:
	STMInputError() : std::runtime_error("") {}
};


	// IMPLEMENTATION
	
	/*
		THIS IS FUCKITTY FUCK FUCKED
		FUCK THIS FUCKING FUCKWAD FUCKS
		
		IT WON'T FUCKING WORK BECAUSE FUCKING C++ WON'T FUCKING LET YOU FUCKING
		SPECIALIZE A FUCKING TEMPLATE FUNCTION IN A NON-FUCKING TEMPLATE CLASS
		
		SO I NEED TO JUST GHETTO-BALL IT AND DO THE FOLLOWING
		0. MOVE THE WHOLE THING TO INPUT
		1. CHANGE AT() TO JUST RETURN THE DATA
		2. WRITE NORMAL-ASS (NON-MEMBER) TEMPLATE FUNCTION THAT TAKES THE VECTOR OF STRINGS
			RETURNED BY AT AND CONVERTS IT TO SOMETHING USEFUL
		3. SPECIALIZE #2 TO NOT ACT RETARTED WHEN GIVEN A STRING
	*/



// TEMPLATE FUNCTION IMPLEMENTATION

template<typename T>
T str_convert(const std::string &s)
{
	T result;
	std::istringstream val(s);
	if(!(val >> result)) {
		std::stringstream ss;
		ss << "Cannot convert value <" << s << "> from string into requested type";
		throw( std::runtime_error (ss.str() ));
	}
	return result;
}


template<typename T> 
std::vector<T> str_convert(const std::vector<std::string> & sv)
{
	std::vector<T> result;
	for(auto i : sv)
		result.push_back(str_convert<T>(i));
	return result;
}


} // !STMInput namespace

#endif