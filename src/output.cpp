#include "../hdr/output.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <sstream>

namespace STMOutput {

// static variables
bool OutputBuffer::headerWritten = false;
std::vector<std::string> OutputBuffer::keys;
std::map<OutputKeyType, bool> OutputBuffer::append = 
{
	{OutputKeyType::posterior, false},
	{OutputKeyType::resumeData, false}
};


bool OutputOptions::allow_appends(OutputKeyType key)
{
	bool r = false;
	switch(key)
	{
		case OutputKeyType::posterior:
			r = true;
			break;
		case OutputKeyType::resumeData:
			r = false;
			break;
	}
	return r;
}


std::string OutputOptions::serialize(char s) const
{
	std::ostringstream result;

	result << "filename" << s << filename << "\n";
	result << "dirname" << s << dirname << "\n";
	result << "outputMethod" << s << int(outputMethod) << "\n";
		
	return result.str();
}



OutputOptions::OutputOptions(std::string directory, OutputMethodType method, 
	std::string baseFileName) : 
	dirname(directory), filename(baseFileName), outputMethod(method)
{ if(dirname.back() != '/') dirname = dirname + "/"; }


OutputOptions::OutputOptions(STM::SerializationData sd)
{
	filename = sd.at<std::string>("filename")[0];
	dirname = sd.at<std::string>("dirname")[0];
	int om = sd.at<int>("outputMethod")[0];
	outputMethod = OutputMethodType(om);
}


OutputBuffer::OutputBuffer(const std::map<std::string, double> & data, 
		const std::vector<std::string> & keyOrder, OutputKeyType key, 
		OutputOptions options) : OutputOptions(options), keyType(key)
{
	buffer_setup(keyOrder);
	dat.push_back(data);
}


OutputBuffer::OutputBuffer(const std::vector<std::map<std::string, double> > & data, 
		const std::vector<std::string> & keyOrder, OutputKeyType key, 
		OutputOptions options) : OutputOptions(options), dat(data), keyType(key)
{ 	buffer_setup(keyOrder); }


OutputBuffer::OutputBuffer(const std::string & rawOutput, OutputKeyType key, OutputOptions options) :
		OutputOptions(options), outputString(rawOutput), keyType(key)
{
	buffer_setup();
}



void OutputBuffer::buffer_setup()
{
	dataWritten = false;
	filename = dirname;
	switch (keyType)
	{
		case OutputKeyType::posterior:
			filename += "posterior.csv";
			break;
		case OutputKeyType::resumeData:
			filename += "resumeData.txt";
			break;
	}
}


void OutputBuffer::buffer_setup(const std::vector<std::string> & keyOrder)
{ 
	buffer_setup();
	if(keys.empty()) keys = keyOrder;
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
		
		if(outputString.empty())
			prepare_output_string();
		
		std::ofstream csvOutputStream;
		std::ostream & outputStream = set_output_stream(csvOutputStream);
		outputStream << outputString;
		cleanup_output_stream(csvOutputStream);
	}	
	dataWritten = true;
}


void OutputBuffer::prepare_output_string()
{
	std::ostringstream ss;
	if(keyType == OutputKeyType::posterior)
	{
		if(not headerWritten)
		{
			ss << vec_to_str(keys) << "\n";
			headerWritten = true;
		}	
		for(const auto & row : dat) {
			std::vector<double> vals;
			for(const auto & name : keys)
				vals.push_back(row.at(name));
			ss << vec_to_str(vals) << "\n";		
		}
	}
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
			append[keyType] = true && allow_appends(keyType);
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