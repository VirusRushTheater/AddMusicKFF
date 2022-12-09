#pragma once

namespace AddMusic {

enum class AddmusicErrorcode
{
	SUCCESS = 0,				// No problem
	UNKNOWN,					// Unknown error
	ROM_NOT_LOADED,				// ROM not loaded
	ASSERTION_FAILED,			// Assertion failed
	ROM_TOO_SMALL,				// ROM is too small (<= 512 kB)
	IO_ERROR,					// Filesystem error
	NO_AM4_HEADER,				// No AddMusic 4.05 header
	NO_AMM_HEADER,				// No AddMusicM header
	NO_INIT_ASM,				// No INIT.asm to remove AddMusicM from the ROM.
	NO_AMK_SIGNATURE,			// A ROM that is supposed to have been edited previously by this tool has no "@AMK" signature. 
	MUSICLIST_PARSING_ERROR,	// Error parsing Addmusic_song list.txt
	SAMPLE_PARSING_ERROR,		// Error parsing Addmusic_sample groups.txt
	ASAR_ERROR,					// Error from asar compilation
	SFX_PARSING_ERROR,			// Error parsing Addmusic_sound effects.txt
	SFX_COMPILATION_ERROR,		// Failed to compile some SFX file
	SFX_NOT_FOUND_ERROR,		// Some SFX file was not found
	SFX_TABLE0_NOTFOUND,		// SFX Table0 not found in main.asm
	SFX_TABLE1_NOTFOUND,		// SFX Table1 not found in main.asm
	ECHO_BUFFER_OVERFLOW,		// Echo buffer exceeded total space in ARAM
	MUSICPTRS_NOTFOUND,			// MusicPtrs: could not be found
	ROM_OUT_OF_FREE_SPACE,		// Your ROM is out of free space
	FINDFREESPACE_ERROR,		// Error on findFreeSpace function
	ADDSAMPLE_ERROR,			// Error on addSample function
	ADDSAMPLEGROUP_ERROR,		// Error on addSampleGroup function

	MUSIC_INFINITE_RECURSION,	// Infinite recursion in your music file
	MUSIC_TARGET_NOT_SPECIFIED,	// Song did not specify target program with #amk, #am4, or #amm.
	MUSIC_NEWER_AMK_VERSION,	// This song was made for a newer version of AddmusicK.
	MUSIC_UNKNOWN_HEX_COMMAND,	// Unknown hex command
	MUSIC_UNEXPECTED_CHARACTER,	// Parser hit an unexpected character
	MUSIC_ILLEGAL_COMMENT,		// Illegal use of comments. Sorry about that. Should be fixed in AddmusicK 2
	MUSIC_CHANNELDIRECTIVE_ERROR,		// channel directive error
	MUSIC_LDIRECTIVE_ERROR,		// L directive error
	MUSIC_GLOBALVOLUME_ERROR,		// L directive error
	MUSIC_VOLUME_ERROR,		// L directive error
};

class AddmusicException : public std::exception
{
	public:
	AddmusicException(
		const std::string& message,
		AddmusicErrorcode code = AddmusicErrorcode::UNKNOWN,
		const std::string& filename = "",
		unsigned int line = 0
	) : _msg(message),
		_code(code),
		_fullmsg(message)
	{
		if (filename != "")
			_fullmsg += "  [@" + filename + ", L" + std::to_string(line) + "]";
		std::exception();
	}

	AddmusicException(
		std::stringstream message,
		AddmusicErrorcode code = AddmusicErrorcode::UNKNOWN,
		const std::string& filename = "",
		unsigned int line = 0
	) : AddmusicException(message.str(), code, filename, line)
	{
	}

	const std::string getMessage() const
	{
		return std::string {_msg};
	}

	AddmusicErrorcode getCode() const
	{
		return _code;
	}

