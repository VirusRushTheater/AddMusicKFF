#include <cstring>
#include <algorithm>

#include "AddmusicException.h"
#include "asarBinding.h"
#include "SPCEnvironment.h"
#include "Utility.h"

#include <iostream>

using namespace AddMusic;

// Will be evaluated relatively to work_dir to find source codes.
constexpr const char DEFAULT_BUILD_FOLDER[] {"build"};

// Default file names
constexpr const char DEFAULT_SONGLIST_FILENAME[] {"Addmusic_list.txt"};
constexpr const char DEFAULT_SAMPLELIST_FILENAME[] {"Addmusic_sample groups.txt"};
constexpr const char DEFAULT_SFXLIST_FILENAME[] {"Addmusic_sound effects.txt"};

SPCEnvironment::SPCEnvironment(const fs::path& work_dir, const fs::path& driver_srcdir) :
	work_dir(work_dir),
	driver_srcdir(driver_srcdir),	
	driver_builddir(driver_srcdir / DEFAULT_BUILD_FOLDER)
{
	if (!fs::exists(work_dir))
		throw fs::filesystem_error("The directory with music has not been found", work_dir, std::error_code());
	if (!fs::exists(driver_srcdir))
		throw fs::filesystem_error("SPC driver source directory not found", driver_srcdir, std::error_code());
	
	// Clean and create an empty "build" directory ([src/build])
	if (fs::exists(driver_builddir))
		deleteDir(driver_builddir);
	fs::create_directory(driver_builddir);
}

SPCEnvironment::SPCEnvironment(const fs::path& work_dir, const fs::path& driver_srcdir, const SPCEnvironmentOptions& opts) :
	SPCEnvironment(work_dir, driver_srcdir)
{
	options = opts;
}

// Equivalent to assembleSPCDriver()
bool SPCEnvironment::_assembleSPCDriver1stPass()
{
	std::string patch;
	readTextFile(driver_srcdir / "asm" / "main.asm", patch);
	programPos = scanInt(patch, "base ");

	// Everything is done through memory this time.
	bool firstpass_success;
	AsarBinding firstpass (driver_srcdir / "asm" / "main.asm");
	firstpass_success = firstpass.compileToBin();
	if (!firstpass_success)
	{
		firstpass.printErrors();
		throw AddmusicException("asar reported an error while assembling asm/main.asm. Refer to temp.log for details.");
	}

	// Scans the messages deployed by the assembly process.
	std::string firstpass_stdout = firstpass.getStdout();

	mainLoopPos = scanInt(firstpass_stdout, "MainLoopPos: ");
	reuploadPos = scanInt(firstpass_stdout, "ReuploadPos: ");
	noSFX = (firstpass_stdout.find("NoSFX is enabled") != -1);
	programSize = firstpass.getProgramSize();

	if (options.sfxDump && noSFX) {
		throw AddmusicException("The sound driver build does not support sound effects due to the !noSFX flag\r\nbeing enabled in asm/UserDefines.asm, yet you requested to dump SFX. There will\r\nbe no new SPC dumps of the sound effects since the data is not included by\r\ndefault, nor is the playback code for the sound effects.");
		options.sfxDump = false;
	}
}

// Equivalent to compileGlobalData()
