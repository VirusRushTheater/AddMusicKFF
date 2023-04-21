#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <initializer_list>

#include "MMLBase.h"

namespace AddMusic
{

class SPCEnvironment;

/**
 * @brief Representation of an AddMusic sound effect.
 * 
 */
class SoundEffect : public MMLBase
{
	friend class SPCEnvironment;

public:
	std::string &getEffectiveName();		// Returns name or pointName.
	void compile(SPCEnvironment* spc_);

protected:
	inline void append(unsigned char value)
	{
		data.push_back(value);
	}
	inline void append(std::initializer_list<unsigned char> value)
	{
		data.insert(data.end(), value.begin(), value.end());
	}

private:
	std::vector<uint8_t> data;				// Binary transcription of the text data
	std::string pointName;					// Origin filename
	std::vector<uint8_t> code;				// Binary code, incorporated after compiling the ASM directives.

	// Parser facilities
	std::vector<std::string> defineStrings;
	std::vector<std::string> asmStrings;
	std::vector<std::string> asmNames;
	std::vector<std::string> jmpNames;
	std::vector<int> jmpPoses;

	// Variables used on the Rom Hack routines.
	int pointsTo {0};							// DF9 Pointer storage
	bool add0 {true};								// Add a zero at the end of the binary data.
	bool exists {false};							// Initialized?
	int posInARAM;							// Position in ARAM

	int bank;
	int index;

	// Parser sub-methods
	void parseASM();						// Generates the strings which will compiled afterwards
	void compileASM();						// Pushes the compilable strings to ASAR

	void parseJSR();
	void parseDefine();
	void parseIfdef();
	void parseIfndef();
	void parseEndif();
	void parseUndef();

	// friend class ---
};

}
