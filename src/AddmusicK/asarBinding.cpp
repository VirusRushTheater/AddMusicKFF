#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <numeric>

#include "AddmusicLogging.h"
#include "asarBinding.h"
#include "Utility.h"

// ASAR dependencies
// #include <asar-dll-bindings/c/asardll.h>
#include <asar/interface-lib.h>

using namespace AddMusic;

AsarBinding::AsarBinding(const std::string& patchcontent, const fs::path& environment_dir) :
	_patchcontent(patchcontent),
	is_using_tmpfile(true)
{
	// Creates a temporary file in the environment_dir of your choice.
	_patchfilename = environment_dir / fs::path(std::string(std::tmpnam(nullptr)) + ".asm").filename();
	
	std::ofstream tfile {_patchfilename, std::ios::trunc};
	tfile << patchcontent;
	tfile.close();
}

AsarBinding::AsarBinding(const fs::path& file) :
	_patchfilename(file),
	is_using_tmpfile(false)
{
	readTextFile(_patchfilename, _patchcontent);
}

AsarBinding::~AsarBinding()
{
	if (is_using_tmpfile)
		fs::remove(_patchfilename);
}

bool AsarBinding::compileToBin()
{
	int binlen = 0;
	int buflen = 0x10000;		// 0x10000 instead of 0x8000 because a few things related to sound effects are stored at 0x8000 at times.

	int count = 0, currentCount = 0;		// Count to get Asar's stdout and stderr.

	asar_stdout.clear();
	asar_stderr.clear();
	_compiledbin.clear();

	// auto binOutput {std::make_unique< uint8_t[] >(buflen)};		// C++ fashion array allocation. Deletion is automatic, don't worry.
	uint8_t* binOutput = new uint8_t[buflen]();

	std::string abspatch_path = std::filesystem::absolute(_patchfilename).string();
	asar_patch(abspatch_path.c_str(), (char *)binOutput, buflen, &binlen);

	// Clears the buffers.
	asar_stderr.clear();
	asar_stdout.clear();

	asar_getprints(&count);
	for (currentCount = 0; currentCount != count; currentCount++)
		asar_stdout.push_back(asar_getprints(&count)[currentCount]);

	asar_geterrors(&count);
	for (currentCount = 0; currentCount != count; currentCount++)
		asar_stderr.push_back(asar_geterrors(&count)[currentCount].fullerrdata);

	if (asar_stderr.size() > 0)
	{
		Logging::warning(std::string("ASM compiling with Asar returned errors.") + getStderr());
		return false;
	}
	
	_compiledbin.assign(binOutput, binOutput + binlen);
	delete[] (binOutput);
	
	return true;
}

bool AsarBinding::compileToFile(const fs::path& destfile)
{
	bool compilation_result = compileToBin();
	if (compilation_result)
	{
		writeBinaryFile(destfile, _compiledbin);
		return true;
	}
	else
		return false;
}

// bool asarCompileToBIN(const File &patchName, const File &binOutputFile, bool dieOnError);

bool AsarBinding::patchToRom(fs::path rompath, bool overwrite)
{
	int binlen = 0;
	int buflen;
	int count, currentCount;

	std::vector<uint8_t> patchrom;
	readBinaryFile(rompath, patchrom);

	asar_stdout.clear();
	asar_stderr.clear();

	buflen = patchrom.size();
	asar_patch(_patchfilename.string().c_str(), (char *)patchrom.data(), buflen, &buflen);

	asar_getprints(&count);
	for (currentCount = 0; currentCount != count; currentCount++)
		asar_stdout.push_back(asar_getprints(&count)[currentCount]);

	asar_geterrors(&count);
	for (currentCount = 0; currentCount != count; currentCount++)
		asar_stderr.push_back(asar_geterrors(&count)[currentCount].fullerrdata);

	if (asar_stderr.size() > 0)
	{
		Logging::warning(std::string("ROM patching with Asar returned errors.") + getStderr());
		return false;
	}
	
	if (overwrite)
	{
		writeBinaryFile(rompath, patchrom);
	}
	else
	{
		// Creates an original patched rom name to avoid overwriting.
		int pcount = 0;
		fs::path baseromname = rompath.replace_extension("");
		fs::path romnameext = rompath.extension();

		fs::path new_rompath;
		do
		{
			new_rompath = baseromname.string() + " (patch " + std::to_string(++pcount) + ")" + romnameext.string();
		} while(fs::exists(new_rompath));
		writeBinaryFile(new_rompath, patchrom);
	}

	return true;
}

std::vector<uint8_t> AsarBinding::getCompiledBin() const
{
	if (_compiledbin.size() == 0)
		throw AsarException("A binary has not been generated.");
	return _compiledbin;
}

std::string AsarBinding::getStderr() const
{
	std::string retval;
	for (const std::string& msg : asar_stderr)
		retval += msg + "\n";
	return retval;
}

std::string AsarBinding::getStdout() const
{
	std::string retval;
	for (const std::string& msg : asar_stdout)
		retval += msg + "\n";
	return retval;
}

void AsarBinding::printErrors() const
{
	if (!hasErrors())
		return;
	for (const std::string& msg : asar_stdout)
		std::cerr << msg << std::endl;
}

bool AsarBinding::hasErrors() const
{
	return (asar_stderr.size() > 0);
}

size_t AsarBinding::getProgramSize() const
{
	return _compiledbin.size();
}