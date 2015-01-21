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
	
	Rudimentary CSV parser to assist reading input data for the sampler
	Implemented as a single class (CSV) with 3 public functions and a constructor
	
  	CSV::CSV(const char * filename, size_t header = 0)
		filename: name of the csv file to open
		header: number of header rows to skip
		throws a runtime error if opening file fails

  	std::vector<std::vector<double> > CSV::data()
  		returns a copy of all of the data stored in the CSV file
  		returned as a 2-D vector; indexed as [row][column]

  	std::vector<double> CSV::columns(size_t colNum)
  		return a single column of the CSV file; indexed by colNum

  	std::vector<std::vector<double> > CSV::columns(size_t start, size_t end) const
  		return a 2-D vector of multiple columns of the CSV
  		start: the index of the first column to return
  		end: the index after the last column to return

*/

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <stdexcept>

namespace STMInput {


class CSV {
	public:
	template<typename T> std::vector<T> column(std::string colName) const;  	
  	CSV(const char * filename, char delim = ',');

	private:
	typedef std::vector<std::string> RawCSVColumn;	// to allow us to change it later;
	std::map<std::string, RawCSVColumn> dat;
  	
	template<typename T> T str_convert(const std::string &s) const;
	std::vector<std::string> split_line(const std::string & str, char delim) const;

};


class STMInputHelper
{
	public:
	STMInputHelper(const std::string & parFileName, const std::string & transFileName);
	
	private:
	CSV paramData;
	CSV transitionData;
};


class STMInputError : public std::runtime_error
{

};


// TEMPLATE FUNCTION IMPLEMENTATION

template<typename T>
std::vector<T> CSV::column(std::string colName) const
{
	std::vector<T> result;
	const RawCSVColumn & colValues = dat.at(colName);
	for(const auto & val : colValues)
		result.push_back(str_convert<T>(val));
	return result;
}


template<typename T>
T CSV::str_convert(const std::string &s) const
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