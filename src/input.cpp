#include "../hdr/input.hpp"
#include "../hdr/stmtypes.hpp"
#include <iostream>
#include <algorithm>
#include <functional> 
#include <cctype>
#include <locale>

// trim from start
namespace
{
	inline std::string &ltrim(std::string &s)
	{
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
			return s;
	}

	// trim from end
	inline std::string &rtrim(std::string &s)
	{
			s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
			return s;
	}

	// trim from both ends
	inline std::string &trim(std::string &s) 
	{
			return ltrim(rtrim(s));
	}

	bool notalpha(char c)
	{ return (not std::isalpha(c)); }

}


namespace STMInput
{



void SerializationData::add(const std::string & key, const std::vector<std::string> & data)
{ sData[key] = data; }

std::vector<std::string> SerializationData::at(const std::string & key) const
{ return sData.at(key); }


STMInputHelper::STMInputHelper (const char * filename, InputType type, bool useCube, 
		char delim): useCube(useCube), prevalenceBaseName("prevalence")
{
	switch(type)
	{
		case InputType::transitions:
			setup_transitions(filename, delim);
			break;
		case InputType::parameters:
			setup_parameters(filename, delim);
			break;
		case InputType::resume:
			setup_resume(filename);
			break;
	}
}


void STMInputHelper::setup_transitions(const char * filename, char delim)
{
	std::ifstream transFile;
	transFile.open(filename);
	{
		std::stringstream err;
		if(!transFile.is_open()) err << "Failed to open " << filename << "\n";
		if(!err.str().empty()) throw std::runtime_error(err.str());
	}

	// get the column names from the first line of the CSVs and figure out their order
	std::vector<std::string> transNames;
	get_next_line(transFile, transNames, delim);
	transColIndices = get_col_numbers(transNames);

	try {
		read_transitions(transFile, delim);
	}
	catch (std::out_of_range &e) {
		display_transition_help();
		throw STMInputError();
	}
	transFile.close();
}

void STMInputHelper::setup_parameters (const char * filename, char delim)
{
	std::ifstream initFile;
	initFile.open(filename);
	{
		std::stringstream err;
		if(!initFile.is_open()) err << "Failed to open " << filename << "\n";
		if(!err.str().empty()) throw std::runtime_error(err.str());
	}


	// get the column names from the first line of the CSVs and figure out their order
	std::vector<std::string> initNames;
	get_next_line(initFile, initNames, delim);
	initColIndices = get_col_numbers(initNames);
	
	// read the rest of the data into appropriate objects
	try {
		read_inits(initFile, delim);
	}
	catch (std::out_of_range &e) {
		display_parameter_help();
		throw STMInputError();
	}
	initFile.close();
}


void STMInputHelper::setup_resume(const char * filename)
{
	std::ifstream resFile;
	resFile.open(filename);
	{
		std::stringstream err;
		if(not resFile.is_open()) err << "Failed to open " << filename << "\n";
		if(not err.str().empty()) throw std::runtime_error(err.str());
	}
	
	std::string line;
	std::string keyName;
	while(std::getline(resFile, line))
	{
		line = trim(line);
		if(line == "" or line == "{") continue;
		if(keyName.empty()) 
		{
			keyName = get_resume_key(line);
			if(not keyName.empty())
				resumeData[keyName] = STMInput::SerializationData ();
		} else
		{
			if(line == "}")
			{
				keyName.clear();
			}
			else
			{
				std::string resumeKey;
				std::vector<std::string> data;
				parse_resume_data(line, resumeKey, data);
				resumeData[keyName].add(resumeKey, data);
			}
		}
	}
	resFile.close();
}


void STMInputHelper::parse_resume_data(const std::string & inp, std::string & key, 
		std::vector<std::string> & dat)
{
	size_t index = inp.find(" ");
	key = inp.substr(0, index);
	std::string remainder = inp.substr(index+1);
	dat = split_line(remainder, ' ');
}


std::string STMInputHelper::get_resume_key(std::string line)
{
	line.erase(std::remove_if(line.begin(), line.end(), notalpha), line.end());
	return(line);
}


std::vector<STMParameters::ParameterSettings> STMInputHelper::parameter_inits()
{ return initialValues; }


std::map<std::string, STMLikelihood::PriorDist> STMInputHelper::priors()
{ return priorDists; }

std::map<std::string, STMInput::SerializationData> STMInputHelper::resume_data() const 
{return resumeData; }


std::vector<STMModel::STMTransition> STMInputHelper::transitions()
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
		STM::ParValue init = str_convert<STM::ParValue>(
				line.at(initColIndices.at("initialValue")));
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
		std::string fam = line.at(initColIndices.at("priorDist"));
		STMLikelihood::PriorFamilies family;
		if(fam == "Cauchy")
		{
			family = STMLikelihood::PriorFamilies::Cauchy;
		}
		else
		{
			family = STMLikelihood::PriorFamilies::Normal;
		}
		priorDists[parname] = STMLikelihood::PriorDist (mean, sd, family);
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
	std::cerr << "        samplerVariance -- the variance (step size) to use for tuning the MH sampler\n" << std::endl;
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
	
	
	std::vector<char> states = STMModel::State::state_names();
	std::cerr << "Prevalence columns: at least " << states.size() - 1 << " of the ";
	std::cerr << states.size() << " are required\n";
	for(auto st : states)
	{
		std::cerr << "        " << prevalenceBaseName << st;
		std::cerr << " -- the expected probability of the " << st << "state\n";
	}
	std::cerr << std::endl;
}



std::map<char, double> STMInputHelper::read_prevalence(std::vector<std::string> line)
{
	// build a map of prevalence values
	std::map<char, double> prevalence;
	std::vector<char> states = STMModel::State::state_names();
	std::vector<char> notFound;
	std::out_of_range lastOOR ("");
	for(auto st : states)
	{
		std::string prevName = prevalenceBaseName + st;
		try
		{
			prevalence[st] = str_convert<double>(line.at(transColIndices.at(prevName)));
		}
		catch (std::out_of_range e)
		{
			notFound.push_back(st);
			lastOOR = e;
		}
	}
	if(notFound.size() == 1)
	{
		double val = 1;
		for(auto pr : prevalence)
			val -= pr.second;
		prevalence[notFound[0]] = val;
	}
	else if(notFound.size() > 1)
		throw lastOOR;
	return prevalence;
}


void STMInputHelper::read_transitions(std::ifstream &file, char delim)
{
	static bool rTried = false;
	std::vector<std::string> line;
	int ln = 0;

	while(get_next_line(file, line, delim)) {
		++ln;
		if(line.empty()) continue;
		std::map<char, double> prev = read_prevalence(line);
		char initial = str_convert<char>(line.at(transColIndices.at("initial")));
		char final = str_convert<char>(line.at(transColIndices.at("final")));
		double env1 = str_convert<double>(line.at(transColIndices.at("env1")));
		double env2 = str_convert<double>(line.at(transColIndices.at("env2")));
		int interval = str_convert<int>(line.at(transColIndices.at("interval")));

		trans.push_back(STMModel::STMTransition(initial, final, env1, env2, prev, 
				interval, useCube));
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