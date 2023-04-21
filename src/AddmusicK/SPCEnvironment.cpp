#include <cstring>
#include <algorithm>

#include "AddmusicLogging.h"
#include "asarBinding.h"
#include "SPCEnvironment.h"
#include "Utility.h"

#include <iostream>

using namespace AddMusic;

// Will be evaluated relatively to work_dir to find source codes.
constexpr const char DEFAULT_BUILD_FOLDER[] {"build"};

// Default file names
constexpr const char* DEFAULT_SONGLIST_FILENAME {"Addmusic_list.txt"};
constexpr const char* DEFAULT_SAMPLELIST_FILENAME {"Addmusic_sample groups.txt"};
constexpr const char* DEFAULT_SFXLIST_FILENAME {"Addmusic_sound effects.txt"};

SPCEnvironment::SPCEnvironment(const fs::path& work_dir, const fs::path& driver_srcdir) :
	work_dir(work_dir),
	driver_srcdir(driver_srcdir),	
	driver_builddir(driver_srcdir / DEFAULT_BUILD_FOLDER)
{
	// Delegate these "if (verbose)" clauses to this Logging singleton.
	if (options.verbose)
		Logging::setVerbosity(Logging::Levels::DEBUG);
	
	// Does your work directory have these files?
	if (!fs::exists(work_dir))
		throw fs::filesystem_error("The directory with music has not been found", work_dir, std::error_code());
	if (!fs::exists(work_dir / DEFAULT_SONGLIST_FILENAME))
		throw fs::filesystem_error("The song list file was not found within the work directory.", work_dir / DEFAULT_SONGLIST_FILENAME, std::error_code());
	if (!fs::exists(work_dir / DEFAULT_SAMPLELIST_FILENAME))
		throw fs::filesystem_error("The sample list file was not found within the work directory.", work_dir / DEFAULT_SAMPLELIST_FILENAME, std::error_code());
	if (!fs::exists(work_dir / DEFAULT_SFXLIST_FILENAME))
		throw fs::filesystem_error("The SFX list file was not found within the work directory.", work_dir / DEFAULT_SFXLIST_FILENAME, std::error_code());

	// Make sure the SPC driver source folder exists as well.
	if (!fs::exists(driver_srcdir))
		throw fs::filesystem_error("SPC driver source directory not found", driver_srcdir, std::error_code());
	
	// Copy the SPC driver source entirely into the "build" folder so it can
	// be clean from any procedurally generated files.

	// I do all the hassle with the temporary directory in order to trick the
	// copy method into not recursing infinitely.
	
	// fs::rename throws exceptions when the temporary directory is not located
	// in the same filesystem as the driver.
	if (fs::exists(driver_builddir))
		deleteDir(driver_builddir);
	fs::path tempdir = fs::temp_directory_path();
	copyDir(driver_srcdir, driver_builddir);
	
	Logging::debug(std::string("Driver will be compiled in ") + fs::absolute(driver_builddir).string());
}

SPCEnvironment::SPCEnvironment(const fs::path& work_dir, const fs::path& driver_srcdir, const SPCEnvironmentOptions& opts) :
	SPCEnvironment(work_dir, driver_srcdir)
{
	options = opts;
}

bool SPCEnvironment::_assembleSNESDriver()
{
	std::string patch;
	readTextFile(driver_builddir / "asm" / "SNES" / "patch.asm", patch);
	programUploadPos = scanInt(patch, "!DefARAMRet = ");

	return true;
}

// Equivalent to assembleSPCDriver()
bool SPCEnvironment::_assembleSPCDriver()
{
	std::string patch;
	readTextFile(driver_builddir / "asm" / "main.asm", patch);
	programPos = scanInt(patch, "base ");

	// Everything is done through memory this time.
	bool firstpass_success;
	AsarBinding firstpass (driver_builddir / "asm" / "main.asm");
	firstpass_success = firstpass.compileToBin();
	if (!firstpass_success)
	{
		firstpass.printErrors();
		Logging::error("asar reported an error while assembling asm/main.asm.");
		return false;
	}

	// Scans the messages deployed by the assembly process.
	std::string firstpass_stdout = firstpass.getStdout();

	mainLoopPos = scanInt(firstpass_stdout, "MainLoopPos: ");
	reuploadPos = scanInt(firstpass_stdout, "ReuploadPos: ");
	noSFX = (firstpass_stdout.find("NoSFX is enabled") != -1);
	programSize = firstpass.getProgramSize();

	if (options.sfxDump && noSFX) {
		Logging::warning("The sound driver build does not support sound effects due to the !noSFX flag being enabled in asm/UserDefines.asm, yet you requested to dump SFX. There will be no new SPC dumps of the sound effects since the data is not included by default, nor is the playback code for the sound effects.");
		options.sfxDump = false;
		return false;
	}

	return true;
}

