#include "../hdr/output.hpp"
#include <iostream>
#include <stdexcept>
#include <chrono>
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
	queueMutex.lock();
	OutputBuffer returnVal = data.front();
	data.pop_front();
	queueMutex.unlock();
	return returnVal;
}


void OutputQueue::push(const OutputBuffer & dat)
{
	queueMutex.lock();
	data.push_back(dat);
	queueMutex.unlock();
}


bool OutputQueue::empty() const
{ return data.empty(); }


OutputQueue::OutputQueue()
{ }



OutputWorkerThread::OutputWorkerThread(OutputQueue * queue, bool * const terminateSignal, 
		int pollFreqMilliseconds)
{
	std::chrono::milliseconds sleepTime (pollFreqMilliseconds);
	bool terminate = false;
	while(queue != NULL && terminateSignal != NULL && !terminate) {
		terminate = *terminateSignal;
		while(!queue->empty()) {
			OutputBuffer buff = queue->pop();
			buff.save();
		}		
		if(!terminate)
			std::this_thread::sleep_for(sleepTime);		
	}
}


} // STMOutput