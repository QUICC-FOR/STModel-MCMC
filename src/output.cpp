#include "../hdr/output.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>

namespace STMOutput {

// static variables
bool OutputBuffer::headerWritten = false;
std::vector<std::string> OutputBuffer::keys;
std::map<OutputKeyType, bool> OutputBuffer::append = 
{
	{OutputKeyType::posterior, false},
	{OutputKeyType::inits, false},
	{OutputKeyType::samplerVariance, false}
};


OutputOptions::OutputOptions(std::string directory, OutputMethodType method, 
	std::string baseFileName) : 
	dirname(directory), filename(baseFileName), outputMethod(method)
{ if(dirname.back() != '/') dirname = dirname + "/"; }


OutputBuffer::OutputBuffer(const std::map<std::string, double> & data, 
		const std::vector<std::string> & keyOrder, OutputKeyType key, 
		OutputOptions options) : OutputOptions(options), dataWritten(false), keyType(key)
{
	buffer_setup(keyOrder);
	dat.push_back(data);
}


OutputBuffer::OutputBuffer(const std::vector<std::map<std::string, double> > & data, 
		const std::vector<std::string> & keyOrder, OutputKeyType key, 
		OutputOptions options) : OutputOptions(options), dat(data), dataWritten(false),
		keyType(key)
{ 	buffer_setup(keyOrder); }


void OutputBuffer::buffer_setup(const std::vector<std::string> & keyOrder)
{
	if(keys.empty()) keys = keyOrder;
	if(keyType == OutputKeyType::posterior)
		filename = dirname + "posterior.csv";
}


void OutputBuffer::save()
{
	if(dataWritten) return;
	
	if(outputMethod == OutputMethodType::HDF5)
	{
		outputMethod = OutputMethodType::CSV;
		std::cerr << "HDF5 not yet supported; switching to CSV\n";
		save();
	}
	else
	{
		// stdout and CSV are very similar, so they are handled at the same time
		std::vector<std::string> outData;
		if(not headerWritten)
		{
			outData.push_back(vec_to_str(keys));
			headerWritten = true;
		}	
		for(const auto & row : dat) {
			std::vector<double> vals;
			for(const auto & name : keys)
				vals.push_back(row.at(name));
			outData.push_back(vec_to_str(vals));		
		}
		
		std::ofstream csvOutputStream;
		std::ostream & outputStream = set_output_stream(csvOutputStream);
		for(const auto &d : outData)
			outputStream << d << '\n';
		cleanup_output_stream(csvOutputStream);
	}	
	
	dataWritten = true;
}

std::ostream & OutputBuffer::set_output_stream(std::ofstream & file)
{
	if(outputMethod == OutputMethodType::CSV)
	{
		if(append.at(keyType))
			file.open(filename, std::ofstream::out | std::ofstream::app);
		else
		{
			file.open(filename);
			append[keyType] = true;
		}	
		if(not file.is_open())
			throw(std::runtime_error("Could not open file: " + filename));
		std::ostream & stream = file;
		return stream;
	}
	else if(outputMethod == OutputMethodType::STDOUT)
	{
		return std::cout;
	}
}

void OutputBuffer::cleanup_output_stream(std::ofstream & file)
{
	if(outputMethod == OutputMethodType::CSV and file.is_open())
		file.close();
}



// public members of Output Queue
OutputBuffer OutputQueue::pop()
{
	if(empty())
		throw std::runtime_error("OutputQueue::pop: tried to pop from an empty queue");
	std::lock_guard<std::mutex> lock(queueMutex);
	OutputBuffer returnVal = data.front();
	data.pop_front();
	return returnVal;
}



void OutputQueue::push(const OutputBuffer & dat)
{
	std::lock_guard<std::mutex> lock(queueMutex);
	data.push_back(dat);
}


bool OutputQueue::empty() const
{ return data.empty(); }


OutputQueue::OutputQueue()
{ }



void OutputWorkerThread::start()
{
	while(qu != NULL && endsig != NULL && !terminate) {
		terminate = *endsig;
		while(!qu->empty()) {
			OutputBuffer buff = qu->pop();
			buff.save();
		}		
		if(!terminate)
			std::this_thread::sleep_for(sleepTime);		
	}
}


OutputWorkerThread::OutputWorkerThread(OutputQueue * const queue, bool * const 
		terminateSignal, int pollFreqMilliseconds) : qu(queue), endsig(terminateSignal), 
		sleepTime(pollFreqMilliseconds), terminate(false)
{ }

} // STMOutput