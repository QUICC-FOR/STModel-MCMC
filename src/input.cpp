#include "../hdr/input.hpp"
#include <fstream>
#include <iostream>


namespace STMInput
{

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
		// add checks for prior mean and variance
	}
	catch (std::out_of_range &e) {
		display_parameter_help();
		throw STMInputError();
	}
	// add check for the transition data file
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