
#define BOOST_LIB_DIAGNOSTIC
#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS
#endif

// Build types: "EXE" and "LIB"
#ifndef BUILD_TYPE
	#define BUILD_TYPE	"EXE"
#endif

#ifndef _GLOBALS_H
#define _GLOBALS_H

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

#define AMKVERSION 1
#define AMKMINOR 0
#define AMKREVISION 8		// // //

#define PARSER_VERSION 4			// Used to keep track of incompatible changes to the parser

#define DATA_VERSION 0				// Used to keep track of incompatible changes to any and all compiled data, either to the SNES or to the PC

#include <cstdint>		// // //
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <memory>
#include <filesystem>

//class ROM;
class Music;
class SoundEffect;
class Sample;
class File;
class SampleGroup;

//#include "ROM.h"
#include "Music.h"
#include "Sample.h"
#include "SoundEffect.h"
#include "SampleGroup.h"
#include "BankDefine.h"
#include <sys/types.h>
#include <sys/stat.h>

// ASAR dependencies
#include <asar-dll-bindings/c/asardll.h>

// Return true if an error occurred (if "dieOnError" is true).
bool asarCompileToBIN(const std::filesystem::path &patchName, const std::filesystem::path &binOutput, bool dieOnError = true);
bool asarPatchToROM(const std::filesystem::path &patchName, const std::filesystem::path &romName, bool dieOnError = true);

void openFile(const std::filesystem::path &fileName, std::vector<uint8_t> &vector);
void openTextFile(const std::filesystem::path &fileName, std::string &string);

std::string getQuotedString(const std::string &string, int startPos, int &rawLength);

#define hex2 std::setw(2) << std::setfill('0') << std::uppercase << std::hex
#define hex4 std::setw(4) << std::setfill('0') << std::uppercase << std::hex
#define hex6 std::setw(6) << std::setfill('0') << std::uppercase << std::hex

template <typename T>
void writeFile(const std::filesystem::path &fileName, const std::vector<T> &vector)
{
	std::ofstream ofs;
	ofs.open(fileName, std::ios::binary);
	ofs.write((const char *)vector.data(), vector.size() * sizeof(T));
	ofs.close();
}

void writeTextFile(const std::filesystem::path &fileName, const std::string &string);
int execute(const std::filesystem::path &command, bool prepentDotSlash = true);

void printError(const std::string &error, bool isFatal, const std::string &fileName = "", int line = -1);
void printWarning(const std::string &error, const std::string &fileName = "", int line = -1);

void quit(int code);

int scanInt(const std::string &str, const std::string &value);

//int getSampleIndex(const std::string &name);

//void loadSample(const std::string &name, Sample *srcn);

void insertValue(int value, int length, const std::string &find, std::string &str);

int findFreeSpace(unsigned int size, int start, std::vector<uint8_t> &ROM);	// Returns a position in the ROM with the specified amount of free space, starting at the specified position.  NOT using SNES addresses!  This function writes a RATS address at the position returned.

int SNESToPC(int addr);

int PCToSNES(int addr);

int clearRATS(int PCaddr);
bool findRATS(int addr);

void addSample(const std::filesystem::path &fileName, Music *music, bool important);
void addSample(const std::vector<uint8_t> &sample, const std::string &name, Music *music, bool important, bool noLoopHeader, int loopPoint = 0, bool isBNK = false);
void addSampleGroup(const std::filesystem::path &fileName, Music *music);
void addSampleBank(const std::filesystem::path &fileName, Music *music);

int getSample(const std::filesystem::path &name, Music *music);

void preprocess(std::string &str, const std::string &filename, int &version);

int strToInt(const std::string &str);

//#define max(a, b) (a > b) ? a : b
//#define min(a, b) (a < b) ? a : b

time_t getTimeStamp(const std::filesystem::path &file);

#endif
