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

class STMInputHelper
{
	public:
	STMInputHelper (const char * initFileName, const char * transFileName, 
			bool useCube = false, char delim = ',');
	std::vector<STMParameters::ParameterSettings> parameter_inits();
	std::map<std::string, STMLikelihood::PriorDist> priors();
	std::vector<STMModel::STMTransition> transitions();
	
	private:
	int get_next_line(std::ifstream &file, std::vector<std::string> &dest, char delim) const;
	std::map<std::string, int> get_col_numbers(std::vector<std::string> cNames);
	void read_inits(std::ifstream &file, char delim);
	void display_parameter_help() const;
	void display_transition_help() const;
	void read_transitions(std::ifstream &file, char delim);
	std::map<char, double> read_prevalence(std::vector<std::string> line);
	std::vector<std::string> split_line(const std::string & str, char delim) const;
	template<typename T> T str_convert(const std::string &s) const;
	
	std::map<std::string, int> initColIndices, transColIndices;
	std::vector<STMParameters::ParameterSettings> initialValues;
	std::map<std::string, STMLikelihood::PriorDist> priorDists;
	std::vector<STMModel::STMTransition> trans;
	bool useCube;
	std::string prevalenceBaseName;
};



class STMInputError : public std::runtime_error
{ 
	public:
	STMInputError() : std::runtime_error("") {}
};


// TEMPLATE FUNCTION IMPLEMENTATION

template<typename T>
T STMInputHelper::str_convert(const std::string &s) const
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


} // !STMInput namespace

#endif