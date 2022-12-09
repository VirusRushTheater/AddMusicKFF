#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <sys/stat.h>

#include "Utility.h"

using namespace AddMusic;
namespace fs = std::filesystem;

void openFile(const fs::path &fileName, std::vector<uint8_t> &v)
{
	std::ifstream is (fileName, std::ios::binary);

	is.seekg(0, std::ios::end);
	unsigned int length = (unsigned int)is.tellg();
	is.seekg(0, std::ios::beg);
	v.clear();
	v.reserve(length);

	while (length > 0)
	{
		char temp;
		is.read(&temp, 1);
		v.push_back(temp);
		length--;
	}

	is.close();
}

void openTextFile(const fs::path &fileName, std::string &s)
{
	std::ifstream is(fileName);

	s.assign( (std::istreambuf_iterator<char>(is)), (std::istreambuf_iterator<char>()) );
}

time_t getTimeStamp(const fs::path &file)
{
	struct stat s;
	if (stat(file.c_str(), &s) == -1)
	{
		return 0;
	}
	return s.st_mtime;
}

void quit(int code)
{
	exit(code);
}

int execute(const fs::path &command, bool prepend)
{
     std::string tempstr = command;
     if (prepend)
     {
#ifndef _WIN32
	  tempstr.insert(0, "./");
#endif
     }
     return system(tempstr.c_str());
}

int scanInt(const std::string &str, const std::string &value)		// Scans an integer value that comes after the specified string within another string.  Must be in $XXXX format (or $XXXXXX, etc.).
{
	int i, ret;
	// if ((i = str.find(value)) == -1)
	// 	printError(std::string("Error: Could not find \"") + value + "\"", true);

	std::sscanf(str.c_str() + i + value.length(), "$%X", &ret);	// Woo C functions in C++ code!
	return ret;
}
void writeTextFile(const fs::path &fileName, const std::string &string)
{
	std::ofstream ofs;
	ofs.open(fileName, std::ios::binary);

	std::string n = string;

#ifdef _WIN32
	unsigned int i = 0;
	while (i < n.length())
	{
		if (n[i] == '\n')
		{
			n = n.insert(i, "\r");
			i++;
		}
		i++;
	}
#endif
	ofs.write(n.c_str(), n.size());

	ofs.close();
}

void insertValue(int value, int length, const std::string &find, std::string &str)
{
	int pos = str.find(find);
	if (pos == -1)
		return;
	// if (pos == -1)	{ std::cerr << "Error: \"" << find << "\" could not be found." << std::endl; quit(1); }		// // //
	
	pos += find.length();

	std::stringstream ss;
	ss << std::hex << std::uppercase << std::setfill('0') << std::setw(length) << value << std::dec;
	std::string tempStr = ss.str();
	str.replace(pos+1, length, tempStr);
}

int strToInt(const std::string &str)
{
	std::stringstream a;
	a << str;
	int j;
	a >> j;
	if (a.fail())
		throw std::invalid_argument("Could not parse string");
	return j;
}