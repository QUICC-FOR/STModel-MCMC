#include "../hdr/output.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>

using std::vector;


namespace STMOutput {

OutputOptions::OutputOptions(std::string baseFileName, 
  vector<std::string> dataNames, OutputMethodType method) :
  filename(baseFileName), header(dataNames), outputMethod(method)
{ }


OutputBuffer::OutputBuffer(const vector<double> & data, OutputKeyType key, 
  OutputOptions options) : OutputOptions(options), dataWritten(false)
{
	dat.push_back(data);
}


OutputBuffer::OutputBuffer(const vector<vector<double> > data, OutputKeyType key, 
  OutputOptions options) : OutputOptions(options), dat(data), dataWritten(false)
{ }

void OutputBuffer::save()
{
	if(dataWritten) return;
	
	
	
	// stdout implementation (the only one thus far)
	vector<std::string> stdoutData;
	if(header.size() > 0)
		stdoutData.push_back(vec_to_str(header));
		
	for(vector<vector<double> >::iterator i = dat.begin(); i != dat.end(); i++) {
		stdoutData.push_back(vec_to_str(*i));
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