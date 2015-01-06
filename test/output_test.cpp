#include "../src/output.cpp"
#include <vector>
#include <string>
using std::vector;
using std::string;

int main(void)
{
	int errVal = 0;
	
	// scalar with default options
	vector<double> vals;
	vals.push_back(1.5);
	STMOutput::OutputBuffer buff (vals, STMOutput::OutputKeyType::INITS);
	buff.save();
	
	// test that 2 saves don't actually write twice
	buff.save();
	
	// vector with defaults
	vals.push_back(65.3);
	buff = STMOutput::OutputBuffer(vals, STMOutput::OutputKeyType::INITS);
	buff.save();
	
	// table with a header
	vector<vector<double> > table;
	table.push_back(vals);
	table.push_back(vals);
	vector<string> header;
	header.push_back("val 1");
	header.push_back("val 2");
	buff = STMOutput::OutputBuffer(table, STMOutput::OutputKeyType::INITS, STMOutput::OutputOptions("", header));
	buff.save();
	
	return errVal;

}