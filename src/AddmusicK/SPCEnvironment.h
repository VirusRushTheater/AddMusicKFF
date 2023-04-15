#pragma once

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
 * @brief By default, SPCOptions will load with these parameters.
 */
struct SPCEnvironmentOptions
{
	bool justSPCsPlease {false};
	bool aggressive {false};
	bool allowSA1 {true};
	bool verbose {true};
};

/**
 * @brief The heart of the program. It encompasses some utility functions
 * related specifically to SPC and Rom hacking (which are different to the
 * general utility functions located in Utility.cpp), global options which
 * were previously defined on the program's argument list; and the environment
 * in which the songs are compiled.
 * 
 * The idea is to store the general sample list and global songs that will be
 * present concurrently to each local song; as well as define the program flow
 * independently from the CLI, in order to make this work on console as well
 * as a graphic environment or as a part of another hacking tool as an API.
 */
class SPCEnvironment
{
public:
	/**
	 * @brief Instance SPCEnvironment with the default options.
	 */
	SPCEnvironment(const fs::path& work_dir, const fs::path& src_dir);

	/**
	 * @brief Instance SPCEnvironment with different options.
	 */
	SPCEnvironment(const fs::path& work_dir, const fs::path& src_dir, const SPCEnvironmentOptions& opts);

	/**
	 * @brief Loads the "Addmusic_list.txt" file.
	 */
	void loadMusicList();

	/**
	 * @brief Loads the "Addmusic_sample groups.txt" file.
	 */
	void loadSampleList();

	/**
	 * @brief Loads the "Addmusic_sound effects.txt" file. 
	 */
	void loadSFXList();

	/**
	 * @brief Retrieves the program loading position from the "SNES/patch.asm"
	 * file. 
	 */
	int assembleSNESDriver();

	/**
	 * @brief Retrieves program positions and a preliminary program size from
	 * the results of compilation of main.asm. 
	 */
	void assembleSPCDriver();

	/**
	 * @brief Prepares the internal state of the SFXs.
	 */
	void compileSFX();

	/**
	 * @brief Compiles the sound effects and includes their data in the main
	 * assembly program "tempmain.asm".
	 * 
	 * Creates SFX1DF9Table.bin, SFX1DFCTable.bin and SFXData.bin from the
	 * compiled SFX data, which will be included as an "incbin" directive in
	 * the main assembly program -> "main.bin".
	 */
	void compileGlobalData();

	/**
	 * @brief Prepares the echo buffer size on each music file, compiles them
	 * and prepares the SNES/SongSampleList.asm
	 */
	void compileMusic();
	void fixMusicPointers();
	void generateSPCs();
	void assembleSNESDriver2();
	void generateMSC();

	

private:
	SPCEnvironmentOptions options;							// User-defined options.

	fs::path rom_path;										// Path of the ROM
	std::vector<uint8_t> rom;								// ROM contents

	fs::path src_dir;										// Root directory from which source ASM files will be found.
	fs::path build_dir;										// Directory in which generated files will be put.
	fs::path work_dir;										// Root directory from which user-editable files will be found.

	std::vector<std::unique_ptr<BankDefine>> bankDefines;	// Tree with bank names and their respective list of sample file names.

	SoundEffect soundEffectsDF9[256];						// 1DF9 sfx
	SoundEffect soundEffectsDFC[256];						// 1DFC sfx
	SoundEffect *soundEffects[2] = {soundEffectsDF9, soundEffectsDFC};

	std::vector<Sample> samples;

	std::vector<Music*> global_songs;						// Future treatment for music
	Music musics[256];

	bool verbose;											// Always true

	// Internals from assembly stuff
	int programUploadPos;									// Set on assembleSNESDriver()
	int programPos;
	int mainLoopPos;
	int reuploadPos;
	int highestGlobalSong;
	int songSampleListSize;

	int songCount {0};
	bool checkEcho {true};

	size_t programSize;
	bool usingSA1;
	bool noSFX;
	bool sfxDump;
};

}