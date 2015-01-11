#ifndef STM_OUTPUT_H
#define STM_OUTPUT_H

/*
	QUICC-FOR ST-Model MCMC 
	OutputHelper.hpp
	
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
	
	Provides the interface between the MCMC sampler and any writing to disk.
	This provides an abstraction layer, so we can change the file format or whatever
	without messing with the rest of the program.
*/

#include <string>
#include <sstream>
#include <vector>


namespace STMOutput {

enum class OutputKeyType {
	POSTERIOR,	// for writing posterior samples
	INITS,		// initial values
	TUNING		// tuning parameters
};

enum class OutputMethodType {
	CSV,		// writes to csv files (comma separated, with a header) (not implemented)
	HDF5,		// uses the HDF5 library and writes all objects to a single file (not implemented)
	STDOUT		// dumps raw data to stdout
};
/*

class OutputWorker
{
	public:
	OutputWorker(OutputQueue * outQueue);
	
	private:
	OutputQueue * queue;
};


class OutputQueue
{
	// implements a FIFO queue with thread safety
	public:
	OutputBuffer pop();
	void push(OutputBuffer & dat);

	private:
	lock();
	release();
	std::mutex queueMutex;
	std::deque<double> data;
}
*/

class OutputOptions
{
	public:
	OutputOptions(std::string baseFileName = "STMOutput", 
		std::vector<std::string> dataNames = std::vector<std::string> (),
		OutputMethodType method = OutputMethodType::STDOUT);
		
	protected:
	std::string filename;
	std::vector<std::string> header;
	OutputMethodType outputMethod;
};


class OutputBuffer: protected OutputOptions
{
/*
	Implementation note:
	Inheritance is from OutputOptions so that all of the options can be saved internally,
	just by invoking the base class copy constructor when initializing the OutputBuffer object
	inheritance is protected so that the OutputHelper constructor is not available publicly
*/
	public:
	/*
		data: either a scalar, vector, or table (2-d vector) of doubles
		key: one of the OutputKeyType settings; controls how data are written
		options: an optional (ha!) parameter that controls further details about how
		  the object will be written to disk. See the documentation for class 
		  OutputOptions for details
	*/
	OutputBuffer(const std::vector<double> & data, OutputKeyType key, 
		  OutputOptions options = OutputOptions());
  	OutputBuffer(const std::vector<std::vector<double> > data, OutputKeyType key, 
  		  OutputOptions options = OutputOptions());

	
	/*
		save() does the work of writing the object's data to disk
		when it returns, the OutputHelper can be safely deleted
		note that consecutive calls to save() will not duplicate the output
	*/
	void save();


	private:
	std::vector<std::vector<double> > dat;
	bool dataWritten;
	
	template<typename T> 
	std::string vec_to_str(std::vector<T> inDat, char delim = ',');

};



template<typename T>
std::string OutputBuffer::vec_to_str(std::vector<T> inDat, char delim)
{
	if(inDat.size() == 0) return "";

	std::stringstream ss;
	ss << inDat[0];
	for(int i = 1; i < inDat.size(); i++)
		ss << delim << inDat[i];
	
	return ss.str();
}

} // STMOutput
#endif