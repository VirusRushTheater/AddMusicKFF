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

class SPCEnvironment
{
public:
	SPCEnvironment(const fs::path& work_dir);
	
	/**
	 * @brief Loads the "Addmusic_sample groups.txt" file.
	 */
	void loadSampleList(const fs::path& samplelist_file);

	/**
	 * @brief Loads the "Addmusic_sound effects.txt" file. 
	 */
	void loadSFXList(const fs::path& sfxlist_file);

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
	bool noSFX;
	bool sfxDump;
};

}