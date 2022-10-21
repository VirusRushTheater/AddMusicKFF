#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>

#include "Utility.h"
#include "Directory.h"

void openFile(const File &fileName, std::vector<uint8_t> &v)
{
	std::ifstream is (fileName.cStr(), std::ios::binary);

	// if (!is)
	// 	printError(std::string("Error: File \"") + fileName.cStr() + std::string("\" not found."), true);

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

void openTextFile(const File &fileName, std::string &s)
{
	std::ifstream is(fileName.cStr());

	// if (!is)
	// 	printError(std::string("Error: File \"") + fileName.cStr() + std::string("\" not found."));

	s.assign( (std::istreambuf_iterator<char>(is)), (std::istreambuf_iterator<char>()) );
}

time_t getTimeStamp(const File &file)
{
	struct stat s;
	if (stat(file, &s) == -1)
	{
		//std::cout << "Could not determine timestamp of \"" << file << "\"." << std::endl;
		return 0;
	}
	return s.st_mtime;
}


void printError(const std::string &error, bool isFatal, const std::string &fileName, int line)
{
	std::ostringstream oss;
	if (line >= 0)
	{
		oss << std::dec << "File: " << fileName << " Line: " << line << ":\n";
	}

	fputs((oss.str() + error).c_str(), stderr);
	fputc('\n', stderr);
	//puts((oss.str() + error).c_str());
	//putchar('\n');
	if (isFatal) quit(1);
}

void printWarning(const std::string &error, const std::string &fileName, int line)
{
	std::ostringstream oss;
	if (line >= 0)
	{
		oss << "File: " << fileName << " Line: " << line << ":\n";
	}
	puts((oss.str() + error).c_str());
	putchar('\n');
}

void quit(int code)
{
	exit(code);
}

int execute(const File &command, bool prepend)
{
     std::string tempstr = command.cStr();
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
	// 	printError(std::string("Error: Could not find \"") + value + "\"");

	std::sscanf(str.c_str() + i + value.length(), "$%X", &ret);	// Woo C functions in C++ code!
	return ret;
}

bool fileExists(const File &fileName)
{
	std::ifstream is(fileName.cStr(), std::ios::binary);

	bool yes = !(!is);

	if (yes)
	{
		is.seekg(0, std::ios::end);
		unsigned int length = (unsigned int)is.tellg();
		is.seekg(0, std::ios::beg);

	}

	is.close();

	return yes;
}

unsigned int getFileSize(const File &fileName)
{
	std::ifstream is(fileName.cStr(), std::ios::binary);

	if (!is) return 0;

	is.seekg(0, std::ios::end);
	unsigned int length = (unsigned int)is.tellg();
	is.close();
	return length;
}

void removeFile(const File &fileName)
{
	if (remove(fileName.cStr()) == 1)
	{
		std::cerr << "Could not delete critical file \"" << fileName.cStr() << "\"." << std::endl;		// // //
		quit(1);
	}
}

void writeTextFile(const File &fileName, const std::string &string)
{
	std::ofstream ofs;
	ofs.open(fileName.cStr(), std::ios::binary);

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
	if (pos == -1)	{ std::cerr << "Error: \"" << find << "\" could not be found." << std::endl; quit(1); }		// // //
	pos += find.length();

	std::stringstream ss;
	ss << std::hex << std::uppercase << std::setfill('0') << std::setw(length) << value << std::dec;
	std::string tempStr = ss.str();
	str.replace(pos+1, length, tempStr);
}