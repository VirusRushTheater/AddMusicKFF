#include <fstream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <map>
#include <stack>
#include <algorithm>

// ASAR dependencies
#include <asar-dll-bindings/c/asardll.h>

#include "globals.h"
#include "AddmusicException.hpp"

using namespace AddMusic;
namespace fs = std::filesystem;

//ROM rom;
// std::vector<uint8_t> rom;

// Music musics[256];
// //Sample samples[256];
// std::vector<Sample> samples;
// SoundEffect soundEffectsDF9[256];
// SoundEffect soundEffectsDFC[256];
// SoundEffect *soundEffects[2] = {soundEffectsDF9, soundEffectsDFC};
// //std::vector<SampleGroup> sampleGroups;
// std::vector<std::unique_ptr<BankDefine>> bankDefines;
// std::map<std::filesystem::path, int> sampleToIndex;

// bool convert = true;
// bool checkEcho = true;
// bool forceSPCGeneration = false;
// int bankStart = 0x200000;
// bool verbose = false;
// bool aggressive = false;
// bool dupCheck = true;
// bool validateHex = true;
// bool doNotPatch = false;
// int errorCount = 0;
// bool optimizeSampleUsage = true;
// bool usingSA1 = false;
// bool allowSA1 = true;
// bool forceNoContinuePrompt = false;
// bool sfxDump = false;
// bool visualizeSongs = false;
// bool redirectStandardStreams = false;
// bool noSFX = false;

// int programPos;
// int programUploadPos;
// int mainLoopPos;
// int reuploadPos;
// int programSize;
// int highestGlobalSong;
// int totalSampleCount;
// int songCount = 0;
// int songSampleListSize;
// // bool useAsarDLL;

int findFreeSpace(unsigned int size, int start, std::vector<uint8_t> &ROM)
{
	if (size == 0)
		throw AddmusicException("Internal error: Requested free ROM space cannot be 0 bytes.", AddmusicErrorcode::FINDFREESPACE_ERROR);
	if (size > 0x7FF8)
		throw AddmusicException("Internal error: Requested free ROM space cannot exceed 0x7FF8 bytes.", AddmusicErrorcode::FINDFREESPACE_ERROR);

	size_t pos = 0;
	size_t runningSpace = 0;
	size += 8;
	int i;

	for (i = start; (unsigned)i < ROM.size(); i++)
	{
		if (runningSpace == size)
		{
			pos = i;
			break;
		}

		if (i % 0x8000 == 0)
			runningSpace = 0;

		if ((unsigned)i < ROM.size() - 4 && memcmp(&ROM[i], "STAR", 4) == 0)
		{
			unsigned short RATSSize = ROM[i+4] | ROM[i+5] << 8;
			unsigned short sizeInv = (ROM[i+6] | ROM[i+7] << 8) ^ 0xFFFF;
			if (RATSSize != sizeInv)
			{
				runningSpace += 1;
				continue;
			}
			i += RATSSize + 8;	// Would be nine if the loop didn't auto increment.
			runningSpace = 0;

		}
		else if (ROM[i] == 0 || aggressive)
		{
			runningSpace += 1;
		}
		else
		{
			runningSpace = 0;
		}
	}

	if (runningSpace == size)
		pos = i;

	if (pos == 0)
	{
		if (start == 0x080000)
			return -1;
		else
			return findFreeSpace(size, 0x080000, rom);
	}

	pos -= size;

	ROM[pos+0] = 'S';
	ROM[pos+1] = 'T';
	ROM[pos+2] = 'A';
	ROM[pos+3] = 'R';
	pos += 4;
	size -= 9;			// Not -8.  -8 would accidentally protect one too many bytes.
	ROM[pos+0] = size & 0xFF;
	ROM[pos+1] = size >> 8;
	size ^= 0xFFFF;
	ROM[pos+2] = size & 0xFF;
	ROM[pos+3] = size >> 8;
	pos -= 4;
	return pos;
}

int SNESToPC(int addr)					// Thanks to alcaro.
{
	if (addr < 0 || addr > 0xFFFFFF ||		// not 24bit
		(addr & 0xFE0000) == 0x7E0000 ||	// wram
		(addr & 0x408000) == 0x000000)		// hardware regs
		return -1;
	if (usingSA1 && addr >= 0x808000)
		addr -= 0x400000;
	addr = ((addr & 0x7F0000) >> 1 | (addr & 0x7FFF));
	return addr;
}

