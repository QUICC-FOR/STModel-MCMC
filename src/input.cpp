#include "../hdr/input.hpp"
#include <iostream>
#include <algorithm>

namespace STMInput
{


STMInputHelper::STMInputHelper (const char * initFileName, const char * transFileName, char delim)
{
	std::ifstream initFile, transFile;
	initFile.open(initFileName);
	transFile.open(transFileName);
	{
		std::stringstream err;
		if(!initFile.is_open()) err << "Failed to open " << initFileName << ">\n";
		if(!transFile.is_open()) err << "Failed to open " << transFileName << ">\n";
		if(!err.str().empty()) throw std::runtime_error(err.str());
	}


	// get the column names from the first line of the CSVs and figure out their order
	std::vector<std::string> initNames, transNames;
	get_next_line(initFile, initNames, delim);
	get_next_line(transFile, transNames, delim);
	initColIndices = get_col_numbers(initNames);
	transColIndices = get_col_numbers(transNames);
	
	// read the rest of the data into appropriate objects
	try {
		read_inits(initFile, delim);
	}
	catch (std::out_of_range &e) {
		display_parameter_help();
		throw STMInputError();
	}
	
	try {
		read_transitions(transFile, delim);
	}
	catch (std::out_of_range &e) {
		display_transition_help();
		throw STMInputError();
	}

	initFile.close();
	transFile.close();
}


std::vector<STMParameters::ParameterSettings> STMInputHelper::parameter_inits()
{ return initialValues; }


std::map<std::string, STMLikelihood::PriorDist> STMInputHelper::priors()
{ return priorDists; }


std::vector<STMLikelihood::Transition> STMInputHelper::transitions()
{ return trans; }



std::map<std::string, int> STMInputHelper::get_col_numbers(std::vector<std::string> cNames)
{
	std::map<std::string, int> result;
	for(int i = 0; i < cNames.size(); ++i)
		result[cNames[i]] = i;
	return(result);
}



int STMInputHelper::get_next_line(std::ifstream &file, std::vector<std::string> &dest, char delim) const
{
	std::string line;
	if(!std::getline(file, line))
		return 0;

	// clean quotation marks from the string
	line.erase(std::remove(line.begin(), line.end(), '"'), line.end());

	std::vector<std::string> result = split_line(line, delim);
	dest.clear();
	dest.insert(dest.begin(), result.begin(), result.end());
	return 1;
}


void STMInputHelper::read_inits(std::ifstream &file, char delim)
{
	std::vector<std::string> line;
	while(get_next_line(file, line, delim)) {
		std::string parname = line.at(initColIndices.at("name"));
		STMParameters::STMParameterValueType init = 
				str_convert<STMParameters::STMParameterValueType>(line.at(
				initColIndices.at("initialValue")));
		bool setVariance = true;
		double variance;
		try {
			variance = str_convert<double>(line.at(initColIndices.at("samplerVariance")));
		}
		catch (std::out_of_range &e) {
			setVariance = false;
		}
		STMParameters::ParameterSettings newParam;
		if(setVariance)
			newParam = STMParameters::ParameterSettings(parname, init, variance);
		else
			newParam = STMParameters::ParameterSettings(parname, init);
		
		initialValues.push_back(newParam);
		
		double mean = str_convert<double>(line.at(initColIndices.at("priorMean")));
		double sd = str_convert<double>(line.at(initColIndices.at("priorSD")));
		priorDists[parname] = STMLikelihood::PriorDist (mean, sd);
	}
}


void STMInputHelper::display_parameter_help() const
{
	std::cerr << "Error when reading data from the parameter file: missing required column\n";
	std::cerr << "File should be comma-delimited with the following columns\n";
	std::cerr << "Note that a row with column names is required, and names much match exactly\n";
	std::cerr << "    Required:\n";
	std::cerr << "        name -- the name of the parameter (as specified in the likelihood)\n";
	std::cerr << "        initialValue -- the starting value for the parameter\n";
	std::cerr << "        priorMean -- the mean of the prior\n";
	std::cerr << "        priorSD -- the standard deviation of the prior\n";
	std::cerr << "    Optional:\n";
	std::cerr << "        samplerVariance -- the variance (step size) to use for tuning the MH sampler\n\n";
}


void STMInputHelper::display_transition_help() const
{
	std::cerr << "Error when reading data from the transition file: missing required column\n";
	std::cerr << "Each row in this file is an observed transition within a single plot\n";	
	std::cerr << "The file should be comma-delimited with the following columns\n";
	std::cerr << "Note that the first row must contain column names, and names much match exactly\n";
	std::cerr << "        initial -- the initial state of the plot\n";
	std::cerr << "        final -- the final state of the plot\n";
	std::cerr << "        env1 -- the first environmental variable (temperature)\n";
	std::cerr << "        env2 -- the second environmental variable (precipitation)\n";
	std::cerr << "        interval -- number of years between the two samples\n";
	std::cerr << "        expectedB -- the expected probability of the B state\n";
	std::cerr << "        expectedT -- the expected probability of the T state\n";
	std::cerr << "        expectedM -- the expected probability of the M state\n";
	std::cerr << "        expectedR -- the expected probability of the R state\n\n";
}


void STMInputHelper::read_transitions(std::ifstream &file, char delim)
{
	std::vector<std::string> line;
	int ln = 1;
	while(get_next_line(file, line, delim)) {
		if(line.empty()) continue;
		STMLikelihood::Transition newTrans;
		newTrans.initial = str_convert<char>(line.at(transColIndices.at("initial")));
		newTrans.final = str_convert<char>(line.at(transColIndices.at("final")));
		newTrans.env1 = str_convert<double>(line.at(transColIndices.at("env1")));
		newTrans.env2 = str_convert<double>(line.at(transColIndices.at("env2")));
		newTrans.interval = str_convert<int>(line.at(transColIndices.at("interval")));
		newTrans.expected['B'] = str_convert<double>(line.at(transColIndices.at("expectedB")));
		newTrans.expected['T'] = str_convert<double>(line.at(transColIndices.at("expectedT")));
		newTrans.expected['R'] = str_convert<double>(line.at(transColIndices.at("expectedR")));
		newTrans.expected['M'] = str_convert<double>(line.at(transColIndices.at("expectedM")));
		
		trans.push_back(newTrans);
	}
}

std::vector<std::string> STMInputHelper::split_line(const std::string & str, char delim) const
{
    std::stringstream lineStream(str);
    std::string cell;
    std::vector<std::string> result;
    while(std::getline(lineStream, cell, delim))
        result.push_back(cell);
    return result;
}

} // namespace STMInput