#include "../hdr/output.hpp"
#include <iostream>

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

}