int PCToSNES(int addr)
{
	if (addr < 0 || addr >= 0x400000)
		return -1;

	addr = ((addr << 1) & 0x7F0000) | (addr & 0x7FFF) | 0x8000;

	if (!usingSA1 && (addr & 0xF00000) == 0x700000)
		addr |= 0x800000;

	if (usingSA1 && addr >= 0x400000)
		addr += 0x400000;
	return addr;
}

bool findRATS(int offset)
{
	if (rom[offset] != 0x53) {
		return false;
	}
	if (rom[offset+1] != 0x54) {
		return false;
	}
	if (rom[offset+2] != 0x41) {
		return false;
	}
	if (rom[offset+3] != 0x52) {
		return false;
	}
	return true;
}

int clearRATS(int offset)
{
	int size = ((rom[offset + 5] << 8) | rom[offset+4]) + 8;
	int r = size;
	while (size >= 0)
		rom[offset + size--] = 0;
	return r+1;
}

void addSample(const File &fileName, Music *music, bool important)
{
	std::vector<uint8_t> temp;
	std::string actualPath = "";

	std::string relativeDir = music->name;
	std::string absoluteDir = "samples/" + (std::string)fileName;
	std::replace(relativeDir.begin(), relativeDir.end(), '\\', '/');
	relativeDir = "music/" + relativeDir;
	relativeDir = relativeDir.substr(0, relativeDir.find_last_of('/'));
	relativeDir += "/" + (std::string)fileName;

	if (fileExists(relativeDir))
		actualPath = relativeDir;
	else if (fileExists(absoluteDir))
		actualPath = absoluteDir;
	else
		printError("Could not find sample " + (std::string)fileName, true, music->name);

	openFile(actualPath, temp);
	addSample(temp, actualPath, music, important, false);
}

void addSample(const std::vector<uint8_t> &sample, const std::string &name, Music *music, bool important, bool noLoopHeader, int loopPoint, bool isBNK)
{
	Sample newSample;
	newSample.important = important;
	newSample.isBNK = isBNK;

	if (sample.size() != 0)
	{
		if (!noLoopHeader)
		{
			if ((sample.size() - 2) % 9 != 0)
			{
				std::stringstream errstream;

				errstream << "The sample \"" + name + "\" was of an invalid length (the filesize - 2 should be a multiple of 9).  Did you forget the loop header?" << std::endl;
				printError(errstream.str(), true);
			}

			newSample.loopPoint = (sample[1] << 8) | (sample[0]);
			newSample.data.assign(sample.begin() + 2, sample.end());
		}
		else
		{
			newSample.data.assign(sample.begin(), sample.end());
			newSample.loopPoint = loopPoint;
		}
	}
	newSample.exists = true;
	newSample.name = name;

	if (dupCheck)
	{
		for (int i = 0; i < samples.size(); i++)
		{
			if (samples[i].name == newSample.name)
			{
				music->mySamples.push_back(i);
				return;						// Don't add two of the same sample.
			}
		}

		for (int i = 0; i < samples.size(); i++)
		{
			if (samples[i].data == newSample.data)
			{
				//Don't add samples from BNK files to the sampleToIndex map, because they're not valid filenames.
				if (!(newSample.isBNK)) {
					sampleToIndex[name] = i;
				}
				music->mySamples.push_back(i);
				return;
			}
		}
		//BNK files don't qualify for the next check. 
		if (!(newSample.isBNK)) {
			fs::path p1 = "./"+newSample.name;
			//If the sample in question was taken from a sample group, then use the sample group's important flag instead.
			for (int i = 0; i < bankDefines.size(); i++)
			{
				for (int j = 0; j < bankDefines[i]->samples.size(); j++)
				{
					fs::path p2 = "./samples/"+*(bankDefines[i]->samples[j]);
					if (fs::equivalent(p1, p2))
					{
						//Copy the important flag from the sample group definition.
						newSample.important = bankDefines[i]->importants[j];
						break;
					}
				}
			}
		}
	}
	//Don't add samples from BNK files to the sampleToIndex map, because they're not valid filenames.
	if (!(newSample.isBNK)) {
		sampleToIndex[newSample.name] = samples.size();
	}
	music->mySamples.push_back(samples.size());
	samples.push_back(newSample);					// This is a sample we haven't encountered before.  Add it.
}

void addSampleGroup(const File &groupName, Music *music)
{

	for (int i = 0; i < bankDefines.size(); i++)
	{
		if ((std::string)groupName == bankDefines[i]->name)
		{
			for (int j = 0; j < bankDefines[i]->samples.size(); j++)
			{
				std::string temp;
				//temp += "samples/";
				temp += *(bankDefines[i]->samples[j]);
				addSample((File)temp, music, bankDefines[i]->importants[j]);
			}
			return;
		}
	}
	std::cerr << music->name << ":\n";		// // //
	std::cerr << "The specified sample group, \"" << groupName << "\", could not be found." << std::endl;
	quit(1);
}

