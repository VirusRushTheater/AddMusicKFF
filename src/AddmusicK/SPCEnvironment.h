#pragma once

#include <vector>
#include <memory>
#include <filesystem>

#include "BankDefine.h"
#include "SoundEffect.h"
#include "Music.h"

namespace fs = std::filesystem;

namespace AddMusic
{

class SPCEnvironment
{
public:
	SPCEnvironment(const fs::path& work_dir);
	
	void loadSampleList(const fs::path& samplelist_file);
	void loadSFXList(const fs::path& sfxlist_file);

	int assembleSNESDriver();
	void assembleSPCDriver();

	void compileSFX();
	void compileGlobalData();

	void compileMusic();

private:
	fs::path src_dir;										// Root directory from which source ASM files will be found.
	fs::path build_dir;										// Directory in which generated files will be put.
	fs::path work_dir;										// Root directory from which user-editable files will be found.

	std::vector<std::unique_ptr<BankDefine>> bankDefines;	// Tree with bank names and their respective list of sample file names.

	SoundEffect soundEffectsDF9[256];						// 1DF9 sfx
	SoundEffect soundEffectsDFC[256];						// 1DFC sfx
	SoundEffect *soundEffects[2] = {soundEffectsDF9, soundEffectsDFC};

	std::vector<Music*> global_songs;						// My treatment of global songs differs in which 

	bool verbose;											// Always true

	// Internals from assembly stuff
	int programUploadPos;									// Set on assembleSNESDriver()
	int programPos;
	int mainLoopPos;
	int reuploadPos;

	size_t programSize;
	bool noSFX;
	bool sfxDump;
};

}