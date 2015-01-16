#include "../hdr/output.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>

using std::vector;

/*
	PROBLEM TO FIX:
	OutputBuffer is created with a map; however, there is no way to control the ordering
	of elements, so it will differ from one buffer to the next. this is a major problem
	when writing the data, as parameter values will be scrambled. Need to build in a way
	(possibly via the save() function) to access elements by name but in a consistent
	order
	
	it might look like this:
	the outputWorker, on init, gets its posterior parameter names and stores them in a
	vector
	the save() method accepts this vector as an optional parameter; if it is there, then
	it will use the ordering of items in that vector; thus, when saving posterior samples
	we can always use the same order


*/

namespace STMOutput {

OutputOptions::OutputOptions(std::string baseFileName, 
  vector<std::string> dataNames, OutputMethodType method) :
  filename(baseFileName), header(dataNames), outputMethod(method)
{ }


OutputBuffer::OutputBuffer(const std::map<std::string, double> & data, 
		const std::vector<std::string> & keyOrder, OutputKeyType key, 
		OutputOptions options) : OutputOptions(options), keys(keyOrder), 
		dataWritten(false)
{
	dat.push_back(data);
}


OutputBuffer::OutputBuffer(const std::vector<std::map<std::string, double> > & data, 
		const std::vector<std::string> & keyOrder, OutputKeyType key, 
		OutputOptions options) : OutputOptions(options), dat(data), keys(keyOrder), 
		dataWritten(false)
{ }


void OutputBuffer::save()
{
	if(dataWritten) return;
	
	// stdout implementation (the only one thus far)
	vector<std::string> stdoutData;

//	FIX --- writing the header isn't implemented, because it doesn't make the same sense
// 	now that data are stored as a map; instead should just write the keys if a flag is set
// indicating that the header needs to be written
// 	if(header.size() > 0)
// 		stdoutData.push_back(vec_to_str(header));
		
	for(const auto & row : dat) {
		std::vector<double> vals;
		for(const auto & name : keys)
			vals.push_back(row.at(name));
		stdoutData.push_back(vec_to_str(vals));		
	}

	// write to stdout
	for(vector<std::string>::iterator i = stdoutData.begin(); i != stdoutData.end(); i++) {
		std::cout << *i << std::endl;
	}
	
	dataWritten = true;
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