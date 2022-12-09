#pragma once

#define BOOST_LIB_DIAGNOSTIC
#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS
#endif

//#if defined(linux) && !defined(stricmp)
//#error Please use -Dstrnicmp=strncasecmp on Unix-like systems.
//#endif

// Grab yer ducktape, folks!
#ifdef _WIN32
#define strnicmp _strnicmp
#else
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#include <cstdint>		// // //
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <memory>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>

#include "defines.h"

namespace AddMusic
{

// Return true if an error occurred (if "dieOnError" is true).
bool asarCompileToBIN(const std::filesystem::path &patchName, const std::filesystem::path &binOutput, bool dieOnError = true);
bool asarPatchToROM(const std::filesystem::path &patchName, const std::filesystem::path &romName, bool dieOnError = true);

std::string getQuotedString(const std::string &string, int startPos, int &rawLength);

int findFreeSpace(unsigned int size, int start, std::vector<uint8_t> &ROM);	// Returns a position in the ROM with the specified amount of free space, starting at the specified position.  NOT using SNES addresses!  This function writes a RATS address at the position returned.

int SNESToPC(int addr);
int PCToSNES(int addr);
int clearRATS(int PCaddr);
bool findRATS(int addr);


// void addSample(const std::filesystem::path &fileName, Music *music, bool important);
// void addSample(const std::vector<uint8_t> &sample, const std::string &name, Music *music, bool important, bool noLoopHeader, int loopPoint = 0, bool isBNK = false);
// void addSampleGroup(const std::filesystem::path &fileName, Music *music);
// void addSampleBank(const std::filesystem::path &fileName, Music *music);

// int getSample(const std::filesystem::path &name, Music *music);

void preprocess(std::string &str, const std::string &filename, int &version);

}