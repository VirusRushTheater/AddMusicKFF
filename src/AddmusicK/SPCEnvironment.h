#pragma once

#include <map>
#include <vector>
#include <memory>
#include <filesystem>

#include "BankDefine.h"
#include "SoundEffect.h"
#include "Sample.h"
#include "Music.h"

namespace fs = std::filesystem;

namespace AddMusic
{

/**
 * @brief Initialization options that combine the SPC and ROM hacking
 * functionality. Normally you won't need to change any of these.
 */
struct SPCEnvironmentOptions
{
	bool aggressive {false};
	bool allowSA1 {true};
	bool verbose {true};

	bool optimizeSampleUsage {true};
	bool validateHex {true};
	bool convert {true};
	bool dupCheck {true};
	bool checkEcho {true};

	bool useCustomSPCDriver {false};
	fs::path customSPCDriverPath;
	
	bool sfxDump {false};
	bool doNotPatch {false};
	uint32_t bankStart {0x200000};
};

/**
 * @brief The heart of the program. It encompasses some utility functions
 * related specifically to SPC generation, global options which
 * were previously defined on the program's argument list; and the environment
 * in which the songs are compiled.
 * 
 * If you want ROM hacking capabilities, you might need to instance a
 * ROMEnvironment instead, which inherits this class and adds those capabilities
 * on top.
 */
class SPCEnvironment
{
	friend class Music;

public:
	/**
	 * @brief Instance SPCEnvironment with the default options.
	 */
	SPCEnvironment(const fs::path& work_dir);

	/**
	 * @brief Instance SPCEnvironment with different options.
	 */
	SPCEnvironment(const fs::path& work_dir, const SPCEnvironmentOptions& opts);

	/**
	 * @brief Destructor.
	 */
	~SPCEnvironment();

	/**
	 * @brief Equivalent of running AddMusicK with the "justSPCsPlease" option. Does not need a SMW ROM to run.
	 */
	bool generateSPCFiles(const std::vector<fs::path>& textFilesToCompile, const fs::path& output_folder = ".");

	/**
	 * Loads a sample list file.
	 */
	void loadSampleList(const fs::path& samplelistfile);

	/**
	 * Loads a music list file.
	 */
	void loadMusicList(const fs::path& musiclistfile);

	/**
	 * Loads a SFX list file. 
	 */
	void loadSFXList(const fs::path& sfxlistfile);

	int SNESToPC(int addr);

	int PCToSNES(int addr);

	SPCEnvironmentOptions options;							// User-defined options.

protected:
	/**
	 * Useful to retrieve patch info embedded in SNES/patch.asm.
	 * Equivalent to assembleSNESDriver().
	 */
	bool _assembleSNESDriver();

	/**
	 * Useful to retrieve certain info with the help of the asar assembler.
	 * Equivalent to assembleSPCDriver()
	 */
	bool _assembleSPCDriver();

	bool _compileSFX();

	bool _compileGlobalData();

	bool _compileMusic();

	bool _fixMusicPointers();

	bool _generateSPCs();

	fs::path driver_srcdir;									// Root directory from which driver ASM files will be found.
	fs::path driver_builddir;								// Directory in which generated driver files will be put.
	fs::path work_dir;										// Root directory from which user-editable files will be found.
	fs::path spc_output_dir;								// Where to store the resulting SPCs.
	
	bool spc_build_plan {false};
	bool using_custom_spc_driver {false};

	int programUploadPos;

	// Use the SA1 expansion chip. Will be true unless either the ROM says the opposite
	// or you don't want it.
	bool usingSA1 {true};

	// Information you get on the first pass of main.asm
	int programPos;
	int mainLoopPos;
	int reuploadPos;
	bool noSFX;
	size_t programSize;

	// Sample system.
	// Will eventually refactor this with a more sophisticated method.
	std::vector<Sample> samples;
	std::map<fs::path, int> sampleToIndex;
	std::vector<std::unique_ptr<BankDefine>> bankDefines;

	// Music system.
	// Will also refactor this with a more sophisticated method.
	int highestGlobalSong {0};
	int songCount {0};
	Music* musics;		// Refactored it to be allocated in the heap.

	// SFX system
	// Will also refactor this with a more sophisticated method.
	SoundEffect* soundEffectsDF9;
	SoundEffect* soundEffectsDFC;
	SoundEffect *soundEffects[2];

	// Stored this variable to mimic some behavior before I refactor more things.
	bool justSPCsPlease {true};

	int songSampleListSize;
};

}