int bankSampleCount = 0;			// Used to give unique names to sample bank brrs.

void addSampleBank(const File &fileName, Music *music)
{
	std::vector<uint8_t> bankFile;
	std::string actualPath = "";

	std::string relativeDir = music->name;
	std::string absoluteDir = "samples/" + (std::string)fileName;
	std::replace(relativeDir.begin(), relativeDir.end(), '\\', '/');
	relativeDir = "music/" + relativeDir;
	relativeDir = relativeDir.substr(0, relativeDir.find_last_of('/'));
	relativeDir += "/" + (std::string)fileName;

	if (fileExists(relativeDir))
		actualPath = relativeDir;
	else if (fileExists(absoluteDir))
		actualPath = absoluteDir;
	else
		throw AddmusicException("Could not find sample bank " + (std::string)fileName + music->name, AddmusicErrorcode::ADDSAMPLE_ERROR);

	openFile(actualPath, bankFile);

	if (bankFile.size() != 0x8000)
		printError("The specified bank file was an illegal size.", true);
	bankFile.erase(bankFile.begin(), bankFile.begin() + 12);
	//Sample bankSamples[0x40];
	Sample tempSample;
	int currentSample = 0;
	for (currentSample = 0; currentSample < 0x40; currentSample++)
	{
		unsigned short startPosition = bankFile[currentSample * 4 + 0] | (bankFile[currentSample * 4 + 1] << 8);
		tempSample.loopPoint = (bankFile[currentSample * 4 + 2] | bankFile[currentSample * 4 + 3] << 8) - startPosition;
		tempSample.data.clear();

		if (startPosition == 0 && tempSample.loopPoint == 0)
		{
			addSample("EMPTY.brr", music, true);
			continue;
		}

		startPosition -= 0x8000;

		int pos = startPosition;

		while (pos < bankFile.size())
		{
			for (int i = 0; i < 9; i++)
			{
				tempSample.data.push_back(bankFile[pos]);
				pos++;
			}

			if ((tempSample.data[tempSample.data.size() - 9] & 1) == 1)
			{
				break;
			}
		}

		char temp[20];
		sprintf(temp, "__SRCNBANKBRR%04X", bankSampleCount++);
		tempSample.name = temp;
		addSample(tempSample.data, tempSample.name, music, true, true, tempSample.loopPoint, true);
	}
}

int getSample(const File &name, Music *music)
{
	std::string actualPath = "";

	std::string relativeDir = music->name;
	std::string absoluteDir = "samples/" + (std::string)name;
	std::replace(relativeDir.begin(), relativeDir.end(), '\\', '/');
	relativeDir = "music/" + relativeDir;
	relativeDir = relativeDir.substr(0, relativeDir.find_last_of('/'));
	relativeDir += "/" + (std::string)name;

	if (fileExists(relativeDir))
		actualPath = relativeDir;
	else if (fileExists(absoluteDir))
		actualPath = absoluteDir;
	else
		printError("Could not find sample " + (std::string)name, true, music->name);



	File ftemp = actualPath;
	std::map<File, int>::const_iterator it = sampleToIndex.begin();

	fs::path p1 = actualPath;

	while (it != sampleToIndex.end())
	{
		fs::path p2 = (std::string)it->first;
		if (fs::equivalent(p1, p2))
			return it->second;

		//if ((std::string)it->first == (std::string)ftemp)
		//	return it->second;
		it++;
	}


	return -1;
}

std::string getQuotedString(const std::string &string, int startPos, int &rawLength)
{
	std::string tempstr;
	rawLength = startPos;
	while (string[startPos] != '\"')
	{
		if (startPos > string.length())
			printError("Unexpected end of file found.", false);

		if (string[startPos] == '\\')
		{
			if (string[startPos+1] == '"')
			{
				tempstr += '"';
				startPos++;
			}
			else
			{
				printError("Error: The only escape sequence allowed is \"\\\"\".", false);	// ""\"\\\"\".""? What?
				return tempstr;
			}
		}
		else
			tempstr += string[startPos];

		startPos++;
	}
	rawLength = startPos - rawLength;
	return tempstr;
}

#define error(str) printError(str, true, filename, line)

