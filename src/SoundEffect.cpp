#include <cmath>
#include <regex>
#include <sstream>
#include <iomanip>
#include <filesystem>

#include "AddmusicException.hpp"
#include "SoundEffect.h"
#include "asarBinding.h"

using namespace AddMusic;

namespace fs = std::filesystem;

void SoundEffect::parse()
{
	text += "                   ";
	int version = 0;			// Unused for sound effects for now.
	preprocess();

	// Initializing static variables
	pos = 0;
	line = 0;
	triplet = false;
	defaultNoteLength = 8;
	inDefineBlock = false;

	unsigned int instr = -1;			// Current instrument
	unsigned char lastNote = -1;		// 
	bool firstNote = true;
	bool pitchSlide = false;
	int octave = 4;
	unsigned char lastNoteValue = -1;
	int volume[2] = {0x7F, 0x7F};
	unsigned char i8, j8;
	int i, j;
	bool updateVolume = false;			// Adjust the volume of the next note.
	bool inComment = false;				// Ignore the line, it is commented (comment with a colon ;).

	// Aliases for attributes to be used within lambdas.
	int* line_p = 			&line;
	unsigned int* pos_p = 	&pos;
	fs::path& filename_p = 	filename;
	std::string& text_p = 	text;

	// Macro for throwing an error.
	auto parseError = [&filename_p, &line_p](const std::string msg, bool fatality = true)
	{
		return AddmusicException(msg, AddmusicErrorcode::PARSING_ERROR, fatality, filename_p, *line_p);
	};

	// Main parsing routine
	while (pos < text.size())
	{
		// End a comment after a line break
		if (text[pos] == '\n')
			inComment = false;
		
		// Ignore the following parsing if a comment is detected.
		else if (inComment == true)
			pos++;
		if (inComment)
			continue;

		// Parsing
		switch (text[pos])
		{

		// Preprocessor directives.
		case '#':
			// #asm -> parseASM
			if (text.substr(pos+1, 3) == "asm")
				parseASM();
			
			// #jsr -> parseJSR
			else if (text.substr(pos+1, 3) == "jsr")
				parseJSR();

			// #define -> parseDefine
			else if (text.substr(pos+1, 6) == "define")
				parseDefine();
			
			// #undef -> parseUndef
			else if (text.substr(pos+1, 5) == "undef")
				parseUndef();
			
			// #ifdef -> parseIfdef
			else if (text.substr(pos+1, 5) == "ifdef")
				parseIfdef();
			
			// #ifndef -> parseIfndef
			else if (text.substr(pos+1, 6) == "ifndef")
				parseIfndef();
			
			// #endif -> parseEndif
			else if (text.substr(pos+1, 5) == "endif")
				parseEndif();

			// Another directive is not allowed.
			else
			{
				pos++;
				throw parseError("Channel declarations are not allowed in sound effects.");
			}
			continue;
		
		// Exclamation mark counts as an EOL, I guess?
		// Command has been deprecated on Music.cpp I see.
		case '!':
			pos = ~0;
			continue;
		
		// v{number} -> Volume command. Number is between 0 and 127.
		case 'v':
			pos++;
			i = parseInt();
			if (i == -1) 
				throw parseError("Error parsing volume command.");
			if (i > 0x7F)
				throw parseError("Volume too high.  Only values from 0 - 127 are allowed.");

			volume[0] = i;
			volume[1] = i;
			skipSpaces();

			if (text[pos] == ',')
			{
				pos++;
				skipSpaces();
				i = parseInt();
				if (i == -1)
					throw parseError("Error parsing volume command.");
				if (i > 0x7F)
				throw parseError("Illegal value for volume command.  Only values between 0 and 127 are allowed.");
				volume[1] = i;
			}

			updateVolume = true;
			break;
		
		// i{number} -> Adjust the length of following notes.
		case 'l':
			pos++;
			i = parseInt();
			if (i == -1) { throw parseError("Error parsing 'l' directive.", false); continue; }
			if (i > 192) { throw parseError("Illegal value for 'l' directive.", false); continue; }
			defaultNoteLength = i;
			break;

		// @{number} -> Adjust the patch number.
		case '@':
			pos++;
			i = parseInt();
			if (i <  0x00)
				throw parseError("Error parsing instrument ('@') command.");
			if (i > 0x7F)
				throw parseError("Illegal value for instrument ('@') command.");

			j = -1;

			skipSpaces();

			if (text[pos] == ',')
			{
				pos++;
				skipSpaces();
				j = parseInt();
				if (j < 0)
					throw parseError("Error parsing noise instrument ('@,') command.");
				if (j > 0x1F)
					throw parseError("Illegal value for noise instrument ('@,') command.  Only values between 0 and 31");
			}

			append(0xDA);
			if (j != -1)
				append(0x80 | j);
			append(i);
			instr = i;
			break;

		// o{number} -> Changes the octave of the following notes.
		case 'o':
			pos++;
			i = parseInt();
			if (i == -1)
				throw parseError("Error parsing octave directive.");
			if (i < 0 || i > 6)
				throw parseError("Illegal value for octave command.");

			octave = i;
			break;

		// ${number} -> Hex command. It's inserted directly into the SFX binary.
		case '$':
			pos++;
			i = parseHex();
			if (i == -1)
				throw parseError("Error parsing hex command.");
			if (i > 0xFF)
				throw parseError("Illegal hex value.");

			append(i);

			break;

		// > -> Increases the octave in one step for the following notes.
		case '>':
			pos++;
			if (++octave > 6)
				throw parseError("Illegal octave reached via '>' directive.");
			break;

		// > -> Decreases the octave in one step for the following notes.
		case '<':
			pos++;
			if (--octave < 1)
				throw parseError("Illegal octave reached via '<' directive.");
			break;

		// { -> Enables a triplet block
		case '{':
			if (triplet)
				throw parseError("Triplet enable directive specified in a triplet block.");
			triplet = true;
			break;

		// { -> Disables a triplet block
		case '}':
			if (!triplet)
				throw parseError("Triplet disable directive specified outside a triplet block.");
			triplet = false;
			break;

		// Actual notes:
		// [a-g] -> white notes
		// r -> rest
		// ^ -> tied note
		// Most of the data insertion happens here.
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'r': case '^':
			j = text[pos];	// Character

			if (j == 'r')
				i = 0xC7, pos++;
			else if (j == '^')
				i = 0xC6, pos++;
			else
				i = parsePitch(j, octave);

			if (i < 0)
				i = 0xC7;

			j = parseNoteLength(defaultNoteLength);

			if (i == lastNoteValue && !updateVolume)
				i = 0;

			// You can make a pitch bend with the & character.
			if (i != 0xC6 && i != 0xC7 && (text[pos] == '&' || pitchSlide))
			{
				pitchSlide = true;
				if (firstNote == true)
				{
					if (lastNote == -1)
						lastNote = i;
					else
					{
						if (j > 0)
						{
							append(j);
							lastNoteValue = j;
						}
						if (updateVolume)
						{
							append(volume[0]);
							if (volume[0] != volume[1]) append(volume[1]);
							updateVolume = false;
						}

						append(0xDD);
						append(lastNote);
						append(0x00);
						append(lastNoteValue);
						append(i);
						firstNote = false;
					}
				}
				else
				{
					if (j > 0)
					{
						append(j);
						lastNoteValue = j;
					}

					if (updateVolume)
					{
						append(volume[0]);
						if (volume[0] != volume[1]) append(volume[1]);
						updateVolume = false;
					}

					append(0xEB);
					append(0x00);
					append(lastNoteValue);
					append(i);
				}

				if (j < 0) lastNoteValue = j;
				pos++;
				break;
			}
			else
			{
				firstNote = true;
				pitchSlide = false;
			}

			if (j >= 0x80)
			{
				append(0x7F);

				if (updateVolume)
				{
					append(volume[0]);
					if (volume[0] != volume[1]) append(volume[1]);
					updateVolume = false;
				}

				append(i);

				j -= 0x7F;

				while (j > 0x7F)
				{
					j -= 0x7F;
					append(0xC6);
				}

				if (j > 0)
				{
					if (j != 0x7F) append(j);
					append(0xC6);
				}

				lastNoteValue = j;
				break;


			}
			else if (j > 0)
			{
				append(j);
				lastNoteValue = j;
				if (updateVolume)
				{
					append(volume[0]);
					if (volume[0] != volume[1]) append(volume[1]);
					updateVolume = false;
				}

				append(i);
			}
			else
				append(i);
			break;

			// Phew...
		case '\n':
			pos++;
			line++;
			break;

		case ';':
			pos++;
			inComment = true;
			break;

		default:
			if (!isspace(text[pos]))
				throw parseError(std::string("Warning: Unexpected symbol '") + text[pos] + std::string("'found."), false);

			pos++;
			break;

		}

	}

	// TODO: Transform this global directive into something local.
	/*
	if (soundEffects[bank][index].add0)
		append(0x00);
	*/

	// compileASM();
}

