#pragma once

#include "defines.h"
#include "AddmusicException.hpp"

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
	 * 
	 * TODO: List of directives, for Doxygen.
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

	/**
	 * @brief Exception shortcut which includes filename and line number.
	 * 
	 * Usually it's used with lambdas within the methods to omit error_code.
	 * One of such lambdas can be written like this:
	 * 
	 * auto preprocessError = [this](const std::string msg, bool fatality = true)
	 * {
	 *     return this->fileError(msg, AddmusicErrorcode::PARSING_ERROR, fatality);
	 * };
	 */
	// inline AddmusicException fileError(const std::string msg, AddmusicErrorcode error_code, bool fatality = true)
	// {
	// 	return AddmusicException(msg, error_code, fatality, filename, line);
	// }

	bool is_open {false};		// File has been opened

	// PARSER
	std::string text;			// File contents
	unsigned int pos {0};			// Parser position
	int line {1};					// Parser line
	fs::path filename;			// File path

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