std::string getArgument(const std::string &str, char endChar, unsigned int &pos, const std::string &filename, int line, bool breakOnNewLines)
{
	std::string temp;

	while (true)
	{
		if (endChar == ' ')
		{
			if (str[pos] == ' ' || str[pos] == '\t')
				break;
		}
		else
			if (str[pos] == endChar)
				break;

		if (pos == str.length())
			error("Unexpected end of file found.");
		if (breakOnNewLines) if (str[pos] == '\r' || str[pos] == '\n') break;			// Break on new lines.
		temp += str[pos];
		pos++;
	}

	return temp;
}

#define skipSpaces				\
while (i < str.size() && isspace(str[i]))	\
{						\
	if (str[i] == '\n' || str[i] == '\r')	\
		break;				\
	i++;					\
}

void preprocess(std::string &str, const std::string &filename, int &version)
{
	// Handles #ifdefs.  Maybe more later?

	std::map<std::string, int> defines;

	unsigned int i = 0;

	int level = 0, line = 1;

	std::string newstr;

	bool okayToAdd = true;

	i = 0;

	std::stack<bool> okayStatus;


	while (true)
	{
		if (i == str.length()) break;

		if (i < str.length())
			if (str[i] == '\n')
				line++;

		if (str[i] == '\"')
		{
			i++;
			if (okayToAdd)
			{
				newstr += '\"';
				newstr += getArgument(str, '\"', i, filename, line, false) + '\"';
			}
			i++;

		}
		else if (str[i] == '#')
		{
			std::string temp;
			i++;

			if (strncmp(str.c_str() + i, "amk=1", 5) == 0)					// Special handling so that we can have #amk=1.
			{
				if (version >= 0)
					version = 1;
				i+=5;
				continue;
			}

			temp = getArgument(str, ' ', i, filename, line, true);

			if (temp == "define")
			{
				if (!okayToAdd) { level++; continue; }

				skipSpaces;
				std::string temp2 = getArgument(str, ' ', i, filename, line, true);
				if (temp2.length() == 0)
					error("#define was missing its argument.");

				skipSpaces;
				std::string temp3 = getArgument(str, ' ', i, filename, line, true);
				if (temp3.length() == 0)
					defines[temp2] = 1;
				else
				{
					int j;
					try
					{
						j = strToInt(temp3);
					}
					catch (...)
					{
						error("Could not parse integer for #define.");
					}
					defines[temp2] = j;
				}
			}
			else if (temp == "undef")
			{
				if (!okayToAdd) { level++; continue; }

				skipSpaces;
				std::string temp2 = getArgument(str, ' ', i, filename, line, true);
				if (temp2.length() == 0)
					error("#undef was missing its argument.");
				defines.erase(temp2);
			}
			else if (temp == "ifdef")
			{
				if (!okayToAdd) { level++; continue; }

				skipSpaces;
				std::string temp2 = getArgument(str, ' ', i, filename, line, true);
				if (temp2.length() == 0)
					error("#ifdef was missing its argument.");

				okayStatus.push(okayToAdd);

				if (defines.find(temp2) == defines.end())
					okayToAdd = false;
				else
					okayToAdd = true;

				level++;
			}
			else if (temp == "ifndef")
			{
				if (!okayToAdd) { level++; continue; }

				skipSpaces;
				std::string temp2 = getArgument(str, ' ', i, filename, line, true);

				okayStatus.push(okayToAdd);

				if (temp2.length() == 0)
					error("#ifndef was missing its argument.");
				if (defines.find(temp2) != defines.end())
					okayToAdd = false;
				else
					okayToAdd = true;

				level++;
			}
			else if (temp == "if")
			{
				if (!okayToAdd) { level++; continue; }

				skipSpaces;
				std::string temp2 = getArgument(str, ' ', i, filename, line, true);
				if (temp2.length() == 0)
					error("#if was missing its first argument.");

				if (defines.find(temp2) == defines.end())
					error("First argument for #if was never defined.");


				skipSpaces;
				std::string temp3 = getArgument(str, ' ', i, filename, line, true);
				if (temp3.length() == 0)
					error("#if was missing its comparison operator.");


				skipSpaces;
				std::string temp4 = getArgument(str, ' ', i, filename, line, true);
				if (temp4.length() == 0)
					error("#if was missing its second argument.");

				okayStatus.push(okayToAdd);

				int j;
				try
				{
					j = strToInt(temp4);
				}
				catch (...)
				{
					error("Could not parse integer for #if.");
				}

				if (temp3 == "==")
					okayToAdd = (defines[temp2] == j);
				else if (temp3 == ">")
					okayToAdd = (defines[temp2] > j);
				else if (temp3 == "<")
					okayToAdd = (defines[temp2] < j);
				else if (temp3 == "!=")
					okayToAdd = (defines[temp2] != j);
				else if (temp3 == ">=")
					okayToAdd = (defines[temp2] >= j);
				else if (temp3 == "<=")
					okayToAdd = (defines[temp2] <= j);
				else
					error("Unknown operator for #if.");

				level++;
			}
			else if (temp == "endif")
			{
				if (level > 0)
				{
					level--;
					okayToAdd = okayStatus.top();
					okayStatus.pop();
				}
				else
					error("There was an #endif without a matching #ifdef, #ifndef, or #if.");
			}
			else if (temp == "amk")
			{
				if (version >= 0)
				{
					skipSpaces;
					std::string temp = getArgument(str, ' ', i, filename, line, true);
					if (temp.length() == 0)
					{
						printError("#amk must have an integer argument specifying the version.", false, filename, line);
					}
					else
					{
						int j;
						try
						{
							j = strToInt(temp);
						}
						catch (...)
						{
							error("Could not parse integer for #amk.");
						}
						version = j;
						if (version == 3)
						{
							error("Codec's AddmusicK Beta has not been implemented yet.");
						}
					}
				}
			}
			else if (temp == "amm")
				version = -2;
			else if (temp == "am4")
				version = -1;
			else
			{
				if (okayToAdd)
				{
					newstr += "#";
					newstr += temp;
				}
			}

		}
		else
		{
			if (okayToAdd || str[i] == '\n') newstr += str[i];
			i++;
		}

	}

	if (level != 0)
		error("There was an #ifdef, #ifndef, or #if without a matching #endif.");

	i = 0;
	if (version != -2)			// For now, skip comment erasing for #amm songs.  #amk songs will follow suit in a later version.
	{

		while (i < newstr.length())
		{
			if (newstr[i] == ';')
			{
				while (i < newstr.length() && newstr[i] != '\n')
					newstr = newstr.erase(i, 1);
				continue;
			}
			i++;
		}
	}

	str = newstr;
}


