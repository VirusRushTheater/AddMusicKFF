#include <stack>
#include <map>
#include <regex>
#include <exception>
#include <cmath>

#include "AddmusicException.h"
#include "MMLBase.h"

using namespace AddMusic;

void MMLBase::preprocess()
{
	// Handles #ifdefs.  Maybe more later?
	std::map<std::string, int> defines;
	std::stack<bool> okayStatus;
	
	unsigned int i = 0;					// String cursor
	int level = 0;		
	std::string newstr;					// Temporary string to hold the preprocessed "text"
	bool okayToAdd = true;			

	std::string& str = text;			// Alias of "file content" to handle the previous implementation of "preprocess"

	line = 1;							// Defined as object attribute for error logging

	// getArgument, but now as a Lambda.
	auto getArgument = [&str, &i](char endChar, bool breakOnNewLines)
	{
		std::string temp;
		for (;; i++)
		{
			// Parse until a whitespace is found
			if (endChar == ' ')
			{
				if (str[i] == ' ' || str[i] == '\t')
					break;
			}
			// Parse until a set character is found
			else
				if (str[i] == endChar)
					break;

			/*
			if (i == str.length())
				error("Unexpected end of file found.");
			*/

			// Parse until a line break is found.
			if (breakOnNewLines && (str[i] == '\r' || str[i] == '\n'))
				break;
			
			// Queue said character
			temp += str[i];
		}
		return temp;
	};
	
	i = 0;

	// Parsing
	while (true)
	{
		// EOF
		if (i == str.length())
			break;

		// Next line
		if (i < str.length())
			if (str[i] == '\n')
				line++;
		
		// Get quoted argument. Do not stop, even on line breaks
		if (str[i] == '\"')
		{
			i++;
			if (okayToAdd)
			{
				newstr += '\"';
				newstr += getArgument('\"', false) + '\"';
			}
			i++;
		}

		// Sharp (#) directive.
		else if (str[i] == '#')
		{
			std::string temp;
			i++;

			// Force AMK version into AMK1.
			if (str.substr(i, 6) == "#amk=1")					// Special handling so that we can have #amk=1.
			{
				if (addmusicversion >= 0)
					addmusicversion = 1;
				i+=5;
				continue;
			}

			// Gets next continuous string not separated by whitespaces or line breaks
			temp = getArgument(' ', true);

			// #define {temp2} {temp3}
			// temp2 is a string and temp3 an int. The definition is stored in a map.
			if (temp == "define")
			{
				if (!okayToAdd) { level++; continue; }

				skipSpaces();
				std::string temp2 = getArgument(' ', true);
				if (temp2.length() == 0)
					throw AddmusicException("#define was missing its argument.", this);

				skipSpaces();
				std::string temp3 = getArgument(' ', true);
				if (temp3.length() == 0)
					defines[temp2] = 1;
				else
				{
					int j;
					try
					{
						j = std::stoi(temp3);
					}
					catch (...)
					{
						throw AddmusicException("Could not parse integer for #define.", this);
					}
					defines[temp2] = j;
				}
			}

			// #undef {temp2}
			// Removes some variable from the map.
			else if (temp == "undef")
			{
				if (!okayToAdd) { level++; continue; }

				skipSpaces();
				std::string temp2 = getArgument(' ', true);
				if (temp2.length() == 0)
					throw AddmusicException("#undef was missing its argument.", this);
				defines.erase(temp2);
			}

			// #ifdef {temp2}
			else if (temp == "ifdef")
			{
				if (!okayToAdd) { level++; continue; }

				skipSpaces();
				std::string temp2 = getArgument(' ', true);
				if (temp2.length() == 0)
					throw AddmusicException("#ifdef was missing its argument.", this);

				okayStatus.push(okayToAdd);

				if (defines.find(temp2) == defines.end())
					okayToAdd = false;
				else
					okayToAdd = true;

				level++;
			}

			// #ifndef {temp2}
			else if (temp == "ifndef")
			{
				if (!okayToAdd) { level++; continue; }

				skipSpaces();
				std::string temp2 = getArgument(' ', true);

				okayStatus.push(okayToAdd);

				if (temp2.length() == 0)
					throw AddmusicException("#ifndef was missing its argument.", this);
				if (defines.find(temp2) != defines.end())
					okayToAdd = false;
				else
					okayToAdd = true;

				level++;
			}

			// #if {temp2} {temp3} {temp4}
			// {temp2} is a definition, {temp3} is a comparison operator and {temp4} is an integer.
			else if (temp == "if")
			{
				if (!okayToAdd) { level++; continue; }

				skipSpaces();
				std::string temp2 = getArgument(' ', true);
				if (temp2.length() == 0)
					throw AddmusicException("#if was missing its first argument.", this);

				if (defines.find(temp2) == defines.end())
					throw AddmusicException("First argument for #if was never defined.", this);

				skipSpaces();
				std::string temp3 = getArgument(' ', true);
				if (temp3.length() == 0)
					throw AddmusicException("#if was missing its comparison operator.", this);

				skipSpaces();
				std::string temp4 = getArgument(' ', true);
				if (temp4.length() == 0)
					throw AddmusicException("#if was missing its second argument.", this);

				okayStatus.push(okayToAdd);

				int j;
				try
				{
					j = std::stoi(temp4);
				}
				catch (...)
				{
					throw AddmusicException("Could not parse integer for #if.", this);
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
					throw AddmusicException("Unknown operator for #if.", this);

				level++;
			}

			// #endif
			else if (temp == "endif")
			{
				if (level > 0)
				{
					level--;
					okayToAdd = okayStatus.top();
					okayStatus.pop();
				}
				else
					throw AddmusicException("There was an #endif without a matching #ifdef, #ifndef, or #if.", this);
			}

			// #amk{temp}
			else if (temp == "amk")
			{
				if (addmusicversion >= 0)
				{
					skipSpaces();
					std::string temp = getArgument(' ', true);
					if (temp.length() == 0)
					{
						throw AddmusicException("#amk must have an integer argument specifying the version.", false, this);
					}
					else
					{
						int j;
						try
						{
							j = std::stoi(temp);
						}
						catch (...)
						{
							throw AddmusicException("Could not parse integer for #amk.", this);
						}

						addmusicversion = j;
						if (addmusicversion == 3)
						{
							throw AddmusicException("Codec's AddmusicK Beta has not been implemented yet.", this);
						}
					}
				}
			}

			// #amm
			else if (temp == "amm")
				addmusicversion = -2;
			
			// #am4
			else if (temp == "am4")
				addmusicversion = -1;


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
			if (okayToAdd || str[i] == '\n')
				newstr += str[i];
			i++;
		}
	}

	// Finishes the deal
	str = newstr;
}

std::string MMLBase::getQuotedString(const std::string &string, int startPos, int &rawLength)
{
	std::string retval;
	rawLength = startPos;

	for (;; string[++startPos] != '\"')
	{
		// EOF
		if (startPos > string.length())
			throw AddmusicException("Unexpected end of file found.", false, this);

		// Ignore quotes if they are escaped.
		if (string[startPos] == '\\')
		{
			if (string[startPos+1] == '"')
			{
				retval += '"';
				startPos++;
			}
			else
			{
				throw AddmusicException(R"(Error: The only escape sequence allowed is "\"".)", false, this);
				return retval;
			}
		}
		else
			retval += string[startPos];
	}
	rawLength = startPos - rawLength;
	return retval;
}

int MMLBase::parseHex()
{
	int i = 0;
	int d = 0;
	int j;

	while (pos < text.length())
	{
		if      ('0' <= text[pos] && text[pos] <= '9') j = text[pos] - 0x30;
		else if ('A' <= text[pos] && text[pos] <= 'F') j = text[pos] - 0x37;
		else if ('a' <= text[pos] && text[pos] <= 'f') j = text[pos] - 0x57;
		else break;
		pos++;
		d++;
		i = (i*16) + j;
	}
	if (d == 0) return -1;

	return i;
}

int MMLBase::parseInt()
{
	// EOL
	if (pos >= text.length())
		return -1;
	//if (text[pos] == '$') { pos++; return getHex(); }	// Allow for things such as t$20 instead of t32.

	int i = 0;
	int d = 0;

	while (text[pos] >= '0' && text[pos] <= '9')
	{
		d++;
		i = (i * 10) + text[pos] - '0';
		pos++;
		if (pos >= text.length()) break;
	}

	if (d == 0) return -1; else return i;
}

int MMLBase::parsePitch(int letter, int octave)
{
	static const int pitches[] = {9, 11, 0, 2, 4, 5, 7};

	letter = pitches[letter - 0x61] + (octave - 1) * 12 + 0x80;

	pos++;
	if (text[pos] == '+') {letter++; pos++;}
	else if (text[pos] == '-') {letter--; pos++;}
	if (letter < 0x80)
		return -1;
	if (letter >= 0xC6)
		return -2;

	return letter;
}

int MMLBase::parseNoteLength(int i)
{
	i = parseInt();
	if (i == -1 && text[pos] == '=')
	{
		pos++;
		i = parseInt();
		if (i == -1)
			throw AddmusicException("Error parsing note length.", false, this);
		return i;
	}

	if (i < 1 || i > 192)
		i = defaultNoteLength;
	i = 192 / i;

	int frac = i;

	while (text[pos] == '.')
	{
		frac = frac / 2;
		i += frac;
		pos++;
	}

	if (triplet)
		i = (int)floor(((double)i * 2.0 / 3.0) + 0.5);
	return i;
}

