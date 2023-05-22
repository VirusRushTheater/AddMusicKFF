#include "MMLParserBase.h"
#include "Utility.h"

using namespace AddMusicExperimental;

bool MMLParserBase::compileText(const std::string& text, const fs::path& cwd)
{
	this->text = text;
	this->cwd = cwd;

	return compile();
}

bool MMLParserBase::compileFile(const fs::path& file)
{
	AddMusic::readTextFile(file, this->text);
	this->cwd = file.parent_path();

	return compile();
}

bool MMLParserBase::compile()
{
	return true;
}
