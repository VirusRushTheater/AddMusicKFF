#pragma once

#include "defines.h"
#include "AddmusicException.h"
#include "SPCEnvironment.h"

#include <string>
#include <memory>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace AddMusic
{

/**
 * @brief Base class for any file that might need to be preprocessed, such as
 * SoundEffects or Music files.
 */
class MMLBase
{
public:
	/**
	 * Purely virtual method to compile a source file.
	 */
	virtual void parse() = 0;

protected:
	/**
	 * Preprocess macros and directives. It also processes the AddMusic file
	 * version of this file.
	 */
	void preprocess();
	
	/**
	 * @brief Gets a piece of string between a pair of quotes starting from startPos.
	 * Also returns the raw length of the quoted string in a variable passed by reference.
	 */
	std::string getQuotedString(const std::string &string, int startPos, int &rawLength);

	int parseHex();
	int parseInt();
	int parsePitch(int letter, int octave);
	int parseNoteLength(int i);

	/**
	 * @brief skipSpaces macro turned into an inline method.
	 */
	inline void skipSpaces()
	{
		while (isspace(text[pos]))
		{
			if (text[pos] == '\n')
				line++;
			pos++;
		}
	}

	bool is_open {false};		// File has been opened

	// PARSER
	std::string text;			// File contents
	unsigned int pos 	{0};	// Parser position
	int line 			{1};	// Parser line
	fs::path filename;			// File path
	std::string name;			// Name, as assigned by the legacy lists.

	int addmusicversion {0};	// AddMusic version of this file (retrieved at preprocessing)

	// FORMERLY STATIC VARIABLES
	int defaultNoteLength;		// Default note length while parsing
	bool triplet;				// Parsing triplet?
	bool inDefineBlock;			// 

	// FRIEND CLASS FOR PRECISE ERROR REPORTING
	friend class AddmusicException;

private:

};

}