#pragma once

#include <string>
#include <cstdlib>

// Local definitions
#include "Music.h"
#include "Sample.h"
#include "SoundEffect.h"
#include "SampleGroup.h"
#include "Directory.h"
#include "BankDefine.h"
#include "Directory.h"

// ASAR dependencies
#include <asar/interface-lib.h>

// Grab yer ducktape, folks!
#ifdef _WIN32
#define strnicmp _strnicmp
#else
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#define AMKVERSION 		1
#define AMKMINOR 		0
#define AMKREVISION 	8			

#define PARSER_VERSION 	4			// Used to keep track of incompatible changes to the parser

#define DATA_VERSION 	0			// Used to keep track of incompatible changes to any and all compiled data, either to the SNES or to the PC

#define error(str) printError(str, true, filename, line)

#define hex2 std::setw(2) << std::setfill('0') << std::uppercase << std::hex
#define hex4 std::setw(4) << std::setfill('0') << std::uppercase << std::hex
#define hex6 std::setw(6) << std::setfill('0') << std::uppercase << std::hex

/**
 * @brief Addmusic core class
 * Made so the working of the program can be instanced in both the CLI and as
 * a library.
 */
class AddMusicK
{
	public:
	AddMusicK(
		std::string _ROMName = 			"",
		bool _convert = 				true,
		bool _checkEcho = 				true,
		int  _bankStart = 				0x200000,
		bool _verbose = 				false,
		bool _aggressive = 				false,
		bool _dupCheck = 				true,
		bool _validateHex = 			true,
		bool _doNotPatch = 				false,
		bool _optimizeSampleUsage = 	true,
		bool _allowSA1 = 				true,
		bool _sfxDump = 				false,
		bool _visualizeSongs = 			false,
		bool _justSPCsPlease = 			false
	) :
		ROMName (_ROMName),
		convert (_convert),
		checkEcho (_checkEcho),
		bankStart (_bankStart),
		verbose (_verbose),
		aggressive (_aggressive),
		dupCheck (_dupCheck),
		validateHex (_validateHex),
		doNotPatch (_doNotPatch),
		optimizeSampleUsage (_optimizeSampleUsage),
		allowSA1 (_allowSA1),
		sfxDump (_sfxDump),
		visualizeSongs (_visualizeSongs),
		justSPCsPlease (_justSPCsPlease)
	{
		addMusicKMain();
	}

	~AddMusicK()
	{

	}

	// Migrated from globals.cpp
	/**
	 * @brief Returns a position in the ROM with the specified amount of free
	 * space, starting at the specified position.  NOT using SNES addresses!
	 * 
	 * This function writes a RATS address at the position returned.
	 * 
	 * @param size 		Amount of space requested
	 * @param start 	Offset where to start searching
	 * @param ROM 		ROM data
	 * 
	 * @return int		Address in the ROM where the free space is 
	 */
	static int findFreeSpace(unsigned int size, int start, std::vector<uint8_t> &ROM);

	/**
	 * @brief No idea what this function does.
	 * 
	 * @param addr 
	 * @return int 
	 */
	static int SNESToPC(int addr);

	/**
	 * @brief No idea what this function does.
	 * 
	 * @param addr 
	 * @return int 
	 */
	static int PCToSNES(int addr);

	/**
	 * @brief Tells if the 'STAR' signature is present in the given ROM offset.
	 * 
	 * @param offset 	ROM offset.
	 * @return 			True if the 'STAR' signature was found there.
	 */
	bool findRATS(int offset);

	/**
	 * @brief No idea what this function does.
	 * 
	 * @param offset 
	 * @return int 
	 */
	int clearRATS(int offset);

	void addSample(const File &fileName, Music *music, bool important);
	void addSample(const std::vector<uint8_t> &sample, const std::string &name, Music *music, bool important, bool noLoopHeader, int loopPoint, bool isBNK);

	void addSampleGroup(const File &groupName, Music *music);

	void addSampleBank(const File &fileName, Music *music);

	int getSample(const File &name, Music *music);

	void preprocess(std::string &str, const std::string &filename, int &version);

	bool asarCompileToBIN(const File &patchName, const File &binOutputFile, bool dieOnError);

	bool asarPatchToROM(const File &patchName, const File &romName, bool dieOnError);

	int addMusicKMain();

	void cleanROM();
	void tryToCleanSampleToolData();
	void tryToCleanAM4Data();
	void tryToCleanAMMData();

	void assembleSNESDriver();
	void assembleSPCDriver();
	void loadMusicList();
	void loadSampleList();
	void loadSFXList();
	void compileSFX();
	void compileGlobalData();
	void compileMusic();
	void fixMusicPointers();
	void generateSPCs();
	void assembleSNESDriver2();
	void generateMSC();
	void cleanUpTempFiles();

	void generatePNGs();

	// == END MIGRATION from globals.cpp ==

	public:
	File ROMName;

	std::vector<uint8_t> rom;	// ROM in memory
	std::vector<uint8_t> romHeader;
	std::string tempDir;		// Temporary directory path. To be used by compilation purposes.

	private:
	Music musics[256];

	std::vector<Sample> samples;
	SoundEffect soundEffectsDF9[256];
	SoundEffect soundEffectsDFC[256];
	SoundEffect *soundEffects[2] = {soundEffectsDF9, soundEffectsDFC};

	std::vector<std::unique_ptr<BankDefine>> bankDefines;
	std::map<File, int> sampleToIndex;

	bool convert = true;
	bool checkEcho = true;
	bool forceSPCGeneration = false;
	int  bankStart = 0x200000;
	bool verbose = false;
	bool aggressive = false;
	bool dupCheck = true;
	bool validateHex = true;
	bool doNotPatch = false;
	int  errorCount = 0;
	bool optimizeSampleUsage = true;
	bool usingSA1 = false;
	bool allowSA1 = true;
	bool forceNoContinuePrompt = false;
	bool sfxDump = false;
	bool visualizeSongs = false;
	bool redirectStandardStreams = false;
	bool noSFX = false;

	bool justSPCsPlease = false;

	int programPos;
	int programUploadPos;
	int mainLoopPos;
	int reuploadPos;
	int programSize;
	int highestGlobalSong;
	int totalSampleCount;
	int songCount = 0;
	int songSampleListSize;

	int bankSampleCount = 0;			// Used to give unique names to sample bank brrs.
};