#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <fstream>
#include <filesystem>

#include "defines.h"
// 
#include "AddmusicLogging.h"

namespace fs = std::filesystem;

namespace AddMusicExperimental
{

class MMLParserBase
{
	public:
	/**
	 * Compiles an arbitrary string and returns true if the parsing was successful.
	 */
	bool compileText(const std::string& text, const fs::path& cwd = fs::current_path());

	/**
	 * Compiles a file and returns true if the parsing was successful.
	 */
	bool compileFile(const fs::path& file);

	protected:
	/**
	 * Base method of compilation, which can be extended on children classes.
	 */
	virtual bool compile();

	std::string_view getQuotedString();

	private:
	fs::path cwd;			// Useful for foreign file references. Changeable with #path directives.
	std::string text;		// MML text is integrally stored here.
};

}