void SPCEnvironment::loadSampleList(const fs::path& samplelistfile)
{
	std::string str;
	readTextFile(samplelistfile, str);

	std::string groupName;
	std::string tempName;

	unsigned int i = 0;
	bool gettingGroupName = false;
	bool gettingSampleName = false;

	// Important routines in parsers will be put as lambdas here.
	auto createSampleGroup = [](const std::string& group_name) -> void {

	};

	while (i < str.length())
	{
		if (gettingGroupName == false && gettingSampleName == false)
		{
			if (groupName.length() == 0)
			{
				if (isspace(str[i]))
				{
					i++;
					continue;
				}
				else if (str[i] == '#')
				{
					i++;
					gettingGroupName = true;
					groupName.clear();
					continue;
				}
			}
			else
			{
				if (isspace(str[i]))
				{
					i++;
					continue;
				}
				else if (str[i] == '{')
				{
					i++;
					gettingSampleName = true;
					continue;
				}
				else
				{
					Logging::error("Error parsing sample groups.txt.  Expected opening curly brace.");
				}
			}
		}
		else if (gettingGroupName == true)
		{
			if (isspace(str[i]))
			{
				// Create a bank definition and give it a name.
				std::unique_ptr<BankDefine> sg = std::make_unique<BankDefine>();
				sg->name = groupName;
				bankDefines.push_back(std::move(sg));
				i++;
				gettingGroupName = false;
				continue;
			}
			else
			{
				groupName += str[i];
				i++;
				continue;
			}
		}
		else if (gettingSampleName)
		{
			if (tempName.length() > 0)
			{
				if (str[i] == '\"')
				{
					tempName.erase(tempName.begin(), tempName.begin() + 1);
					bankDefines[bankDefines.size() - 1]->samples.push_back(std::make_unique<std::string>(tempName));
					bankDefines[bankDefines.size() - 1]->importants.push_back(false);
					tempName.clear();
					i++;
					continue;
				}
				else
				{
					tempName += str[i];
					i++;
				}
			}
			else
			{
				if (isspace(str[i]))
				{
					i++;
					continue;
				}
				else if (str[i] == '\"')
				{
						i++;
						tempName += ' ';
						continue;
				}
				else if (str[i] == '}')
				{
					gettingSampleName = false;
					gettingGroupName = false;
					groupName.clear();
					i++;
					continue;
				}
				else if (str[i] == '!')
				{
					if (bankDefines[bankDefines.size() - 1]->importants.size() == 0)
						Logging::error("Error parsing Addmusic_sample groups.txt: Importance specifier ('!') must come after asample declaration, not before it.");
					bankDefines[bankDefines.size() - 1]->importants[bankDefines[bankDefines.size() - 1]->importants.size() - 1] = true;
					i++;
				}
				else
				{
					Logging::error("Error parsing sample groups.txt.  Expected opening quote.");
				}
			}
		}
	}
}

void SPCEnvironment::loadMusicList(const fs::path& musiclistfile)
{
	std::string musicFile;
	readTextFile(musiclistfile, musicFile);

	if (musicFile[musicFile.length()-1] != '\n')
		musicFile += '\n';

	unsigned int i = 0;

	bool inGlobals = false;
	bool inLocals = false;
	bool gettingName = false;
	int index = -1;
	int shallowSongCount = 0;

	std::string tempName;

	// TODO: To be uncommented once I find out how to make samples and SFX work.
	/*
	while (i < musicFile.length())
	{
		if (isspace(musicFile[i]) && !gettingName)
		{
			i++;
			continue;
		}

		if (strncmp(musicFile.c_str() + i, "Globals:", 8) == 0)
		{
			inGlobals = true;
			inLocals = false;
			i+=8;
			continue;
		}

		if (strncmp(musicFile.c_str() + i, "Locals:", 7) == 0)
		{
			inGlobals = false;
			inLocals = true;
			i+=7;
			continue;
		}

		if (!inGlobals && !inLocals)
			Logging::error("Error: Could not find \"Globals:\" label in list.txt");

		if (index < 0)
		{
			if      ('0' <= musicFile[i] && musicFile[i] <= '9') index = musicFile[i++] - '0';
			else if ('A' <= musicFile[i] && musicFile[i] <= 'F') index = musicFile[i++] - 'A' + 10;
			else if ('a' <= musicFile[i] && musicFile[i] <= 'f') index = musicFile[i++] - 'a' + 10;
			else Logging::error("Invalid number in list.txt.");

			index <<= 4;


			if      ('0' <= musicFile[i] && musicFile[i] <= '9') index |= musicFile[i++] - '0';
			else if ('A' <= musicFile[i] && musicFile[i] <= 'F') index |= musicFile[i++] - 'A' + 10;
			else if ('a' <= musicFile[i] && musicFile[i] <= 'f') index |= musicFile[i++] - 'a' + 10;
			else if (isspace(musicFile[i])) index >>= 4;
			else Logging::error("Invalid number in list.txt.");

			if (!isspace(musicFile[i]))
				Logging::error("Invalid number in list.txt.");
			if (inGlobals)
				highestGlobalSong = std::max(highestGlobalSong, index);
			if (inLocals)
				if (index <= highestGlobalSong)
					Logging::error("Error: Local song numbers must be greater than the largest global song number.");
		}
		else
		{
			if (musicFile[i] == '\n' || musicFile[i] == '\r')
			{
				musics[index].name = tempName;
				if (inLocals && justSPCsPlease == false)
				{
					openTextFile((std::string("music/") + tempName), musics[index].text);
				}
				musics[index].exists = true;
				index = -1;
				i++;
				shallowSongCount++;
				gettingName = false;
				tempName.clear();
				continue;

			}
			gettingName = true;
			tempName += musicFile[i++];
		}
	}

	Logging::verbose(std::string("Read in all ") + std::to_string(shallowSongCount) + " songs.");

	for (int i = 255; i >= 0; i--)
	{
		if (musics[i].exists)
		{
			songCount = i+1;
			break;
		}
	}
	*/
}