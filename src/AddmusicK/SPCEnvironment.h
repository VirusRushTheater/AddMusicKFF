#pragma once

#include <vector>
#include <memory>
#include <filesystem>

// #include "BankDefine.h"
// #include "SoundEffect.h"
// #include "Sample.h"
// #include "Music.h"

namespace fs = std::filesystem;

namespace AddMusic
{

/**
 * @brief By default, SPCOptions will load with these parameters.
 */
struct SPCEnvironmentOptions
{
	bool justSPCsPlease {false};
	bool aggressive {false};
	bool allowSA1 {true};
	bool verbose {true};
	
	bool sfxDump {false};
};

/**
 * @brief The heart of the program. It encompasses some utility functions
 * related specifically to SPC and Rom hacking (which are different to the
 * general utility functions located in Utility.cpp), global options which
 * were previously defined on the program's argument list; and the environment
 * in which the songs are compiled.
 * 
 * The idea is to have the driver source files (driver_srcdir) separated from
 * the user's music files to avoid confusion and polluting any of these folders
 * with procedurally generated files.
 * 
 * TODO: Will eventually make some way to pack the driver source in compile time
 * and extract it afterwards. 
 */
class SPCEnvironment
{
public:
	/**
	 * @brief Instance SPCEnvironment with the default options.
	 */
	SPCEnvironment(const fs::path& work_dir, const fs::path& driver_srcdir);

	/**
	 * @brief Instance SPCEnvironment with different options.
	 */
	SPCEnvironment(const fs::path& work_dir, const fs::path& driver_srcdir, const SPCEnvironmentOptions& opts);

	/**
	 * Loads a sample list file.
	 */
	void loadSampleList(const fs::path& samplelistfile);

	/**
	 * Loads a music list file.
	 */
	void loadMusicList(const fs::path& musiclistfile);
private:

	/**
	 * Useful to retrieve certain info with the help of the asar assembler.
	 * Equivalent to assembleSPCDriver()
	 */
	bool _assembleSPCDriver1stPass();

	SPCEnvironmentOptions options;							// User-defined options.

	fs::path driver_srcdir;									// Root directory from which driver ASM files will be found.
	fs::path driver_builddir;								// Directory in which generated driver files will be put.
	fs::path work_dir;										// Root directory from which user-editable files will be found.

	// Information you get on the first pass of main.asm
	int programPos;
	int mainLoopPos;
	int reuploadPos;
	bool noSFX;
	size_t programSize;

};

}