bool asarCompileToBIN(const File &patchName, const File &binOutputFile, bool dieOnError)
{
	fs::remove("temp.log");
	fs::remove("temp.txt");

	int binlen = 0;
	int buflen = 0x10000;		// 0x10000 instead of 0x8000 because a few things related to sound effects are stored at 0x8000 at times.

	uint8_t *binOutput = (uint8_t *)malloc(buflen);

	asar_patch(patchName.cStr(), (char *)binOutput, buflen, &binlen);
	int count = 0, currentCount = 0;
	std::string printout;

	asar_getprints(&count);

	while (currentCount != count)
	{
		printout += asar_getprints(&count)[currentCount];
		printout += "\n";
		currentCount++;
	}
	if (count > 0)
		writeTextFile("temp.txt", printout);
///////////////////////////////////////////////////////////////////////////////
	count = 0; currentCount = 0;
	printout.clear();

	asar_geterrors(&count);

	while (currentCount != count)
	{
		printout += asar_geterrors(&count)[currentCount].fullerrdata + (std::string)"\n";
		currentCount++;
	}
	if (count > 0)
	{
		writeTextFile("temp.log", printout);
		free(binOutput);
		return false;
	}

	std::vector<uint8_t> v;
	v.assign(binOutput, binOutput + binlen);
	writeFile(binOutputFile, v);
	free(binOutput);
	return true;
}

bool asarPatchToROM(const File &patchName, const File &romName, bool dieOnError)
{
	fs::remove("temp.log");
	fs::remove("temp.txt");

	int binlen = 0;
	int buflen;

	std::vector<uint8_t> patchrom;
	openFile(romName, patchrom);
	buflen = patchrom.size();

	asar_patch(patchName.cStr(), (char *)patchrom.data(), buflen, &buflen);
	int count = 0, currentCount = 0;
	std::string printout;

	asar_getprints(&count);

	while (currentCount != count)
	{
		printout += asar_getprints(&count)[currentCount];
		printout += "\n";
		currentCount++;
	}
	if (count > 0)
		writeTextFile("temp.txt", printout);
///////////////////////////////////////////////////////////////////////////////
	count = 0; currentCount = 0;
	printout.clear();

	asar_geterrors(&count);

	while (currentCount != count)
	{
		printout += asar_geterrors(&count)[currentCount].fullerrdata + (std::string)"\n";
		currentCount++;
	}
	if (count > 0)
	{
		writeTextFile("temp.log", printout);
		return false;
	}

	writeFile(romName, patchrom);
	return true;
}