	std::string getCodeAsText(AddmusicErrorcode code) const
	{
		switch (code)
		{
			case AddmusicErrorcode::SUCCESS:					return ("SUCCESS");
			case AddmusicErrorcode::UNKNOWN:					return ("UNKNOWN");
			case AddmusicErrorcode::ROM_NOT_LOADED:				return ("ROM_NOT_LOADED");
			case AddmusicErrorcode::ASSERTION_FAILED:			return ("ASSERTION_FAILED");
			case AddmusicErrorcode::ROM_TOO_SMALL:				return ("ROM_TOO_SMALL");
			case AddmusicErrorcode::IO_ERROR:					return ("IO_ERROR");
			case AddmusicErrorcode::NO_AM4_HEADER:				return ("NO_AM4_HEADER");
			case AddmusicErrorcode::NO_AMM_HEADER:				return ("NO_AMM_HEADER");
			case AddmusicErrorcode::NO_INIT_ASM:				return ("NO_INIT_ASM");
			case AddmusicErrorcode::NO_AMK_SIGNATURE:			return ("NO_AMK_SIGNATURE");
			case AddmusicErrorcode::MUSICLIST_PARSING_ERROR:	return ("MUSICLIST_PARSING_ERROR");
			case AddmusicErrorcode::SAMPLE_PARSING_ERROR:		return ("SAMPLE_PARSING_ERROR");
			case AddmusicErrorcode::ASAR_ERROR:					return ("ASAR_ERROR");
			case AddmusicErrorcode::SFX_PARSING_ERROR:			return ("SFX_PARSING_ERROR");
			case AddmusicErrorcode::SFX_COMPILATION_ERROR:		return ("SFX_COMPILATION_ERROR");
			case AddmusicErrorcode::SFX_NOT_FOUND_ERROR:		return ("SFX_NOT_FOUND_ERROR");
			case AddmusicErrorcode::SFX_TABLE0_NOTFOUND:		return ("SFX_TABLE0_NOTFOUND");
			case AddmusicErrorcode::SFX_TABLE1_NOTFOUND:		return ("SFX_TABLE1_NOTFOUND");
			case AddmusicErrorcode::ECHO_BUFFER_OVERFLOW:		return ("ECHO_BUFFER_OVERFLOW");
			case AddmusicErrorcode::ROM_OUT_OF_FREE_SPACE:		return ("ROM_OUT_OF_FREE_SPACE");
			case AddmusicErrorcode::FINDFREESPACE_ERROR:		return ("FINDFREESPACE_ERROR");
			case AddmusicErrorcode::ADDSAMPLE_ERROR:			return ("ADDSAMPLE_ERROR");
			case AddmusicErrorcode::ADDSAMPLEGROUP_ERROR:		return ("ADDSAMPLEGROUP_ERROR");

			case AddmusicErrorcode::MUSIC_INFINITE_RECURSION:	return ("MUSIC_INFINITE_RECURSION");
			case AddmusicErrorcode::MUSIC_TARGET_NOT_SPECIFIED:	return ("MUSIC_TARGET_NOT_SPECIFIED");
			case AddmusicErrorcode::MUSIC_NEWER_AMK_VERSION:	return ("MUSIC_NEWER_AMK_VERSION");
			case AddmusicErrorcode::MUSIC_UNKNOWN_HEX_COMMAND:	return ("MUSIC_UNKNOWN_HEX_COMMAND");
			case AddmusicErrorcode::MUSIC_UNEXPECTED_CHARACTER:	return ("MUSIC_UNEXPECTED_CHARACTER");
			case AddmusicErrorcode::MUSIC_ILLEGAL_COMMENT:		return ("MUSIC_ILLEGAL_COMMENT");
			case AddmusicErrorcode::MUSIC_CHANNELDIRECTIVE_ERROR:		return ("MUSIC_CHANNELDIRECTIVE_ERROR");
			case AddmusicErrorcode::MUSIC_LDIRECTIVE_ERROR:		return ("MUSIC_LDIRECTIVE_ERROR");
			case AddmusicErrorcode::MUSIC_GLOBALVOLUME_ERROR:		return ("MUSIC_GLOBALVOLUME_ERROR");
			case AddmusicErrorcode::MUSIC_VOLUME_ERROR:		return ("MUSIC_VOLUME_ERROR");
			default: return "?";
		}
	}

	const char *what() const noexcept
	{
		std::stringstream rtext;
		rtext << "[" << getCodeAsText(_code) << "] " << _fullmsg << std::endl;
		return rtext.str().c_str();
	}

	private:
	std::string _fullmsg;
	std::string _msg;
	AddmusicErrorcode _code;
};
}