void SoundEffect::parseASM()
{
	auto parseASMError = [this](const std::string msg, bool fatality = false)
	{
		return this->fileError(msg, AddmusicErrorcode::PARSEASM_ERROR, fatality);
	};

	pos+=4;
	if (isspace(text[pos]) == false)
		throw parseASMError("Error parsing asm directive.");

	skipSpaces();

	std::string tempname;

	while (isspace(text[pos]) == false)
	{
		if (pos >= text.length())
			break;

		tempname += text[pos++];
	}

	skipSpaces();

	if (text[pos] != '{')
		throw parseASMError("Error parsing asm directive.");

	int startPos = ++pos;

	while (text[pos] != '}')
	{
		if (pos >= text.length())
			throw parseASMError("Error parsing asm directive.");

		pos++;
	}

	int endPos = pos;
	pos++;

	asmStrings.push_back(text.substr(startPos, endPos - startPos));
	asmNames.push_back(tempname);
}

void SoundEffect::compileASM()
{
	//int codeSize = 0;

	std::vector<unsigned int> codePositions;

	for (unsigned int i = 0; i < asmStrings.size(); i++)
	{
		codePositions.push_back(code.size());

		std::stringstream asmCode;
		asmCode << 
			"arch spc700-raw\n\n"

			"org $000000\n" 
			"incsrc \"asm/main.asm\"\n"
			"base $" << hex4 << posInARAM + code.size() + data.size() << "\n\n"
			
			"org $008000\n\n" <<

			asmStrings[i];
		
		AsarBinding asar {asmCode.str(), "."};
		asar.compileToBin();
		
		// writeTextFile("temp.asm", asmCode.str());

		// if (!asarCompileToBIN("temp.asm", "temp.bin"))
		// 	throw fileError("asar reported an error.  Refer to temp.log for details.", AddmusicErrorcode::COMPILEASM_ERROR, false);

		std::vector<uint8_t> temp {asar.getCompiledBin()};

		// openFile("temp.bin", temp);

		temp.erase(temp.begin(), temp.begin() + 0x08000);

		for (unsigned int j = 0; j < temp.size(); j++)
			code.push_back(temp[j]);
	}

	for (unsigned int i = 0; i < asmStrings.size(); i++)
	{
		int k = -1;
		for (unsigned int j = 0; j < jmpNames.size(); j++)
		{
			if (asmNames[i] == jmpNames[j])
			{
				k = j;
				break;
			}
		}

		if (k == -1)
			throw fileError("Could not match asm and jsr names.", AddmusicErrorcode::COMPILEASM_ERROR, false);

		data[jmpPoses[k]] = (posInARAM + data.size() + codePositions[k]) & 0xFF;
		data[jmpPoses[k]+1] = (posInARAM + data.size() + codePositions[k]) >> 8;
	}
}

