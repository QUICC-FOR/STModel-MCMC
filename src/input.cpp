#include "../hdr/input.hpp"
#include <fstream>
#include <iostream>


namespace STMInput
{

std::map<std::string, STMLikelihood::PriorDist> STMInputHelper::priors()
{
	std::map<std::string, STMLikelihood::PriorDist> result;
	std::vector<std::string> parNames = parameter_names();
	std::vector<double> means = prior_mean();
	std::vector<double> sds = prior_sd();
	for(int i = 0; i < parNames.size(); ++i)
		result[parNames[i]] = STMLikelihood::PriorDist (means.at(i), sds.at(i));
	return result;
}



STMInputHelper::STMInputHelper(const std::string & parFileName, 
		const std::string & transFileName) : paramData(parFileName.c_str()),
		transitionData(transFileName.c_str())
{
	// make sure all input data is present
	// if not, display some documentation and then raise an exception
	try {
		const std::vector<std::string> & testNames = parameter_names();
		const std::vector<STMParameters::STMParameterValueType> & testVals = intial_values();
		const std::vector<double> & testMean = prior_mean();
		const std::vector<double> & testSD = prior_sd();		
	}
	catch (std::out_of_range &e) {
		display_parameter_help();
		throw STMInputError();
	}
	try {
		const std::vector<std::string> & testInitState = initial_state();
		const std::vector<std::string> & testFinalState = final_state();
		const std::vector<double> & testEnv1 = env_1();
		const std::vector<double> & testEnv2 = env_2();
		const std::vector<std::map<std::string, double> > & testExpected = expected_probs();
		const std::vector<int> & testInterval = tr_interval();
	}
	catch (std::out_of_range &e) {
		display_transition_help();
		throw STMInputError();
	}
}


std::vector<STMParameters::ParameterSettings> STMInputHelper::parameter_inits()
{
	std::vector<double> variance;
	bool setVariance = true;
	try {
		variance = parameter_variance();
	}
	catch (std::out_of_range &e) {
		setVariance = false;
	}
	std::vector<STMParameters::ParameterSettings> result;
	std::vector<std::string> parNames = parameter_names();
	std::vector<STMParameters::STMParameterValueType> parVals = intial_values();
	for(int i = 0; i < parNames.size(); i++) {
		STMParameters::ParameterSettings ps (parNames[i], parVals[i]);
		if(setVariance)
			ps.variance = variance[i];
		result.push_back(ps);
	}
	return(result);
}


std::vector<STMLikelihood::Transition> transitions()
{
	std::vector<STMLikelihood::Transition> result;
	std::vector<char> initial = initial_state();
	for(int i = 0; i < initial.size(); ++i)
	{
	
	}
}
/*
struct Transition {
	char initial, final;
	double env1, env2;
	std::map<char, double> expected;
	int interval;
};
*/


const std::vector<std::string> & STMInputHelper::parameter_names()
{
	if(parNames.empty())
		parNames = paramData.column<std::string>("name");
	return parNames;
}


const std::vector<STMParameters::STMParameterValueType> & STMInputHelper::intial_values()
{
	if(initVals.empty())
		initVals = paramData.column<STMParameters::STMParameterValueType>("initialValue");
	return initVals;
}


const std::vector<double> & STMInputHelper::parameter_variance()
{
	if(parVariance.empty())
		parVariance = paramData.column<double>("samplerVariance");
	return parVariance;
}


const std::vector<double> & STMInputHelper::prior_mean()
{
	if(priorMean.empty())
		priorMean = paramData.column<double>("priorMean");
	return priorMean;
}


const std::vector<double> & STMInputHelper::prior_sd()
{
	if(priorSD.empty())
		priorSD = paramData.column<double>("priorSD");
	return priorSD;
}


const std::vector<double> & STMInputHelper::env_1()
{
	if(env1.empty())
		env1 = paramData.column<double>("env1");
	return env1;
}


const std::vector<double> & STMInputHelper::env_2()
{
	if(env2.empty())
		env2 = paramData.column<double>("env2");
	return env2;
}


const std::vector<std::map<std::string, double> > & STMInputHelper::expected_probs()
{
	if(expectedProbs.empty()) {
		std::vector<double> B = paramData.column<double>("expectedB");
		std::vector<double> M = paramData.column<double>("expectedM");
		std::vector<double> T = paramData.column<double>("expectedT");
		std::vector<double> R = paramData.column<double>("expectedR");
		for(int i = 0; i < B.size(); ++i) {
			std::map<std::string, double> newRow;
			newRow["B"] = B.at(i);
			newRow["M"] = M.at(i);
			newRow["T"] = T.at(i);
			newRow["R"] = R.at(i);
			expectedProbs.push_back(newRow);
		}
	}
	return expectedProbs;
}

const std::vector<int> & STMInputHelper::tr_interval()
{
	if(trInterval.empty())
		trInterval = paramData.column<int>("interval");
	return trInterval;
}


const std::vector<std::string> & STMInputHelper::final_state()
{
	if(finalState.empty())
		finalState = paramData.column<std::string>("final");
	return finalState;
}


const std::vector<std::string> & STMInputHelper::initial_state()
{
	if(initialState.empty())
		initialState = paramData.column<std::string>("initial");
	return initialState;
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


CSV::CSV (const char * filename, char delim)
{
	// parse the input file
	std::ifstream inputFile;
	inputFile.open(filename);
	if(!inputFile.is_open()) {
		std::stringstream err;
		err << "Failed to open file <" << filename << ">\n";
		throw std::runtime_error(err.str());
	}

	// get the parameter names from the first line of the CSV
	std::string line;
	std::getline(inputFile, line);
	std::vector<std::string> header = split_line(line, delim);
	for(const auto & name : header)
		dat[name] = RawCSVColumn();

	// add data to the columns one row at a time
	while(std::getline(inputFile, line)) {
		std::vector<std::string> row = split_line(line, delim);
		for(int i = 0; i < row.size(); i++)
			dat.at(header.at(i)).push_back(row[i]);
	}

	inputFile.close();
}

std::vector<std::string> CSV::split_line(const std::string & str, char delim) const
{
    std::stringstream lineStream(str);
    std::string cell;
    std::vector<std::string> result;
    while(std::getline(lineStream, cell, delim))
        result.push_back(cell);
    return result;
}




} // namespace STMInput