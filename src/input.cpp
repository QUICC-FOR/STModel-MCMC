#include "../hdr/input.hpp"
#include <fstream>


namespace STMInput
{

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