void SoundEffect::parseJSR()
{
	pos+=4;
	if (isspace(text[pos]) == false)
		throw fileError("Error parsing jsr command.", AddmusicErrorcode::PARSING_ERROR, false);

	skipSpaces();

	std::string tempname;

	while (isspace(text[pos]) == false)
	{
		if (pos >= text.length())
			break;

		tempname += text[pos++];
	}

	jmpNames.push_back(tempname);
	append(0xFD);
	jmpPoses.push_back(data.size());
	append(0x00);
	append(0x00);
}

void SoundEffect::parseDefine()
{
	pos += 7;
	skipSpaces();
	std::string defineName;
	while (!isspace(text[pos]) && pos < text.length())
	{
		defineName += text[pos++];
	}

	for (unsigned int z = 0; z < defineStrings.size(); z++)
		if (defineStrings[z] == defineName)
			throw fileError("A string cannot be defined more than once.", AddmusicErrorcode::PARSING_ERROR, false);

	defineStrings.push_back(defineName);
}

void SoundEffect::parseUndef()
{
	pos += 6;
	skipSpaces();
	std::string defineName;
	while (!isspace(text[pos]) && pos < text.length())
	{
		defineName += text[pos++];
	}
	unsigned int z = -1;
	for (z = 0; z < defineStrings.size(); z++)
		if (defineStrings[z] == defineName)
			goto found;

	throw fileError("The specified string was never defined.", AddmusicErrorcode::PARSING_ERROR, false);

found:
	defineStrings[z].clear();
}

void SoundEffect::parseIfdef()
{
	pos+=6;
	inDefineBlock = true;
	skipSpaces();
	std::string defineName;
	while (!isspace(text[pos]) && pos < text.length())
	{
		defineName += text[pos++];
	}

	unsigned int z = -1;

	int temp;

	for (unsigned int z = 0; z < defineStrings.size(); z++)
		if (defineStrings[z] == defineName)
			goto found;

	temp = text.find("#endif", pos);

	if (temp == -1)
		throw fileError("#ifdef was missing a matching #endif.", AddmusicErrorcode::PARSING_ERROR, false);

	pos = temp;
found:
	return;
}

void SoundEffect::parseIfndef()
{
	pos+=7;
	inDefineBlock = true;
	skipSpaces();
	std::string defineName;
	while (!isspace(text[pos]) && pos < text.length())
	{
		defineName += text[pos++];
	}

	unsigned int z = -1;

	for (unsigned int z = 0; z < defineStrings.size(); z++)
		if (defineStrings[z] == defineName)
			goto found;
	return;

found:
	int temp = text.find("#endif", pos);

	if (temp == -1)
		throw fileError("#ifdef was missing a matching #endif.", AddmusicErrorcode::PARSING_ERROR, false);

	pos = temp;
}

void SoundEffect::parseEndif()
{
	pos += 6;
	if (inDefineBlock == false)
		throw fileError("#endif was found without a matching #ifdef or #ifndef", AddmusicErrorcode::PARSING_ERROR, false);
	else
		inDefineBlock = false;
}
